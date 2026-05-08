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

/* Private define ------------------------------------------------------------*/
#define SLEEP_TIME_MS			1000		// 1000ms = 1s

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void button_handler(BUTTON_ID id, BUTTON_EVENT event);

/* External variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
* @brief Main function
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

	for( size_t i = 0; i < BUTTON_ID_COUNT; i++) {
		ret = button_register_callback((BUTTON_ID)i, button_handler);
		if (ret < 0) {
			printf("Failed to register button callback for button %zu\n", i);
			return -1;
		}
	}

	while (1) {
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}

/* Private user code ---------------------------------------------------------*/
/**
 * @brief Button event handler to toggle corresponding LED on button press
 * @param[in]	id			Button identifier
 * @param[in]	event		Button event type (pressed or released)
 */
static void button_handler(BUTTON_ID id, BUTTON_EVENT event)
{
	if (event == BUTTON_EVENT_PRESSED) {
		switch (id){
		case BUTTON_ID_0:
			led_blink_toggle(id, 500, 500);	// Toggle blinking with 500ms on and 500ms off
			break;
		case BUTTON_ID_1:
			led_blink_toggle(id, 250, 250);	// Toggle blinking with 500ms on and 500ms off
			break;
		case BUTTON_ID_2:
			led_blink_toggle(id, 125, 125);	// Toggle blinking with 125ms on and 125ms off
			break;
		case BUTTON_ID_3:
			led_toggle(id);
			break;
		default:
			break;
		}
	}
}