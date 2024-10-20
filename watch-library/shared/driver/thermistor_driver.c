/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "thermistor_driver.h"
#include "sam.h"
#include "watch.h"
#include "watch_utility.h"

#ifdef HAS_TEMPERATURE_SENSOR

void thermistor_driver_enable(void) {
    // Enable the ADC peripheral, which we'll use to read the thermistor value.
    watch_enable_adc();
    // Enable analog circuitry on the sense pin, which is tied to the thermistor resistor divider.
    HAL_GPIO_TEMPSENSE_in();
    HAL_GPIO_TEMPSENSE_pmuxen(HAL_GPIO_PMUX_ADC);
    // Enable digital output on the enable pin, which is the power to the thermistor circuit.
    HAL_GPIO_TS_ENABLE_out();
    // and make sure it's off.
    HAL_GPIO_TS_ENABLE_write(!THERMISTOR_ENABLE_VALUE);
}

void thermistor_driver_disable(void) {
    // Disable the ADC peripheral.
    watch_disable_adc();
    // Disable analog circuitry on the sense pin to save power.
    HAL_GPIO_TEMPSENSE_pmuxdis();
    HAL_GPIO_TEMPSENSE_off();
    // Disable the enable pin's output circuitry.
    HAL_GPIO_TS_ENABLE_off();
}

float thermistor_driver_get_temperature(void) {
    // set the enable pin to the level that powers the thermistor circuit.
    HAL_GPIO_TS_ENABLE_write(THERMISTOR_ENABLE_VALUE);
    // get the sense pin level
    uint16_t value = watch_get_analog_pin_level(HAL_GPIO_TEMPSENSE_pin());
    // and then set the enable pin to the opposite value to power down the thermistor circuit.
    HAL_GPIO_TS_ENABLE_write(!THERMISTOR_ENABLE_VALUE);

    return watch_utility_thermistor_temperature(value, THERMISTOR_HIGH_SIDE, THERMISTOR_B_COEFFICIENT, THERMISTOR_NOMINAL_TEMPERATURE, THERMISTOR_NOMINAL_RESISTANCE, THERMISTOR_SERIES_RESISTANCE);
}

#endif // HAS_TEMPERATURE_SENSOR