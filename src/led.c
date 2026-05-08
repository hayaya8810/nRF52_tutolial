/**
******************************************************************************
* @file				: led.c
* @brief			: LED driver.
******************************************************************************
* @attention
*
* Copyright (c) 2026 hayaya inc.
* All rights reserved.
*
* This software is licensed under terms that can be found in the LICENSE file
* in the root directory of this software component.
* If no LICENSE file comes with this software, it is provided AS-IS.
*
******************************************************************************
*/
/* Includes ------------------------------------------------------------------*/
#include <zephyr/drivers/gpio.h>

/* Private includes ----------------------------------------------------------*/
#include "led.h"

/* Private define ------------------------------------------------------------*/
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

/* Private typedef -----------------------------------------------------------*/
/**
 * @brief LED context structure for blinking functionality
 */
typedef struct __LED_CONTEXT {
	struct k_work_delayable blink_work;		// Delayed work for blinking
	uint32_t on_time_ms;					// On time in milliseconds
	uint32_t off_time_ms;					// Off time in milliseconds
	bool is_on;								// Current state of the LED
	bool is_blinking;						// Flag indicating if blinking is active
} LED_CONTEXT;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/**
 * @brief GPIO specifications for each LED, obtained from device tree
 */
static const struct gpio_dt_spec leds[] = {
	GPIO_DT_SPEC_GET(LED0_NODE, gpios),
	GPIO_DT_SPEC_GET(LED1_NODE, gpios),
	GPIO_DT_SPEC_GET(LED2_NODE, gpios),
	GPIO_DT_SPEC_GET(LED3_NODE, gpios),
};

/**
 * @brief LED contexts for each LED, used for blinking functionality
 */
static LED_CONTEXT led_contexts[LED_ID_COUNT];

/* Private function prototypes -----------------------------------------------*/
static void led_blink_work_handler(struct k_work *work);

/* External variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief		Initialize the LEDs
 * @return		0			success
 * 				negative	error code on failure
 */
int led_init(void)
{
	for (size_t i = 0; i < LED_ID_COUNT; i++) {
		if (!device_is_ready(leds[i].port)) {
			return -1;
		}
		int ret = gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return -1;
		}
		k_work_init_delayable(&led_contexts[i].blink_work, led_blink_work_handler);
		led_contexts[i].is_on = false;
		led_contexts[i].is_blinking = false;
	}
	return 0;
}

/**
 * @brief		Set the state of an LED
 * @param[in]	id			LED identifier
 * @param[in]	state		Desired LED state
 * @return		0			success
 * 				negative	error code on failure
 */
int led_set(LED_ID id, LED_STATE state)
{
	if (id >= LED_ID_COUNT) {
		return -1;			// Invalid LED ID
	}
	return gpio_pin_set_dt(&leds[id], state == LED_STATE_ON ? 1 : 0);
}

/**
 * @brief		Get the state of an LED
 * @param[in]	id			LED identifier
 * @param[out]	state		Current LED state
 * @return		0			success
 * 				negative	error code on failure
 */
int led_get(LED_ID id, LED_STATE *state)
{
	if (id >= LED_ID_COUNT || state == NULL) {
		return -1;			// Invalid argument
	}

	gpio_flags_t flags;
	int ret = gpio_pin_get_config_dt(&leds[id], &flags);
	if (ret < 0 || (flags & GPIO_OUTPUT) == 0) {
		return -1;			// Error getting LED output state
	}

	int physical_high = (flags & GPIO_OUTPUT_INIT_HIGH) != 0;
	int active_low = (leds[id].dt_flags & GPIO_ACTIVE_LOW) != 0;

	*state = (physical_high ^ active_low) ? LED_STATE_ON : LED_STATE_OFF;
	return 0;
}

/**
 * @brief		Toggle the state of an LED
 * @param[in]	id			LED identifier
 * @return		0			success
 * 				negative	error code on failure
 */
int led_toggle(LED_ID id)
{
	if (id >= LED_ID_COUNT) {
		return -1;			// Invalid LED ID
	}
	return gpio_pin_toggle_dt(&leds[id]);
}

/**
 * @brief		Set the state of multiple LEDs using a bitmask
 * @param[in]	on_mask		Bitmask indicating which LEDs to turn on (1 = on, 0 = off)
 * @return		0			success
 * 				negative	error code on failure
 */
int led_set_mask( uint32_t on_mask)
{
	for (size_t i = 0; i < LED_ID_COUNT; i++) {
		LED_STATE state = (on_mask & (1 << i)) ? LED_STATE_ON : LED_STATE_OFF;
		int ret = led_set(i, state);
		if (ret < 0) {
			return -1;		// Error setting LED state
		}
	}
	return 0;
}

/**
 * @brief		Toggle the state of multiple LEDs using a bitmask
 * @param[in]	toggle_mask		Bitmask indicating which LEDs to toggle (1 = toggle, 0 = no change)
 * @return		0			success
 * 				negative	error code on failure
 */
int led_toggle_mask( uint32_t toggle_mask)
{
	for (size_t i = 0; i < LED_ID_COUNT; i++) {
		if (toggle_mask & (1 << i)) {
			int ret = led_toggle(i);
			if (ret < 0) {
				return -1;	// Error toggling LED state
			}
		}
	}
	return 0;
}

/**
 * @brief		Start blinking an LED with specified on/off times
 * @param[in]	id				LED identifier
 * @param[in]	on_time_ms		On time in milliseconds
 * @param[in]	off_time_ms		Off time in milliseconds
 * @return		0				success
 * 				negative		error code on failure
 */
int led_blink_start( LED_ID id, uint32_t on_time_ms, uint32_t off_time_ms)
{
	if (id >= LED_ID_COUNT) {
		return -1;			// Invalid LED ID
	}
	led_contexts[id].on_time_ms = on_time_ms;
	led_contexts[id].off_time_ms = off_time_ms;
	led_contexts[id].is_blinking = true;
	k_work_reschedule(&led_contexts[id].blink_work, K_NO_WAIT);
	return 0;
}

/**
 * @brief		Stop blinking an LED
 * @param[in]	id			LED identifier
 * @return		0			success
 * 				negative	error code on failure
 */
int led_blink_stop( LED_ID id)
{
	if (id >= LED_ID_COUNT) {
		return -1;			// Invalid LED ID
	}

	// Stop blinking and reset LED state to off
	led_contexts[id].is_blinking = false;
	k_work_cancel_delayable(&led_contexts[id].blink_work);
	led_contexts[id].is_on = false;
	return led_set(id, LED_STATE_OFF);
}

/**
 * @brief		Toggle blinking state of an LED (start if stopped, stop if started)
 * @param[in]	id				LED identifier
 * @param[in]	on_time_ms		On time in milliseconds (used if starting blinking)
 * @param[in]	off_time_ms		Off time in milliseconds (used if starting blinking)
 * @return		0				success
 * 				negative		error code on failure
 */
int led_blink_toggle( LED_ID id, uint32_t on_time_ms, uint32_t off_time_ms)
{
	if (id >= LED_ID_COUNT) {
		return -1;			// Invalid LED ID
	}
	if (led_contexts[id].is_blinking) {
		return led_blink_stop(id);
	} else {
		return led_blink_start(id, on_time_ms, off_time_ms);
	}
}


/* Private user code ---------------------------------------------------------*/
/**
 * @brief		Work handler for LED blinking functionality
 * @param[in]	work		Work item for blinking
 */
static void led_blink_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	for( size_t i = 0; i < LED_ID_COUNT; i++) {
		if (&led_contexts[i].blink_work == dwork) {
			if (led_contexts[i].is_blinking) {
				// Toggle LED state
				led_contexts[i].is_on = !led_contexts[i].is_on;
				led_set(i, led_contexts[i].is_on ? LED_STATE_ON : LED_STATE_OFF);

				// Reschedule work for the next toggle
				uint32_t delay_ms = led_contexts[i].is_on ? led_contexts[i].on_time_ms : led_contexts[i].off_time_ms;
				k_work_reschedule(&led_contexts[i].blink_work, K_MSEC(delay_ms));
			}
		}
	}
}