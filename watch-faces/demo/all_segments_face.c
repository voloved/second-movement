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

#include <stdlib.h>
#include <string.h>
#include "all_segments_face.h"
#include "watch.h"
#include "delay.h"

#define TICK_FREQ 4
#define WAIT_SEC 2

void all_segments_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(all_segments_state_t));
        memset(*context_ptr, 0, sizeof(all_segments_state_t));
    }
}

void all_segments_face_activate(void *context) {
    all_segments_state_t *state = (all_segments_state_t *)context;
    watch_lcd_type_t lcd_type = watch_get_lcd_type();
    state->num_com = 3;
    state->num_seg = 27 - state->num_com;

    if (lcd_type == WATCH_LCD_TYPE_CUSTOM) {
        state->num_com = 4;
    }

    if (lcd_type == WATCH_LCD_TYPE_GSHOCK) {
        state->num_com = 4;
        state->num_seg = 27;
    }

    state->curr_com = 0;
    state->curr_seg = 0;
    state->delay_ticks = 0;
    state->curr_show = ALL_SEGMENTS_SHOW_FULL;
    movement_request_tick_frequency(TICK_FREQ);
}

bool all_segments_face_loop(movement_event_t event, void *context) {
    all_segments_state_t *state = (all_segments_state_t *)context;
    switch (event.event_type) {
        case EVENT_TICK:
        if (state->delay_ticks > 0) {
            state->delay_ticks--;
            if (state->delay_ticks == 0) {
                watch_clear_display();
            }
            break;
        }
        switch (state->curr_show) {
            case ALL_SEGMENTS_SHOW_FULL:
                for (state->curr_com = 0; state->curr_com < state->num_com; state->curr_com++) {
                    for (state->curr_seg = 0; state->curr_seg < state->num_seg; state->curr_seg++) {
                        watch_set_pixel(state->curr_com, state->curr_seg);
                    }
                }
                state->curr_com = 0;
                state->curr_seg = 0;
                state->delay_ticks = TICK_FREQ * WAIT_SEC;
                state->curr_show = (state->curr_show + 1) % ALL_SEGMENTS_COUNT;
                break;
            case ALL_SEGMENTS_SHOW_FULL_SLOWLY:
                if (state->curr_seg >= state->num_seg) {
                    state->curr_com = state->curr_com + 1;
                    state->curr_seg = 0 ;
                }
                if (state->curr_com >= state->num_com) {
                    state->curr_com = 0;
                    state->curr_seg = 0;
                    state->delay_ticks = TICK_FREQ * WAIT_SEC;
                    state->curr_show = (state->curr_show + 1) % ALL_SEGMENTS_COUNT;
                }
                watch_set_pixel(state->curr_com, state->curr_seg);
                printf("COM: %d SEG: %d\r\n", state->curr_com, state->curr_seg);
                state->curr_seg += 1;
                break;
            case ALL_SEGMENTS_SHOW_FULL_COM:
                if (state->curr_seg >= state->num_seg) {
                    state->curr_com = state->curr_com + 1;
                    state->curr_seg = 0 ;
                    watch_clear_display();
                }
                if (state->curr_com >= state->num_com) {
                    state->curr_com = 0;
                    state->curr_seg = 0;
                    state->delay_ticks = TICK_FREQ * WAIT_SEC;
                    state->curr_show = (state->curr_show + 1) % ALL_SEGMENTS_COUNT;
                }
                watch_set_pixel(state->curr_com, state->curr_seg);
                printf("COM: %d SEG: %d\r\n", state->curr_com, state->curr_seg);
                state->curr_seg += 1;
                break;
            case ALL_SEGMENTS_SHOW_INDIVIDUAL:
                watch_clear_display();
                if (state->curr_seg >= state->num_seg) {
                    state->curr_com = state->curr_com + 1;
                    state->curr_seg = 0 ;
                }
                if (state->curr_com >= state->num_com) {
                    state->curr_com = 0;
                    state->curr_seg = 0;
                    state->delay_ticks = TICK_FREQ * WAIT_SEC;
                    state->curr_show = (state->curr_show + 1) % ALL_SEGMENTS_COUNT;
                }
                watch_set_pixel(state->curr_com, state->curr_seg);
                printf("COM: %d SEG: %d\r\n", state->curr_com, state->curr_seg);
                state->curr_seg += 1;
                break;
            case ALL_SEGMENTS_COUNT:
                break;
        }
        break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void all_segments_face_resign(void *context) {
    (void) context;
}
