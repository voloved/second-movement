/*
 * MIT License
 *
 * Copyright (c) 2024 <#author_name#>
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

#ifndef PARTY_FACE_H_
#define PARTY_FACE_H_

#include "movement.h"

/*
 * A DESCRIPTION OF YOUR WATCH FACE
 *
 * and a description of how use it
 *
 */

typedef struct {
    bool blink;
    bool fast;
    uint8_t led : 2; // 0 = None; 1 = Green only; 2 = All
    uint8_t text : 2;
    uint8_t color : 2;
    uint8_t party_text : 2;
    uint8_t curr_day : 5;
    bool unused;
    int8_t prev_text;
} party_state_t;

void party_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void party_face_activate(movement_settings_t *settings, void *context);
bool party_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void party_face_resign(movement_settings_t *settings, void *context);

#define party_face ((const watch_face_t){ \
    party_face_setup, \
    party_face_activate, \
    party_face_loop, \
    party_face_resign, \
    NULL, \
})
#define MAX_TEXT  2
// 0 - Party
// 1 - Prom

#endif // PARTY_FACE_H_

