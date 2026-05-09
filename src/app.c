/**
******************************************************************************
* @file				: app.c
* @brief			: Application functions.
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
#include <stdio.h>
#include <ctype.h>
#include <zephyr/sys/util.h>

/* Private includes ----------------------------------------------------------*/
#include "app.h"
#include "led.h"
#include "nus.h"

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
APP_EVENT m_event_queue[10];				// Simple fixed-size event queue
size_t m_event_queue_head = 0;				// Index of the head of the queue
size_t m_event_queue_tail = 0;				// Index of the tail of the queue
size_t m_event_queue_count = 0;				// Number of events currently in the queue

/* Private function prototypes -----------------------------------------------*/
static int app_toggle_leds(uint32_t led_mask);

/* External variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief		Get the next application event from the event queue
 * @param[out]	event		Pointer to store the retrieved event
 * @return		0			success
 * 				negative	error code on failure (e.g., no events in the queue)
 */
int app_event_get(APP_EVENT *event)
{
	if (event == NULL) {
		return -1;				// Invalid argument
	}
	if (m_event_queue_count == 0) {
		return -1;				// No events in the queue
	}

	*event = m_event_queue[m_event_queue_head];
	m_event_queue_head = (m_event_queue_head + 1) % ARRAY_SIZE(m_event_queue);
	m_event_queue_count--;
	return 0;
}

/**
 * @brief		Handle an application event
 * @param[in]	event		Pointer to the event to handle
 * @return		0			success
 * 				negative	error code on failure (e.g., unknown event type)
 */
int app_handle_event(const APP_EVENT *event)
{
	if (event == NULL) {
		return -1;				// Invalid argument
	}

	switch (event->type) {
	case APP_EVENT_TYPE_LED_TOGGLE:
		app_toggle_leds(event->led_mask);
		printf("Handling LED toggle event, led_mask=0x%08X\n", event->led_mask);
		break;
	default:
		return -1;				// Unknown event type
	}

	return 0;
}

int app_event_submit(APP_EVENT_TYPE type, uint32_t led_mask)
{
	if (m_event_queue_count >= ARRAY_SIZE(m_event_queue)) {
		return -1;				// Event queue is full
	}

	m_event_queue[m_event_queue_tail].type = type;
	m_event_queue[m_event_queue_tail].led_mask = led_mask;
	m_event_queue_tail = (m_event_queue_tail + 1) % ARRAY_SIZE(m_event_queue);
	m_event_queue_count++;
	return 0;
}

/* Private user code ---------------------------------------------------------*/
/**
 * @brief		Toggle LEDs based on the provided bitmask
 * @param[in]	led_mask	Bitmask indicating which LEDs to toggle (1 = toggle, 0 = no change)
 * @return		0			success
 * 				negative	error code on failure
 */
static int app_toggle_leds(uint32_t led_mask)
{
	int ret = -1;

	if( BIT(LED_ID_0) & led_mask) {
		ret = led_blink_toggle(LED_ID_0, 500, 500);
		nus_notify_led_event(LED_ID_0);
	}
	if (BIT(LED_ID_1) & led_mask) {
		ret = led_blink_toggle(LED_ID_1, 250, 250);
		nus_notify_led_event(LED_ID_1);
	}
	if (BIT(LED_ID_2) & led_mask) {
		ret = led_blink_toggle(LED_ID_2, 125, 125);
		nus_notify_led_event(LED_ID_2);
	}
	if (BIT(LED_ID_3) & led_mask) {
		ret = led_toggle(LED_ID_3);
		nus_notify_led_event(LED_ID_3);
	}
	return ret;
}
