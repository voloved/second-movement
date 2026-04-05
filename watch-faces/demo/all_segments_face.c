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
#include "watch_common_display.h"

#define TICK_FREQ 4
#define WAIT_SEC 1

typedef enum {
    ALL_SEGMENTS_SHOW_FULL = 0,
    ALL_SEGMENTS_SHOW_NONSENSE,
    ALL_SEGMENTS_SHOW_FULL_SLOWLY,
    ALL_SEGMENTS_SHOW_FULL_COM,
    ALL_SEGMENTS_SHOW_INDIVIDUAL,
    ALL_SEGMENTS_SHOW_CHARACTERS,
    ALL_SEGMENTS_SHOW_SEGMENTS,
    ALL_SEGMENTS_COUNT
} all_segments_show;

static all_segments_show _curr_show;
static uint8_t _num_com;
static uint8_t _num_seg;
static uint8_t _curr_com;
static uint8_t _curr_seg;
static uint8_t _delay_ticks;
static uint8_t _character;
static uint8_t _segment;

void all_segments_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    (void) context_ptr;
}

void all_segments_face_activate(void *context) {
    (void) context;
    watch_lcd_type_t lcd_type = watch_get_lcd_type();
    _num_com = 3;
    _num_seg = 27 - _num_com;

    if (lcd_type == WATCH_LCD_TYPE_CUSTOM) {
        _num_com = 4;
    }

    if (lcd_type == WATCH_LCD_TYPE_GSHOCK) {
        _num_com = 4;
        _num_seg = 27;
    }

    _curr_com = 0;
    _curr_seg = 0;
    _character = '0';
    _segment = 0;
    _delay_ticks = 0;
    _curr_show = ALL_SEGMENTS_SHOW_FULL;
    movement_request_tick_frequency(TICK_FREQ);
}

bool all_segments_face_loop(movement_event_t event, void *context) {
    (void) context;
    switch (event.event_type) {
        case EVENT_ALARM_BUTTON_UP:
            _character = '0';
            _segment = 0;
            _curr_com = 0;
            _curr_seg = 0;
            _delay_ticks = 0;
            _curr_show = (_curr_show + 1) % ALL_SEGMENTS_COUNT;
            watch_clear_display();
            break;
        case EVENT_TICK:
        if (_delay_ticks > 0) {
            _delay_ticks--;
            if (_delay_ticks == 0) {
                watch_clear_display();
            }
            break;
        }
        switch (_curr_show) {
            case ALL_SEGMENTS_SHOW_FULL:
                for (_curr_com = 0; _curr_com < _num_com; _curr_com++) {
                    for (_curr_seg = 0; _curr_seg < _num_seg; _curr_seg++) {
                        watch_set_pixel(_curr_com, _curr_seg);
                    }
                }
                break;
            case ALL_SEGMENTS_SHOW_FULL_SLOWLY:
                if (_curr_seg >= _num_seg) {
                    _curr_com = _curr_com + 1;
                    _curr_seg = 0 ;
                }
                if (_curr_com >= _num_com) {
                    _curr_com = 0;
                    _curr_seg = 0;
                    _delay_ticks = TICK_FREQ * WAIT_SEC;
                }
                watch_set_pixel(_curr_com, _curr_seg);
                printf("COM: %d SEG: %d\r\n", _curr_com, _curr_seg);
                _curr_seg += 1;
                break;
            case ALL_SEGMENTS_SHOW_NONSENSE:
                watch_display_text(WATCH_POSITION_SECONDS, "01");
                break;
            case ALL_SEGMENTS_SHOW_FULL_COM:
                if (_curr_seg >= _num_seg) {
                    _curr_com = _curr_com + 1;
                    _curr_seg = 0 ;
                    watch_clear_display();
                }
                if (_curr_com >= _num_com) {
                    _curr_com = 0;
                    _curr_seg = 0;
                    _delay_ticks = TICK_FREQ * WAIT_SEC;
                }
                watch_set_pixel(_curr_com, _curr_seg);
                printf("COM: %d SEG: %d\r\n", _curr_com, _curr_seg);
                _curr_seg += 1;
                break;
            case ALL_SEGMENTS_SHOW_INDIVIDUAL:
                watch_clear_display();
                if (_curr_seg >= _num_seg) {
                    _curr_com = _curr_com + 1;
                    _curr_seg = 0 ;
                }
                if (_curr_com >= _num_com) {
                    _curr_com = 0;
                    _curr_seg = 0;
                    _delay_ticks = TICK_FREQ * WAIT_SEC;
                }
                watch_set_pixel(_curr_com, _curr_seg);
                printf("COM: %d SEG: %d\r\n", _curr_com, _curr_seg);
                _curr_seg += 1;
                break;
            case ALL_SEGMENTS_SHOW_CHARACTERS:
                watch_display_character(_character, _segment);
                if (_character == 'z'){
                    _character = '0';
                    _segment += 1;
                    watch_clear_display();
                    if (_segment > 11) {
                        _curr_com = 0;
                        _curr_seg = 0;
                        _character = '0';
                        _segment = 0;
                        _delay_ticks = TICK_FREQ * WAIT_SEC;
                    }
                } else {
                    _character +=1;
                }
                break;

            case ALL_SEGMENTS_SHOW_SEGMENTS:
                watch_clear_all_indicators();
                watch_set_indicator(_segment);
                printf("watch_set_indicator: %d\r\n", _segment);
                _segment += 1;
                if (_segment > WATCH_INDICATOR_BOX_COLON_BOTTOM){
                    _segment = 0;
                    _delay_ticks = TICK_FREQ * WAIT_SEC;
                }
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
