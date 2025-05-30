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

#pragma once

/*
 * Accelerometer Interrupt Counter
 *
 * This is an experimental watch face for counting the number of interrupts that
 * the Sensor Watch Motion acceleromoeter board fires. I expect it will be removed
 * once we integrate accelerometer functionality more deeply into Movement.
 */

#include "movement.h"
#include "watch.h"

typedef struct {
    uint8_t new_threshold;
    uint8_t threshold;
    lis2dw_data_rate_t old_rate;
    bool is_setting;
} accel_interrupt_count_state_t;

void accelerometer_status_face_setup(uint8_t watch_face_index, void ** context_ptr);
void accelerometer_status_face_activate(void *context);
bool accelerometer_status_face_loop(movement_event_t event, void *context);
void accelerometer_status_face_resign(void *context);

#define accelerometer_status_face ((const watch_face_t){ \
    accelerometer_status_face_setup, \
    accelerometer_status_face_activate, \
    accelerometer_status_face_loop, \
    accelerometer_status_face_resign, \
    NULL, \
})
