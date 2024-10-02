/**
 * @file GPIO.hpp
 * @brief A simple GPIO control interface for STM32.
 *
 * This file provides a class interface for controlling GPIO pins on STM32 devices
 * using HAL libraries. It allows setting and reading the state of GPIO pins.
 *
 * @details The GPIO class abstracts basic GPIO operations such as digital write and read.
 *
 * @date September 21, 2024
 * @author Sásdi András
 */

#ifndef MODULES_GPIO_GPIO_HPP_
#define MODULES_GPIO_GPIO_HPP_

#include "stm32f4xx_hal.h"

/**
 * @class GPIO
 * @brief A class for controlling GPIO pins on STM32 using HAL libraries.
 *
 * The GPIO class provides simple methods for reading from and writing to a specific GPIO pin.
 */
class GPIO
{
private:
    GPIO_TypeDef* GPIOx;  ///< GPIO port (e.g., GPIOA, GPIOB).
    uint16_t GPIO_Pin;    ///< GPIO pin number (e.g., GPIO_PIN_0, GPIO_PIN_1).

public:
    /**
     * @brief Constructor to initialize the GPIO pin.
     * @param GPIOx_p GPIO port (e.g., GPIOA, GPIOB).
     * @param GPIO_Pin_p GPIO pin number (e.g., GPIO_PIN_0, GPIO_PIN_1).
     *
     * @note It makes it easier if user labels are used when configuring the pins
     *
     * example
     * \code
     	GPIO TEMP_SENS_CS(TEMP_SENS_CS_GPIO_Port, TEMP_SENS_CS_Pin);
		GPIO TEMP_RDY(TEMP_RDY_GPIO_Port, TEMP_RDY_Pin);
     * \endcode
     */
    GPIO(GPIO_TypeDef* GPIOx_p, uint16_t GPIO_Pin_p);

    /**
     * @brief Write a digital value (high or low) to the GPIO pin.
     * @param state_p The desired state to set the GPIO pin (GPIO_PIN_SET or GPIO_PIN_RESET).
     */
    void digitalWrite(GPIO_PinState state_p);

    /**
     * @brief Read the current state of the GPIO pin.
     * @return The current state of the GPIO pin (GPIO_PIN_SET or GPIO_PIN_RESET).
     */
    GPIO_PinState digitalRead();
};

#endif /* MODULES_GPIO_GPIO_HPP_ */
