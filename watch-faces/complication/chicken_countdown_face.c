/*
 * MIT License
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

#include <stdlib.h>
#include <string.h>
#include "chicken_countdown_face.h"
#include "watch.h"
#include "watch_utility.h"

#define DEFAULT_SECONDS 9
#define START_SECONDS -3  // At the start of the count, we want some time between pushing the button and setting up.
#define MAX_SECONDS 300
#define MIN_SECONDS 2

static int16_t _actual_seconds;



static void draw(int16_t number) {
    char buf[8];
    sprintf(buf, "%4d  ", number);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void chicken_countdown_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(chicken_countdown_state_t));
        chicken_countdown_state_t *state = (chicken_countdown_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(chicken_countdown_state_t));
        state->target_seconds = DEFAULT_SECONDS;
        state->running = false;
        state->chime = false;
    }
}

void chicken_countdown_face_activate(void *context) {
    chicken_countdown_state_t *state = (chicken_countdown_state_t *)context;
    state->running = false;
    _actual_seconds = state->target_seconds;
    movement_request_tick_frequency(1);
}

bool chicken_countdown_face_loop(movement_event_t event, void *context) {
    chicken_countdown_state_t *state = (chicken_countdown_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "CHIKN", "CC");
            draw(_actual_seconds);
            if (state->chime) watch_set_indicator(WATCH_INDICATOR_BELL);
            gshock_display_current_time_top_right();
            // fall-through
        case EVENT_TICK:
            if (!state->running) break;
            if (_actual_seconds > 0) {
                _actual_seconds -= 1;
                if (_actual_seconds == 0 && state->chime) {
                    movement_play_signal();
                }
            } else {
                _actual_seconds += 1;
            }
            if (_actual_seconds == 0) {
                _actual_seconds = state->target_seconds;
            }
            draw(_actual_seconds);
            break;
#ifdef FORCE_GSHOCK_LCD_TYPE
        case EVENT_MINUTE:
            gshock_display_current_time_top_right();
            break;
#endif
        case EVENT_ALARM_BUTTON_UP:
            state->running = !state->running;
            if (!state->running) {
                _actual_seconds = state->target_seconds;
                watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
            } else {
                _actual_seconds = START_SECONDS;
                watch_set_indicator(WATCH_INDICATOR_SIGNAL);
            }
            draw(_actual_seconds);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            // increment
            state->target_seconds += 1;
            if (state->target_seconds > MAX_SECONDS) {
                state->target_seconds = MIN_SECONDS;
                if (_actual_seconds > MIN_SECONDS) _actual_seconds = MIN_SECONDS;
            } else {
                _actual_seconds += 1;
            }
            draw(state->target_seconds);
            break;
#ifdef FORCE_GSHOCK_LCD_TYPE
        case EVENT_START_BUTTON_UP:
#endif
        case EVENT_LIGHT_LONG_PRESS:
            // decrement
            state->target_seconds -= 1;
            if (state->target_seconds < MIN_SECONDS) {
                state->target_seconds = MAX_SECONDS;
                _actual_seconds = MAX_SECONDS;
            } else {
                _actual_seconds -= 1;
                if (_actual_seconds > MAX_SECONDS) {
                    _actual_seconds = MAX_SECONDS;
                }
            }
            draw(state->target_seconds);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->chime = !state->chime;
            if (state->chime) {
                watch_set_indicator(WATCH_INDICATOR_BELL);
            } else {
                watch_clear_indicator(WATCH_INDICATOR_BELL);
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // intentionally squelch the light default event; we only show the light when ccd is running or reset
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }

    return true;
}

void chicken_countdown_face_resign(void *context) {
    (void) context;
}
