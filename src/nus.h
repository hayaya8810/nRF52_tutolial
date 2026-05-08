/**
******************************************************************************
* @file				: nus.h
* @brief			: Header for Nordic UART Service notifications.
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef NUS_H
#define NUS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "led.h"

/* Exported functions prototypes ---------------------------------------------*/
int nus_init(void);
bool nus_is_connected(void);
int nus_notify_led_state(void);
int nus_notify_led_event(LED_ID id);

#ifdef __cplusplus
}
#endif

#endif  // NUS_H
