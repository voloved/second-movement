/*
 * MIT License
 *
 * Copyright (c) 2024 Joey Castillo
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

#pragma once

#include "movement.h"

#ifdef HAS_IR_SENSOR

/*
 * LIGHT SENSOR PLAYGROUND
 *
 * Temporary watch face for playing with the light sensor.
 * WARNING: This watch face may not play nicely with watch faces that use the ADC in the background,
 * such as the temperature logger. More improvement and testing needs to be done before this is can
 * be considered a production-ready watch face.
 *
 */

void light_sensor_face_setup(uint8_t watch_face_index, void ** context_ptr);
void light_sensor_face_activate(void *context);
bool light_sensor_face_loop(movement_event_t event, void *context);
void light_sensor_face_resign(void *context);

#define light_sensor_face ((const watch_face_t){ \
    light_sensor_face_setup, \
    light_sensor_face_activate, \
    light_sensor_face_loop, \
    light_sensor_face_resign, \
    NULL, \
})

#endif // HAS_IR_SENSOR
