/**
******************************************************************************
* @file				: led.h
* @brief			: Header for led.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LED_H
#define LED_H


#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/**
 * @brief LED identifiers
 */
typedef enum __LED_ID {
	LED_ID_0,			// on-board LED-1
	LED_ID_1,			// on-board LED-2
	LED_ID_2,			// on-board LED-3
	LED_ID_3,			// on-board LED-4
	LED_ID_COUNT,		// number of on-board LEDs
} LED_ID;

/**
 * @brief LED states
 */
typedef enum __LED_STATE {
	LED_STATE_OFF,  	// LED is off
	LED_STATE_ON,		// LED is on
} LED_STATE;

/* Exported functions prototypes ---------------------------------------------*/
int led_init(void);
int led_set(LED_ID id, LED_STATE state);
int led_get(LED_ID id, LED_STATE *state);
int led_toggle(LED_ID id);
int led_set_mask( uint32_t on_mask);
int led_toggle_mask( uint32_t toggle_mask);
int led_blink_start( LED_ID id, uint32_t on_time_ms, uint32_t off_time_ms);
int led_blink_stop( LED_ID id);
int led_blink_toggle( LED_ID id, uint32_t on_time_ms, uint32_t off_time_ms);

#ifdef __cplusplus
}
#endif

#endif  // LED_H
