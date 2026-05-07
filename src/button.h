/**
******************************************************************************
* @file				: button.h
* @brief			: Header for button.c file.
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
#ifndef BUTTON_H
#define BUTTON_H


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
 * @brief Button identifiers
 */
typedef enum __BUTTON_ID {
	BUTTON_ID_0,		// on-board button-1
	BUTTON_ID_1,		// on-board button-2
	BUTTON_ID_2,		// on-board button-3
	BUTTON_ID_3,		// on-board button-4
	BUTTON_ID_COUNT		// number of on-board buttons
} BUTTON_ID;

/**
 * @brief Button states
 */
typedef enum __BUTTON_STATE {
	BUTTON_STATE_RELEASED,		// Button is released
	BUTTON_STATE_PRESSED,		// Button is pressed
} BUTTON_STATE;

/**
 * @brief Button events
 */
typedef enum __BUTTON_EVENT {
	BUTTON_EVENT_NONE,			// No event
	BUTTON_EVENT_PRESSED,		// Button was pressed
	BUTTON_EVENT_RELEASED,		// Button was released
} BUTTON_EVENT;

/* Exported functions prototypes ---------------------------------------------*/
typedef void (*button_callback_t)(BUTTON_ID id, BUTTON_EVENT event);
int button_init(void);
int button_get(BUTTON_ID id, BUTTON_STATE *state);
int button_get_mask(uint32_t *pressed_mask);
int button_register_callback(BUTTON_ID id, button_callback_t callback);
int button_unregister_callback(BUTTON_ID id);

#ifdef __cplusplus
}
#endif

#endif  // BUTTON_H
