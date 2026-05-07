/**
******************************************************************************
* @file				: button.c
* @brief			: Button driver.
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
#include "button.h"

/* Private define ------------------------------------------------------------*/
#define BUTTON_DEBOUNE_MS		50			// Debounce time in milliseconds
#define BUTTON0_NODE DT_ALIAS(sw0)
#define BUTTON1_NODE DT_ALIAS(sw1)
#define BUTTON2_NODE DT_ALIAS(sw2)
#define BUTTON3_NODE DT_ALIAS(sw3)

/* Private typedef -----------------------------------------------------------*/
/**
 * @brief Button context structure for debouncing and event handling
 */
typedef struct __BUTTON_CONTEXT {
	struct gpio_callback gpio_cb;			// GPIO callback structure for button interrupts
	struct k_work_delayable debounce_work;	// Delayed work for debouncing
	BUTTON_STATE stable_state;				// Stable state of the button after debouncing
	button_callback_t callback;				// User callback function for button events
} BUTTON_CONTEXT;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/**
 * @brief GPIO specifications for each button, obtained from device tree
 */
static const struct gpio_dt_spec buttons[] = {
	GPIO_DT_SPEC_GET(BUTTON0_NODE, gpios),
	GPIO_DT_SPEC_GET(BUTTON1_NODE, gpios),
	GPIO_DT_SPEC_GET(BUTTON2_NODE, gpios),
	GPIO_DT_SPEC_GET(BUTTON3_NODE, gpios),
};

/**
 * @brief Button contexts for each button, used for debouncing and event handling
 */
static BUTTON_CONTEXT button_contexts[BUTTON_ID_COUNT];

/* Private function prototypes -----------------------------------------------*/
static void button_gpio_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
static void button_debounce_work_handler(struct k_work *work);

/* External variables --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/**
 * @brief		Initialize the buttons
 * @return		0			success
 * 				negative	error code on failure
 */
int button_init(void)
{
	for (size_t i = 0; i < BUTTON_ID_COUNT; i++) {
		// Configure button GPIO as input with pull-up and set up interrupt for both edges
		if (!device_is_ready(buttons[i].port)) {
			return -1;
		}

		// Configure GPIO pin as input with pull-up resistor
		int ret = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT | GPIO_PULL_UP);
		if (ret < 0) {
			return -1;
		}

		// Initialize GPIO callback for button interrupts
		gpio_init_callback(&button_contexts[i].gpio_cb, button_gpio_isr, BIT(buttons[i].pin));
		ret = gpio_add_callback(buttons[i].port, &button_contexts[i].gpio_cb);
		if (ret < 0) {
			return -1;
		}

		// Configure GPIO interrupt for both rising and falling edges
		ret = gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_BOTH);
		if (ret < 0) {
			return -1;
		}

		// Initialize debounce work for the button context
		button_contexts[i].stable_state = BUTTON_STATE_RELEASED;
		button_contexts[i].callback = NULL;
		k_work_init_delayable(&button_contexts[i].debounce_work, button_debounce_work_handler);
	}
	return 0;
}

/**
 * @brief		Get the current state of a button
 * @param[in]	id			Button identifier
 * @param[out]	state		Current button state
 * @return		0			success
 * 				negative	error code on failure
 */
int button_get(BUTTON_ID id, BUTTON_STATE *state)
{
	if (id >= BUTTON_ID_COUNT || state == NULL) {
		return -1;			// Invalid argument
	}

	int val = gpio_pin_get_dt(&buttons[id]);
	if (val < 0) {
		return -1;			// Error reading GPIO pin
	}
	*state = (val == 0) ? BUTTON_STATE_PRESSED : BUTTON_STATE_RELEASED;
	return 0;
}

/**
 * @brief		Get the state of all buttons as a bitmask
 * @param[out]	pressed_mask	Bitmask indicating which buttons are pressed (1 = pressed, 0 = released)
 * @return		0			success
 * 				negative	error code on failure
 */
int button_get_mask(uint32_t *pressed_mask)
{
	if (pressed_mask == NULL) {
		return -1;			// Invalid argument
	}

	uint32_t mask = 0;
	for (size_t i = 0; i < BUTTON_ID_COUNT; i++) {
		int val = gpio_pin_get_dt(&buttons[i]);
		if (val < 0) {
			return -1;		// Error reading GPIO pin
		}
		if (val == 0) {		// Button is pressed
			mask |= (1 << i);
		}
	}
	*pressed_mask = mask;
	return 0;
}

/**
 * @brief		Register a callback function for button events
 * @param[in]	id			Button identifier
 * @param[in]	callback	Callback function to be called on button events
 * @return		0			success
 * 				negative	error code on failure
 */
int button_register_callback(BUTTON_ID id, button_callback_t callback)
{
	if (id >= BUTTON_ID_COUNT) {
		return -1;			// Invalid button ID
	}
	button_contexts[id].callback = callback;
	return 0;
}

/**
 * @brief		Unregister the callback function for a button
 * @param[in]	id			Button identifier
 * @return		0			success
 * 				negative	error code on failure
 */
int button_unregister_callback(BUTTON_ID id)
{
	if (id >= BUTTON_ID_COUNT) {
		return -1;			// Invalid button ID
	}
	button_contexts[id].callback = NULL;
	return 0;
}

/* Private user code ---------------------------------------------------------*/
/**
 * @brief		GPIO interrupt service routine for button presses
 * @param[in]	dev			Device that triggered the interrupt
 * @param[in]	cb			GPIO callback structure
 * @param[in]	pins		Bitmask of pins that triggered the interrupt
 */
static void button_gpio_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	// Identify which button triggered the interrupt
	for (size_t i = 0; i < BUTTON_ID_COUNT; i++) {
		if (pins & BIT(buttons[i].pin)) {
			// Schedule debounce work for the corresponding button context
				k_work_reschedule(&button_contexts[i].debounce_work, K_MSEC(BUTTON_DEBOUNE_MS));
			break;
		}
	}
}

/**
 * @brief		Debounce work handler to determine stable button state and trigger events
 * @param[in]	work		Work item for debouncing
 */
static void button_debounce_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	// Identify which button context is associated with the debounce work
	for (size_t i = 0; i < BUTTON_ID_COUNT; i++) {
		if (&button_contexts[i].debounce_work == dwork) {
			// Read the current state of the button GPIO pin
			int val = gpio_pin_get_dt(&buttons[i]);
			BUTTON_STATE current_state = (val == 0) ? BUTTON_STATE_PRESSED : BUTTON_STATE_RELEASED;

			// Check if the state has changed from the stable state
			if (current_state != button_contexts[i].stable_state) {
				// Update the stable state
				button_contexts[i].stable_state = current_state;

				// If a callback is registered, call it with the button ID and event type
				if (button_contexts[i].callback) {
					BUTTON_EVENT event = (current_state == BUTTON_STATE_PRESSED) ? BUTTON_EVENT_PRESSED : BUTTON_EVENT_RELEASED;
					button_contexts[i].callback(i, event);
				}
			}
			break;
		}
	}
}
