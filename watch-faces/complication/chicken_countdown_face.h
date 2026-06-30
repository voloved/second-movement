/*
 * MIT License
 *
 * Copyright (c) 2026 David Volovskiy
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

#ifndef CHICKEN_COUNTDOWN_FACE_H_
#define CHICKEN_COUNTDOWN_FACE_H_

/*
 * CHICKEN COUNTDOWN TIMER face
 *
 * This face cycles down from a starting second to 1 and back around.
 *   ADJUST - Start the countdown loop
 *   ADJUST LONG PRESS - Toggle chime when looping back around
 *   LIGHT - Increment starting second
 *   LIGHT LONG PRESS (Or START on G-Shock) - Decrement starting second
 */

#include "movement.h"

typedef struct {
    int16_t target_seconds;
    bool running;
    bool chime;
} chicken_countdown_state_t;


void chicken_countdown_face_setup(uint8_t watch_face_index, void ** context_ptr);
void chicken_countdown_face_activate(void *context);
bool chicken_countdown_face_loop(movement_event_t event, void *context);
void chicken_countdown_face_resign(void *context);

#define chicken_countdown_face ((const watch_face_t){ \
    chicken_countdown_face_setup, \
    chicken_countdown_face_activate, \
    chicken_countdown_face_loop, \
    chicken_countdown_face_resign, \
    NULL, \
})

#endif // CHICKEN_COUNTDOWN_FACE_H_
