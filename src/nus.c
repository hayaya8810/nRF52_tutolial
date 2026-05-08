/**
******************************************************************************
* @file				: nus.c
* @brief			: Nordic UART Service notifications.
******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/services/nus.h>

/* Private includes ----------------------------------------------------------*/
#include "nus.h"

/* Private define ------------------------------------------------------------*/
#define LED_STATE_MSG_SIZE			20		// Keep messages within the default BLE notification payload

/* Private variables ---------------------------------------------------------*/
/**
 * @brief		Current BLE connection, or NULL if not connected
 */
static struct bt_conn *m_current_conn;

/**
 * @brief		Whether the connected peer has enabled notifications for the NUS TX characteristic
 */
static bool m_notify_enabled;
static struct k_work_delayable m_notify_work;
static bool m_pending_led_event;			// Whether there is a pending LED event to notify (as opposed to a pending periodic state notification)
static LED_ID m_pending_led_id;
static uint32_t m_notify_count;

/**
 * @brief		BLE advertising data and scan response data
 */
static const struct bt_data m_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),	// General discoverable mode, BR/EDR not supported
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),					// Advertise NUS service UUID
};

/**
 * @brief		BLE advertising data and scan response data
 */
static const struct bt_data m_sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),	// Complete device name
};

/* Private function prototypes -----------------------------------------------*/
static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void nus_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len);
static void nus_send_enabled(enum bt_nus_send_status status);
static void notify_work_handler(struct k_work *work);
static void notify_schedule(void);
static int advertising_start(void);
static int led_state_message(char *buf, size_t buf_size);
static int led_state_mask(uint32_t *mask);

/**
 * @brief		BLE connection callbacks structure
 */
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Exported functions --------------------------------------------------------*/
/**
 * @brief		Initialize Bluetooth LE and Nordic UART Service
 * @return		0			success
 * 				negative	error code on failure
 */
int nus_init(void)
{
	int ret;
	static struct bt_nus_cb nus_cb = {
		.received = nus_received,
		.send_enabled = nus_send_enabled,
	};

	k_work_init_delayable(&m_notify_work, notify_work_handler);

	// Initialize Bluetooth subsystem
	ret = bt_enable(NULL);
	if (ret < 0) {
		return ret;
	}

	// Initialize Nordic UART Service with callbacks
	ret = bt_nus_init(&nus_cb);
	if (ret < 0) {
		return ret;
	}

	return advertising_start();
}

/**
 * @brief		Return whether a BLE peer is connected
 */
bool nus_is_connected(void)
{
	return m_current_conn != NULL;
}

/**
 * @brief		Notify the connected peer of all LED states
 * @return		0			success or skipped because no peer subscribes
 * 				negative	error code on failure
 */
int nus_notify_led_state(void)
{
	m_pending_led_event = false;
	notify_schedule();
	return 0;
}

/**
 * @brief		Notify the connected peer of the LED event and all LED states
 * @param[in]	id			LED identifier that changed
 * @return		0			success or skipped because no peer subscribes
 * 				negative	error code on failure
 */
int nus_notify_led_event(LED_ID id)
{
	if (id >= LED_ID_COUNT) {
		return -EINVAL;
	}

	m_pending_led_event = true;
	m_pending_led_id = id;
	notify_schedule();
	return 0;
}

/* Private user code ---------------------------------------------------------*/
/**
 * @brief		BLE connection callback for when a connection is established
 * @param[in]	conn  Pointer to connection object that has been established
 * @param[in]	err   Connection error code (0 if successful)
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		printf("BLE connection failed (err %u)\n", err);
		return;
	}

	m_current_conn = bt_conn_ref(conn);
	printf("BLE connected\n");
	(void)nus_notify_led_state();
}

/**
 * @brief		BLE connection callback for when a connection is disconnected
 * @param[in]	conn   Pointer to connection object that has been disconnected
 * @param[in]	reason Disconnection reason code
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printf("BLE disconnected (reason %u)\n", reason);

	if (m_current_conn != NULL) {
		bt_conn_unref(m_current_conn);
		m_current_conn = NULL;
	}
	m_notify_enabled = false;

	(void)advertising_start();
}

/**
 * @brief		BLE callback for when data is received on the NUS RX characteristic
 * @param[in]	conn  Pointer to connection object from which data was received
 * @param[in]	data  Pointer to received data buffer
 * @param[in]	len   Length of received data in bytes
 */
static void nus_received(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	ARG_UNUSED(conn);

	printf("NUS RX: %.*s\n", len, data);
}

/**
 * @brief		BLE callback for when notification status changes
 * @param[in]	status Notification status
 */
static void nus_send_enabled(enum bt_nus_send_status status)
{
	m_notify_enabled = (status == BT_NUS_SEND_STATUS_ENABLED);
	printf("NUS notifications %s\n", m_notify_enabled ? "enabled" : "disabled");

	if (m_notify_enabled) {
		(void)nus_notify_led_state();
	}
}

/**
 * @brief		Work handler that sends the latest LED state over NUS
 * @param[in]	work Work item
 */
static void notify_work_handler(struct k_work *work)
{
	char msg[LED_STATE_MSG_SIZE];
	int len;
	int pos;

	ARG_UNUSED(work);

	if (!m_notify_enabled) {
		return;
	}

	m_notify_count++;

	if (m_pending_led_event) {
		pos = snprintf(msg, sizeof(msg), "B%u ",
			       (unsigned int)m_pending_led_id);
	} else {
		pos = snprintf(msg, sizeof(msg), "N%u ",
			       (unsigned int)m_notify_count);
	}

	if (pos < 0 || pos >= sizeof(msg)) {
		return;
	}

	len = led_state_message(&msg[pos], sizeof(msg) - pos);
	if (len < 0) {
		return;
	}

	(void)bt_nus_send(NULL, (const uint8_t *)msg, pos + len);
}

/**
 * @brief		Schedule a NUS notification after CCC state has settled
 */
static void notify_schedule(void)
{
	k_work_reschedule(&m_notify_work, K_MSEC(50));
}

/**
 * @brief		Start BLE advertising with predefined parameters and data
 * @return		0			success
 * 				negative	error code on failure
 */
static int advertising_start(void)
{
	int ret;

	ret = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, m_ad, ARRAY_SIZE(m_ad), m_sd, ARRAY_SIZE(m_sd));
	if (ret < 0 && ret != -EALREADY) {
		printf("BLE advertising failed (err %d)\n", ret);
		return ret;
	}

	printf("BLE advertising as %s\n", CONFIG_BT_DEVICE_NAME);
	return 0;
}

/**
 * @brief		Construct a message containing the state of all LEDs
 * @param[out]	buf			Buffer to write the message into
 * @param[in]	buf_size	Size of the buffer in bytes
 * @return		Number of bytes written to the buffer on success
 * 				negative	error code on failure
 */
static int led_state_message(char *buf, size_t buf_size)
{
	uint32_t mask;
	int ret;

	ret = led_state_mask(&mask);
	if (ret < 0) {
		return ret;
	}

	ret = snprintf(buf, buf_size, "L=%X\n", (unsigned int)mask);
	if (ret < 0 || ret >= buf_size) {
		return -ENOMEM;
	}

	return ret;
}

static int led_state_mask(uint32_t *mask)
{
	uint32_t value = 0;

	for (size_t i = 0; i < LED_ID_COUNT; i++) {
		LED_STATE state;
		int ret = led_get((LED_ID)i, &state);

		if (ret < 0) {
			return ret;
		}

		if (state == LED_STATE_ON) {
			value |= BIT(i);
		}
	}

	*mask = value;
	return 0;
}
