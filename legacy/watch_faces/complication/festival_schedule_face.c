/*
 * MIT License
 *
 * Copyright (c) 2024 <David Volovskiy>
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
#include "festival_schedule_face.h"
#include "festival_schedule_arr.h"
#include "watch_utility.h"

#ifndef min
#define min(x, y) ((x) > (y) ? (y) : (x))
#endif

const char festival_name[2] = "LA";

const char festival_stage[FESTIVAL_SCHEDULE_STAGE_COUNT + 1][2] =
{
    [FESTIVAL_SCHEDULE_NO_STAGE]    = "  ",
    [FESTIVAL_SCHEDULE_T_MOBILE]    = "TM",
    [FESTIVAL_SCHEDULE_LAKESHORE]   = "L ",
    [FESTIVAL_SCHEDULE_BUD_LIGHT]   = "BL",
    [FESTIVAL_SCHEDULE_TITOS]       = "TO",
    [FESTIVAL_SCHEDULE_PERRYS]      = "PR",
    [FESTIVAL_SCHEDULE_THE_GROVE]   = "GR",
    [FESTIVAL_SCHEDULE_BMI]         = "BM",
    [FESTIVAL_SCHEDULE_STAGE_COUNT] = "  "
};

const char festival_genre[FESTIVAL_SCHEDULE_GENRE_COUNT + 1][6] =
{
    [FESTIVAL_SCHEDULE_NO_GENRE]    = " NONE ",
    [FESTIVAL_SCHEDULE_POP]         = " POP  ",
    [FESTIVAL_SCHEDULE_INDIE]       = " INdIE",
    [FESTIVAL_SCHEDULE_DREAM_POP]   = "dream7",
    [FESTIVAL_SCHEDULE_K_POP]       = " K-POP",
    [FESTIVAL_SCHEDULE_ROCK]        = " ROCK ",
    [FESTIVAL_SCHEDULE_ALT]         = "   ALT",
    [FESTIVAL_SCHEDULE_PUNK]        = "PUNK  ",
    [FESTIVAL_SCHEDULE_NU_METAL]    = "Num&tL",
    [FESTIVAL_SCHEDULE_PSYCH_ROCK]  = " PSYCH",
    [FESTIVAL_SCHEDULE_HOUSE]       = " HOUSE",
    [FESTIVAL_SCHEDULE_DUBSTEP]     = "DUBStP",
    [FESTIVAL_SCHEDULE_TECHNO]      = " tECNO",
    [FESTIVAL_SCHEDULE_BASS]        = " BASS ",
    [FESTIVAL_SCHEDULE_DnB]         = " dnB  ",
    [FESTIVAL_SCHEDULE_DANCE]       = " DaNCE",
    [FESTIVAL_SCHEDULE_RAP]         = "  rAP ",
    [FESTIVAL_SCHEDULE_TRAP]        = " trAP ",
    [FESTIVAL_SCHEDULE_EMORAP]      = "sdcld ",
    [FESTIVAL_SCHEDULE_SOUL]        = " SOUL ",
    [FESTIVAL_SCHEDULE_RnB]         = " rnb  ",
    [FESTIVAL_SCHEDULE_COUNTRY]     = "Cuntry",
    [FESTIVAL_SCHEDULE_FOLK]        = " FOLK ",
    [FESTIVAL_SCHEDULE_OTHER]       = "OTHEr ",
    [FESTIVAL_SCHEDULE_GENRE_COUNT] = "      "
};

#define FREQ_FAST 8
#define FREQ 4

#define TIMEOUT_SEC 10
#define TIMEOUT_HELD_SEC 2

static int16_t _text_pos;
static const char* _text_looping;
static watch_date_time_t _starting_time;
static watch_date_time_t _ending_time;
static bool _quick_ticks_running;
static uint8_t _ts_ticks;
static festival_schedule_tick_reason _ts_ticks_purpose;
static const uint8_t _act_arr_size = sizeof(festival_acts) / sizeof(festival_schedule_t);
static bool in_le;


static uint8_t _get_next_act_num(uint8_t act_num, bool get_prev){
    int increment = get_prev ? -1 : 1;
    uint8_t next_act = act_num;
    do{
    next_act = (next_act + increment + _act_arr_size) % _act_arr_size;
    }
    while (festival_acts[next_act].start_time.reg == 0);
    return next_act;
}


// Returns 0 if they're the same; Positive if dt1 is newer than dt2; Negative o/w
static int _compare_dates_times(watch_date_time_t dt1, watch_date_time_t dt2) {
    if (dt1.unit.year != dt2.unit.year) {
        return dt1.unit.year - dt2.unit.year;
    }
    if (dt1.unit.month != dt2.unit.month) {
        return dt1.unit.month - dt2.unit.month;
    }
    if (dt1.unit.day != dt2.unit.day) {
        return dt1.unit.day - dt2.unit.day;
    }
    if (dt1.unit.hour != dt2.unit.hour) {
        return dt1.unit.hour - dt2.unit.hour;
    }
    return dt1.unit.minute - dt2.unit.minute;
}

// Returns -1 if already passed, o/w days until start.
static int16_t _get_days_until(watch_date_time_t start_time, watch_date_time_t curr_time){
    start_time.unit.hour = start_time.unit.minute = start_time.unit.second = 0;
    curr_time.unit.hour = curr_time.unit.minute = curr_time.unit.second = 0;
    uint32_t now_timestamp = watch_utility_date_time_to_unix_time(curr_time, 0);
    uint32_t start_timestamp = watch_utility_date_time_to_unix_time(start_time, 0);
    int16_t days_until;
    if (now_timestamp > start_timestamp) // Date already passed
        days_until = -1;
    else
        days_until = (start_timestamp - now_timestamp) / (60 * 60 * 24);
    return days_until;
}

static bool _act_is_playing(uint8_t act_num, watch_date_time_t curr_time){
    if (act_num == FESTIVAL_SCHEDULE_NUM_ACTS) return false;
    return _compare_dates_times(festival_acts[act_num].start_time, curr_time) <= 0 && _compare_dates_times(curr_time, festival_acts[act_num].end_time) < 0;
}

static uint8_t _act_performing_on_stage(uint8_t stage, watch_date_time_t curr_time)
{
    for (int i = 0; i < FESTIVAL_SCHEDULE_NUM_ACTS; i++) {
        if (festival_acts[i].stage == stage && _act_is_playing(i, curr_time))
            return i;
    }
    return FESTIVAL_SCHEDULE_NUM_ACTS;
}

static uint8_t _find_first_available_act(uint8_t first_stage_to_check, watch_date_time_t curr_time, bool reverse)
{
    int increment = reverse ? -1 : 1;
    uint8_t last_stage = (first_stage_to_check - increment + FESTIVAL_SCHEDULE_STAGE_COUNT) % FESTIVAL_SCHEDULE_STAGE_COUNT;
    for (int i = first_stage_to_check;; i = (i + increment + FESTIVAL_SCHEDULE_STAGE_COUNT) % FESTIVAL_SCHEDULE_STAGE_COUNT) {
        uint8_t act_num = _act_performing_on_stage(i, curr_time);
        if (act_num != FESTIVAL_SCHEDULE_NUM_ACTS)
            return act_num;
        if (i == last_stage) break;
    }
    return FESTIVAL_SCHEDULE_NUM_ACTS;
}

static void _display_act(festival_schedule_state_t *state){
    char buf[11];
    uint8_t popularity = festival_acts[state->curr_act].popularity;
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
    _text_looping = festival_acts[state->curr_act].artist;
    _text_pos = FREQ * -1;
    if (popularity > 0 && popularity < 40)
        sprintf(buf, "%.2s%2d%.6s", festival_stage[state->curr_stage], festival_acts[state->curr_act].popularity, festival_acts[state->curr_act].artist);
    else
        sprintf(buf, "%.2s  %.6s", festival_stage[state->curr_stage], festival_acts[state->curr_act].artist);
    watch_display_string(buf , 0);
}

static void _display_act_genre(uint8_t act_num, bool show_weekday){
    char buf[11];
    if (show_weekday){
        watch_date_time_t start_time = festival_acts[act_num].start_time;
        if (start_time.unit.hour < 5)
            start_time.reg = start_time.reg - (1<<17); // Subtract a day if the act starts before 5am.
        sprintf(buf, "%s G%.6s", watch_utility_get_weekday(start_time), festival_genre[festival_acts[act_num].genre]);
        watch_display_string(buf , 0);
    }
    else{
        sprintf(buf, " G%.6s", festival_genre[festival_acts[act_num].genre]);
        watch_display_string(buf , 2);
    }
}

static void _display_act_time(uint8_t act_num, bool clock_mode_24h, bool display_end){
    char buf[11];
    watch_date_time_t disp_time = display_end ? festival_acts[act_num].end_time : festival_acts[act_num].start_time;
    watch_set_colon();
    if (clock_mode_24h){
        watch_set_indicator(WATCH_INDICATOR_24H);

    }
    else{
        watch_clear_indicator(WATCH_INDICATOR_24H);
        // if we are in 12 hour mode, do some cleanup.
        if (disp_time.unit.hour < 12) {
            watch_clear_indicator(WATCH_INDICATOR_PM);
        } else {
            watch_set_indicator(WATCH_INDICATOR_PM);
        }
        disp_time.unit.hour %= 12;
        if (disp_time.unit.hour == 0) disp_time.unit.hour = 12;
    }
    sprintf(buf, "%s%2d%2d%.2d%s", watch_utility_get_weekday(disp_time), disp_time.unit.day, disp_time.unit.hour, disp_time.unit.minute, display_end ? "Ed" : "St");
    watch_display_string(buf, 0);
}

static watch_date_time_t _get_starting_time(void){
    watch_date_time_t date_oldest = festival_acts[0].start_time;
    for (int i = 1; i < FESTIVAL_SCHEDULE_NUM_ACTS; i++) {
        if (festival_acts[i].artist[0] == 0) continue;
        watch_date_time_t date_check = festival_acts[i].start_time;
        if (_compare_dates_times(date_check, date_oldest) < 0)
            date_oldest= date_check;
    }
    return date_oldest;
}

static watch_date_time_t _get_ending_time(void){
    watch_date_time_t date_newest = festival_acts[0].end_time;
    for (int i = 1; i < FESTIVAL_SCHEDULE_NUM_ACTS; i++) {
        watch_date_time_t date_check = festival_acts[i].end_time;
        if (_compare_dates_times(date_check, date_newest) > 0)
            date_newest= date_check;
    }
    return date_newest;
}

static bool _festival_occurring(watch_date_time_t curr_time, bool update_display){
    char buf[15];
    if (_compare_dates_times(_starting_time, curr_time) > 0){
        int16_t days_until = _get_days_until(_starting_time, curr_time);
        if (update_display){
            if (days_until == 0) return true;
            if (days_until <= 999){
                if (days_until > 99) sprintf(buf, "%.2s%02d%3dday", festival_name, _starting_time.unit.year + 20, days_until);
                else sprintf(buf, "%.2s%02d%2d day", festival_name, _starting_time.unit.year + 20, days_until);
            }
            else sprintf(buf, "%.2s%02dWAIT  ", festival_name, _starting_time.unit.year + 20);
            watch_display_string(buf , 0);
        }
        return days_until == 0;
    }
    else if (_compare_dates_times(_ending_time, curr_time) <= 0){
        if (update_display){
            sprintf(buf, "%.2s%02dOVER  ", festival_name, _starting_time.unit.year + 20);
            watch_display_string(buf , 0);
        }
        return false;
    }
    return true;
}

static void _display_curr_day(watch_date_time_t curr_time){  // Assumes festival_occurring function was run before it.
    char buf[13];
    int16_t days_until = _get_days_until(curr_time, _starting_time) + 1;
    if (days_until < 100 && days_until >= 0)
        sprintf(buf, "%.2s%02d day%2d", festival_name, _starting_time.unit.year + 20, days_until);
    else
        sprintf(buf, "%.2s%02d LONg ", festival_name, _starting_time.unit.year + 20);
    watch_display_string(buf , 0);
    return;
}

static void set_ticks_purpose(festival_schedule_tick_reason purpose) {
    _ts_ticks_purpose = purpose;
    switch (purpose) {
        case FESTIVAL_SCHEDULE_TICK_NONE:
            _ts_ticks = 0;
            break;
        case FESTIVAL_SCHEDULE_TICK_SCREEN:
            _ts_ticks = TIMEOUT_SEC * FREQ;
            break;
        case FESTIVAL_SCHEDULE_TICK_LEAVE:
        case FESTIVAL_SCHEDULE_TICK_CYCLE:
            _ts_ticks = TIMEOUT_HELD_SEC * FREQ;
            break;
    }
}

static void _display_title(festival_schedule_state_t *state){
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_TITLE;
    state->curr_act = FESTIVAL_SCHEDULE_NUM_ACTS;
    watch_clear_colon();
    watch_clear_all_indicators();
    state->cyc_through_all_acts = false;
    watch_date_time_t curr_time = movement_get_local_date_time();
    state -> prev_day = (curr_time.reg >> 17);
    state -> festival_occurring = _festival_occurring(curr_time, true);
    if (state -> festival_occurring) _display_curr_day(curr_time);
}

static void _display_screen(festival_schedule_state_t *state, bool clock_mode_24h){
    set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_SCREEN);
    if (state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_START_TIME && state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_END_TIME)
    {
        watch_clear_colon();
        watch_clear_indicator(WATCH_INDICATOR_24H);
        watch_clear_indicator(WATCH_INDICATOR_PM);
    }
    switch (state->curr_screen)
    {
    case FESTIVAL_SCHEDULE_SCREEN_ACT:
    case FESTIVAL_SCHEDULE_SCREENS_COUNT:
        _display_act(state);
        break;
    case FESTIVAL_SCHEDULE_SCREEN_GENRE:
        _display_act_genre(state->curr_act, state->cyc_through_all_acts);
        break;
    case FESTIVAL_SCHEDULE_SCREEN_START_TIME:
        _display_act_time(state->curr_act, clock_mode_24h, false);
        break;
    case FESTIVAL_SCHEDULE_SCREEN_END_TIME:
        _display_act_time(state->curr_act, clock_mode_24h, true);
        break;
    case FESTIVAL_SCHEDULE_SCREEN_TITLE:
        _display_title(state);
        break;
    }
}

void festival_schedule_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(festival_schedule_state_t));
        memset(*context_ptr, 0, sizeof(festival_schedule_state_t));
        festival_schedule_state_t *state = (festival_schedule_state_t *)*context_ptr;
        state->curr_act = FESTIVAL_SCHEDULE_NUM_ACTS;
        state->prev_act = FESTIVAL_SCHEDULE_NUM_ACTS + 1;
        state -> prev_day = 0;
        state->cyc_through_all_acts = false;
    }
}

static void _cyc_all_acts(festival_schedule_state_t *state, bool clock_mode_24h, bool handling_light){
    state->cyc_through_all_acts = true;
    watch_set_indicator(WATCH_INDICATOR_LAP);
    state->curr_act = _get_next_act_num(state->curr_act, handling_light);
    state->curr_stage = festival_acts[state->curr_act].stage;
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
    _display_screen(state, clock_mode_24h);
    return; 
}

static void _handle_btn_up(festival_schedule_state_t *state, bool clock_mode_24h, bool handling_light){
    set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_NONE);
    if (state->cyc_through_all_acts){
        _cyc_all_acts(state, clock_mode_24h, handling_light);
        return;
    }
    if (!state->festival_occurring) return;
    watch_date_time_t curr_time = movement_get_local_date_time();
    if (state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_TITLE) {
        state->curr_stage = handling_light ? 
                        (state->curr_stage - 1 + FESTIVAL_SCHEDULE_STAGE_COUNT) % FESTIVAL_SCHEDULE_STAGE_COUNT :
                        (state->curr_stage + 1) % FESTIVAL_SCHEDULE_STAGE_COUNT;
    }
    if (SHOW_EMPTY_STAGES) {
        state->curr_act = _act_performing_on_stage(state->curr_stage, curr_time);
    }
    else{
        state->curr_act = _find_first_available_act(state->curr_stage, curr_time, handling_light);
        state->curr_stage = festival_acts[state->curr_act].stage;
    }
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
    _display_screen(state, clock_mode_24h);
}

static void start_quick_cyc(void){
    _quick_ticks_running = true;
    movement_request_tick_frequency(FREQ_FAST);
}

static int16_t _loop_text(const char* text, int8_t curr_loc, uint8_t char_len){
    // if curr_loc is negative, then use that many ticks as a delay before looping
    char buf[16];
    const uint8_t num_spaces = 2;
    uint8_t spaces = num_spaces;
    uint8_t text_len = strlen(text);
    uint8_t pos = 10 - char_len;
    if (curr_loc == -1) curr_loc = 0;  // To avoid double-showing the 0
    if (char_len >= text_len || curr_loc < 0) {
        sprintf(buf, "%s", text);
        watch_display_string(buf, pos);
        if (curr_loc < 0) return ++curr_loc;
        return 0;
    }
    else if (curr_loc >= (text_len + num_spaces))
        curr_loc = 0;
    sprintf(buf, "%.6s", text + curr_loc);
    if (curr_loc > text_len)
        spaces = min(num_spaces, curr_loc - text_len);
    for (int i = 0; i < spaces; i++)
        strcat(buf, " ");
    strncat(buf, text, 7-spaces);
    watch_display_string(buf, pos);
    return ++curr_loc;
}

static void handle_ts_ticks(festival_schedule_state_t *state, bool clock_mode_24h){
    static bool _light_held;
    if (_light_held){
        if (!HAL_GPIO_BTN_LIGHT_read()) _light_held = false;
        else return;
    }
    if (_ts_ticks != 0){
        --_ts_ticks;
        switch (_ts_ticks_purpose){
            case FESTIVAL_SCHEDULE_TICK_NONE:
                _ts_ticks = 0;
                break;
            case FESTIVAL_SCHEDULE_TICK_SCREEN:
                if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE || state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_ACT){
                    _ts_ticks = 0;
                }
                else if (_ts_ticks == 0){
                    if(HAL_GPIO_BTN_LIGHT_read()){
                        _ts_ticks = 1 * FREQ; // Give one extra second of delay when the light is on
                        _light_held = true;
                    }
                    else{
                        set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_NONE);
                        state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
                        _display_screen(state, clock_mode_24h);
                    }
                }
                break;
            case FESTIVAL_SCHEDULE_TICK_LEAVE:
                if (!HAL_GPIO_BTN_MODE_read())_ts_ticks = 0;
                else if (_ts_ticks == 0){
                    if(state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE) movement_move_to_face(0);
                    else{
                        set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_LEAVE);  // This is unneeded, but explicit that we remain in TICK_LEAVE
                        _display_title(state);
                    }
                }
                break;
            case FESTIVAL_SCHEDULE_TICK_CYCLE:
                if (_ts_ticks == 0){
                    set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_NONE);
                    start_quick_cyc();
                }
                break;
        }
    }
}

static bool handle_tick(festival_schedule_state_t *state){
    // Returns true if something on the screen changed.
    watch_date_time_t curr_time;
    if (_quick_ticks_running) {
        if (HAL_GPIO_BTN_LIGHT_read()) _handle_btn_up(state, movement_clock_mode_24h(), true);
        else if (HAL_GPIO_BTN_ALARM_read()) _handle_btn_up(state, movement_clock_mode_24h(), false);
        else{
            _quick_ticks_running = false;
            movement_request_tick_frequency(FREQ);
        }
    }
    handle_ts_ticks(state, movement_clock_mode_24h());

    if (state->cyc_through_all_acts) return false;
    curr_time = movement_get_local_date_time();
    if (curr_time.unit.second != 0) return false;
    bool newDay = ((curr_time.reg >> 17) != (state -> prev_day));
    state -> prev_day = (curr_time.reg >> 17);
    state -> festival_occurring = _festival_occurring(curr_time, (newDay && !state->cyc_through_all_acts));
    if (!state->festival_occurring) return false;
    if(state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE) {
        if (newDay) _display_curr_day(curr_time);
        return false;
    }
    if (!_act_is_playing(state->curr_act, curr_time)){
        if (SHOW_EMPTY_STAGES) {
            state->curr_act = FESTIVAL_SCHEDULE_NUM_ACTS;
        }   
        else{
            state->curr_act = _find_first_available_act(state->curr_stage, curr_time, false);
            state->curr_stage = festival_acts[state->curr_act].stage;
        } 
    }
    if ((state->curr_stage == state->prev_stage) && (state->curr_act == state->prev_act)) return false;
    state->prev_stage = state->curr_stage;
    state->prev_act = state->curr_act;
    state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
    _display_screen(state, movement_clock_mode_24h());
    return true;
}

void festival_schedule_face_activate(void *context) {
    (void) context;
    _starting_time = _get_starting_time();
    _ending_time = _get_ending_time();
    _quick_ticks_running = false;
    set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_NONE);
    movement_request_tick_frequency(FREQ);
}

bool festival_schedule_face_loop(movement_event_t event, void *context) {
    festival_schedule_state_t *state = (festival_schedule_state_t *)context;
    bool changed_from_handle_ticks;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            in_le = false;
            if (state->curr_act == FESTIVAL_SCHEDULE_NUM_ACTS) {
                _display_title(state);
            }
            break;
        case EVENT_TICK:
            changed_from_handle_ticks = handle_tick(state);
            if (!changed_from_handle_ticks && state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_ACT
                && !_quick_ticks_running)
                    _text_pos = _loop_text(_text_looping, _text_pos, 6);
                break;
        case EVENT_LOW_ENERGY_UPDATE:
            changed_from_handle_ticks = handle_tick(state);
            if (!changed_from_handle_ticks && !in_le 
                && event.event_type == EVENT_LOW_ENERGY_UPDATE 
                && state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_ACT) {
                in_le = true;
                if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_ACT) {
                    // Resets the act name in LE mode so the beginning of it is shown
                    _display_screen(state, movement_clock_mode_24h());
                }
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            _handle_btn_up(state, movement_clock_mode_24h(), true);
            break;
        case EVENT_ALARM_BUTTON_UP:
            _handle_btn_up(state, movement_clock_mode_24h(), false);
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE){
                _cyc_all_acts(state, movement_clock_mode_24h(), false);
                set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_CYCLE);
            }
            else if (state->festival_occurring && !state->cyc_through_all_acts) break;
            else start_quick_cyc();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            break;  // To overwrite the default LED on
        case EVENT_LIGHT_LONG_PRESS:
            if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE){
                _cyc_all_acts(state, movement_clock_mode_24h(), true);
                set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_CYCLE);
            }
            else if (state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_ACT || (state->festival_occurring && !state->cyc_through_all_acts))
                movement_illuminate_led(); // Will allow led for see acts' genre and times
            else start_quick_cyc();
            break;
        case EVENT_MODE_LONG_PRESS:

            if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE){
                movement_move_to_face(0);
            }
            if (state->curr_screen != FESTIVAL_SCHEDULE_SCREEN_ACT){
                state->curr_screen = FESTIVAL_SCHEDULE_SCREEN_ACT;
                _display_screen(state, movement_clock_mode_24h());
                set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_LEAVE);
            }
            else {
                set_ticks_purpose(FESTIVAL_SCHEDULE_TICK_LEAVE);
                _display_title(state);
            }
            break;
        case EVENT_MODE_BUTTON_UP:
            if (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE) movement_move_to_next_face();
            else if (state->curr_act != FESTIVAL_SCHEDULE_NUM_ACTS){
                do
                {
                    state->curr_screen = (state->curr_screen + 1) % FESTIVAL_SCHEDULE_SCREENS_COUNT;
                } while (state->curr_screen == FESTIVAL_SCHEDULE_SCREEN_TITLE);
                _display_screen(state, movement_clock_mode_24h());
            }
            break;
        case EVENT_TIMEOUT:
            if (state->cyc_through_all_acts){
                state->cyc_through_all_acts = false;
                _display_title(state);
            }
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void festival_schedule_face_resign(void *context) {
    festival_schedule_state_t *state = (festival_schedule_state_t *)context;
    state->curr_act = FESTIVAL_SCHEDULE_NUM_ACTS;
    state->cyc_through_all_acts = false;
    state->prev_act = FESTIVAL_SCHEDULE_NUM_ACTS + 1;
}

