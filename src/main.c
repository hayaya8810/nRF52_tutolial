/**
******************************************************************************
* @file				: main.c
* @brief			: main program for LED blink
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
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Private includes ----------------------------------------------------------*/
#include "button.h"
#include "led.h"
#include "nus.h"
#include "app.h"

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void button_handler(BUTTON_ID id, BUTTON_EVENT event);
static void nus_led_control_callback(uint32_t led_mask);

/* External variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief		Main function
*/
int main(void)
{
	int ret;
	ret = button_init();
	if (ret < 0) {
		printf("Failed to initialize buttons\n");
		return -1;
	}
	ret = led_init();
	if (ret < 0) {
		printf("Failed to initialize LEDs\n");
		return -1;
	}
	ret = nus_init();
	if (ret < 0) {
		printf("Failed to initialize NUS\n");
		return -1;
	}

	for( size_t i = 0; i < BUTTON_ID_COUNT; i++) {
		ret = button_register_callback((BUTTON_ID)i, button_handler);
		if (ret < 0) {
			printf("Failed to register button callback for button %zu\n", i);
			return -1;
		}
	}

	ret = nus_register_led_control_callback(nus_led_control_callback);
	if (ret < 0) {
		printf("Failed to register NUS LED control callback\n");
		return -1;
	}

	while (1) {
		APP_EVENT event;
		if (app_event_get(&event) == 0) {
			app_handle_event(&event);
		}
	}
	return 0;
}

/* Private user code ---------------------------------------------------------*/
/**
 * @brief		Button event handler to toggle corresponding LED on button press
 * @param[in]	id			Button identifier
 * @param[in]	event		Button event type (pressed or released)
 */
static void button_handler(BUTTON_ID id, BUTTON_EVENT event)
{
	uint32_t led_mask = 0x00;

	if (event == BUTTON_EVENT_PRESSED) {
		switch (id){
		case BUTTON_ID_0:
			led_mask = 0x01;	// Mask for LED_ID_0
			nus_notify_led_event((LED_ID)id);
			break;
		case BUTTON_ID_1:
			led_mask = 0x02;	// Mask for LED_ID_1
			nus_notify_led_event((LED_ID)id);
			break;
		case BUTTON_ID_2:
			led_mask = 0x04;	// Mask for LED_ID_2
			nus_notify_led_event((LED_ID)id);
			break;
		case BUTTON_ID_3:
			led_mask = 0x08;	// Mask for LED_ID_3
			nus_notify_led_event((LED_ID)id);
			break;
		default:
			break;
		}
		app_event_submit(APP_EVENT_TYPE_LED_TOGGLE, led_mask);
	}
}

/**
 * @brief		Callback function for controlling LEDs from BLE commands
 * @param[in]	led_mask	Bitmask indicating which LEDs to toggle (1 = toggle, 0 = no change)
 */
static void nus_led_control_callback(uint32_t led_mask)
{
	app_event_submit(APP_EVENT_TYPE_LED_TOGGLE, led_mask);
}
