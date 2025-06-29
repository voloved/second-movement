/*
 * MIT License
 *
 * Copyright (c) 2024 <#David Volovskiy#>
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
#include "party_face.h"
#include "watch_utility.h"

#define HRS_BLINK_BEFORE_SLEEP 10
#define HRS_BLINK_BEFORE_SLEEP_LED 3


void party_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(party_state_t));
        memset(*context_ptr, 0, sizeof(party_state_t));
    }
}

void party_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    party_state_t *state = (party_state_t *)context;
    state->blink = false;
    state->led = false;
    state->fast = false;
    state->prev_text = -1;
}

static void _party_face_init_lcd(party_state_t *state) {
    char text[11];
    const char partyTime[][7] = {"Party", "Tin&e", " It's"};
    const char secondaryText[][7] = {"Pron&"};
    const int partyTextNum = sizeof(partyTime) / sizeof(partyTime[0]);
    const int secondaryTextNum = sizeof(secondaryText) / sizeof(secondaryText[0]);
    const char (*textArray)[7];
    int textArrayNum;
    watch_date_time_t date_time;
    uint8_t disp_loc = 0;
    switch (state->text)
    {
    case 1:
        textArray = secondaryText;
        textArrayNum = secondaryTextNum;
        break;
    case 0:
    default:
        textArray = partyTime;
        textArrayNum = partyTextNum;
        break;
    }
    if (!state->blink){
        state->party_text = 0;
        watch_clear_indicator(WATCH_INDICATOR_BELL);
    }
    else{
        state->party_text = (state->party_text + 1)  % textArrayNum;
        watch_set_indicator(WATCH_INDICATOR_BELL);
    }
    date_time = movement_get_local_date_time();
    if (state->prev_text == state->text && date_time.unit.day == state->curr_day){
        disp_loc = 5;
        sprintf(text, "%s",textArray[state->party_text]);
    }
    else if (state->text == 1) {
        disp_loc = 5;
        sprintf(text, "%s",textArray[state->party_text]);
        watch_clear_display();
    }
    else {
        sprintf(text, "%s%2d %s", watch_utility_get_weekday(date_time), date_time.unit.day, textArray[state->party_text]);
        state->curr_day = date_time.unit.day;
    }
    watch_display_string(text, disp_loc);
    state->prev_text = state->text;
}

static void offset_allow_sleep(bool blinking, bool led_on) {
    movement_cancel_background_task();
    if (!blinking) return;
    uint32_t ts = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), 0);
    uint8_t hours_to_delay = led_on ? HRS_BLINK_BEFORE_SLEEP_LED : HRS_BLINK_BEFORE_SLEEP;
    ts = watch_utility_offset_timestamp(ts, hours_to_delay, 0, 0);  // Allow the watch to blink without timing out for 3 hours.
    movement_schedule_background_task(watch_utility_date_time_from_unix_time(ts, 0));
}

bool party_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    party_state_t *state = (party_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            movement_request_tick_frequency(state->fast ? 4 : 2);
            _party_face_init_lcd(state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->led = (state->led + 1) % 3;
            offset_allow_sleep(state->blink, state->led != 0);
            if (!state->led){
                watch_set_led_off();
                break;
            }
            break;
        case EVENT_LIGHT_LONG_PRESS:
            state->text = (state->text + 1) % MAX_TEXT;
            _party_face_init_lcd(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->blink = !state->blink;
            if (!state->blink)
                _party_face_init_lcd(state);
            offset_allow_sleep(state->blink, state->led != 0);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->fast = !state->fast;
            movement_request_tick_frequency(state->fast ? 4 : 2);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            watch_set_led_off();
            break;
        case EVENT_TICK:
            if (state->blink) {
                if (event.subsecond % 2 == 0)
                    _party_face_init_lcd(state);
                else if (state->text == 0){  // Clear only the bottom row when the party text is occurring
                    watch_display_string("      ", 4);
                    watch_clear_indicator(WATCH_INDICATOR_BELL);
                }
                else
                    watch_clear_display();
            }
            switch (state->led)
            {
            case 0:        
            default:
                break;
            case 1:
                if (event.subsecond % 2 == 0)
                    watch_set_led_green();
                else
                    watch_set_led_off();
                break;
            case 2:
                switch (state->color)
                {
                case 0:
                    watch_set_led_red();
                    break;
                 case 1:
                    watch_set_led_green();
                    break;
                case 2:
                    watch_set_led_yellow();
                    break;               
                default:
                    watch_set_led_off();
                    break;
                }
                state->color = (state->color + 1) % 3;  
            }
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;
        case EVENT_BACKGROUND_TASK:
            watch_set_led_off();
            _party_face_init_lcd(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void party_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    watch_set_led_off();
    movement_cancel_background_task();
}

