/**
******************************************************************************
* @file				: app.h
* @brief			: Header for app.c file.
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
#ifndef APP_H
#define APP_H

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
 * @brief Application event types
 */
typedef enum __APP_EVENT_TYPE {
	APP_EVENT_TYPE_LED_TOGGLE,		// Event type for LED toggle events
} APP_EVENT_TYPE;

/**
 * @brief Application event structure
 */
typedef struct __APP_EVENT {
	APP_EVENT_TYPE type;				// Type of the event
	uint32_t led_mask;					// For LED toggle events, the mask of LEDs that changed
} APP_EVENT;

/* Exported functions prototypes ---------------------------------------------*/
int app_event_get(APP_EVENT *event);
int app_handle_event(const APP_EVENT *event);
int app_event_submit(APP_EVENT_TYPE type, uint32_t led_mask);

#ifdef __cplusplus
}
#endif

#endif  // APP_H
