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
typedef void (*nus_led_control_callback_t)(uint32_t led_mask);
int nus_init(void);
bool nus_is_connected(void);
int nus_notify_led_state(void);
int nus_notify_led_event(LED_ID id);
int nus_register_led_control_callback(nus_led_control_callback_t callback);
int nus_unregister_led_control_callback(void);

#ifdef __cplusplus
}
#endif

#endif  // NUS_H
