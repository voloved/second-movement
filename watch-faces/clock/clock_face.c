/* SPDX-License-Identifier: MIT */

/*
 * MIT License
 *
 * Copyright © 2021-2023 Joey Castillo <joeycastillo@utexas.edu> <jose.castillo@gmail.com>
 * Copyright © 2022 David Keck <davidskeck@users.noreply.github.com>
 * Copyright © 2022 TheOnePerson <a.nebinger@web.de>
 * Copyright © 2023 Jeremy O'Brien <neutral@fastmail.com>
 * Copyright © 2023 Mikhail Svarichevsky <3@14.by>
 * Copyright © 2023 Wesley Aptekar-Cassels <me@wesleyac.com>
 * Copyright © 2024 Matheus Afonso Martins Moreira <matheus.a.m.moreira@gmail.com>
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
#include <math.h>
#include "clock_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_common_display.h"
#include "filesystem.h"
#include "sunriset.h"

// 2.4 volts seems to offer adequate warning of a low battery condition?
// refined based on user reports and personal observations; may need further adjustment.
#ifndef CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD
#define CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD 2400
#endif

static movement_location_t load_location_from_filesystem() {
    movement_location_t location = {0};
    filesystem_read_file("location.u32", (char *) &location.reg, sizeof(movement_location_t));
    return location;
}

#if __EMSCRIPTEN__
static void print_time_debug(watch_date_time_t date_time, const char *time_name) {
    // Such as: print_time_debug(date_time, "Now");
    //          print_time_debug(rise_set_info.time_rise, "Rise");
    //          print_time_debug(rise_set_info.time_set, "Set");
    printf("%s: %d:%02d  %d-%d-%d\n", time_name, date_time.unit.hour,
            date_time.unit.minute, date_time.unit.month,
            date_time.unit.day, date_time.unit.year + WATCH_RTC_REFERENCE_YEAR);
}
#endif

static watch_date_time_t _get_rise_set_time(double rise_set_val, watch_date_time_t date_time) {
    watch_date_time_t scratch_time;
    double minutes, seconds;
    scratch_time.reg = date_time.reg;
    minutes = 60.0 * fmod(rise_set_val, 1);
    seconds = 60.0 * fmod(minutes, 1);
    scratch_time.unit.hour = floor(rise_set_val);
    if (seconds < 30) scratch_time.unit.minute = floor(minutes);
    else scratch_time.unit.minute = ceil(minutes);
    // Handle hour overflow from timezone conversion
    while (scratch_time.unit.hour >= 24) {
        scratch_time.unit.hour -= 24;
        // Increment day (this will be handled by the date arithmetic)
        uint32_t timestamp = watch_utility_date_time_to_unix_time(scratch_time, 0);
        timestamp += 86400;
        scratch_time = watch_utility_date_time_from_unix_time(timestamp, 0);
    }
    if (scratch_time.unit.minute == 60) {
        scratch_time.unit.minute = 0;
        scratch_time.unit.hour = (scratch_time.unit.hour + 1) % 24;
    }
    return scratch_time;
}

static int16_t _compare_dates_times(watch_date_time_t dt1, watch_date_time_t dt2, bool check_date_only) {
    // Returns 0 if they're the same; Positive if dt1 is newer than dt2; Negative o/w
    if (check_date_only) {
        return (dt1.reg >> 12) - (dt2.reg >> 12);
    }
    return (dt1.reg >> 6) - (dt2.reg >> 6);
}

static int8_t _get_if_daytime_result(watch_date_time_t date_time, clock_rise_set_t rise_set_info) {
    // Returns 0 if current time is after sunset; 1 if current time is after sunrise; -1 if predates sunrise;
    int16_t sunset_occurred = _compare_dates_times(date_time, rise_set_info.time_set, false);
    if (sunset_occurred >= 0) return 0;
    int16_t sunrise_occurred = _compare_dates_times(date_time, rise_set_info.time_rise, false);
    if (sunrise_occurred >= 0) return 1;
    return -1;
}

static int8_t _get_if_daytime(watch_date_time_t date_time, clock_rise_set_t *rise_set_info) {
    // 0 is nighttime; 1 is daytime; -1 unknown
    int8_t is_daytime;
    if (rise_set_info->location.reg == 0) {  // No location set
        return -1;
    }
    uint8_t tz_idx = movement_get_timezone_index();
    int8_t date_changed = _compare_dates_times(rise_set_info->time_set, date_time, true);
    if (tz_idx == rise_set_info->tz_idx && date_changed == 0) {
        is_daytime = _get_if_daytime_result(date_time, *rise_set_info);
        return is_daytime == 1;
    }
    rise_set_info->tz_idx = tz_idx;
    int16_t lat_centi = (int16_t)rise_set_info->location.bit.latitude;
    int16_t lon_centi = (int16_t)rise_set_info->location.bit.longitude;
    double lat = (double)lat_centi / 100.0;
    double lon = (double)lon_centi / 100.0;
    int32_t tz = movement_get_current_timezone_offset_for_zone(tz_idx);
    watch_date_time_t utc_now = watch_utility_date_time_convert_zone(date_time, tz, 0); // the current date / time in UTC
    double rise, set;
    uint8_t result = sun_rise_set(utc_now.unit.year + WATCH_RTC_REFERENCE_YEAR, utc_now.unit.month, utc_now.unit.day, lon, lat, &rise, &set);
    if (result != 0) { // Failed to calculate sun rise/set
        return -1;
    }
    double hours_from_utc = ((double)tz) / 3600.0;
    rise += hours_from_utc;
    set += hours_from_utc;
    rise_set_info->time_rise = _get_rise_set_time(rise, date_time);
    rise_set_info->time_set =  _get_rise_set_time(set, date_time);
    is_daytime = _get_if_daytime_result(date_time, *rise_set_info);
    return is_daytime == 1;
}

static void clock_indicate(watch_indicator_t indicator, bool on) {
    if (on) {
        watch_set_indicator(indicator);
    } else {
        watch_clear_indicator(indicator);
    }
}

static void clock_indicate_alarm() {
    clock_indicate(WATCH_INDICATOR_SIGNAL, movement_alarm_enabled());
}

static void clock_indicate_time_signal(clock_state_t *state) {
    clock_indicate(WATCH_INDICATOR_BELL, state->time_signal_enabled);
}

static void clock_indicate_24h() {
    clock_indicate(WATCH_INDICATOR_24H, !!movement_clock_mode_24h());
}

static bool clock_is_pm(watch_date_time_t date_time) {
    return date_time.unit.hour >= 12;
}

static void clock_indicate_pm(watch_date_time_t date_time) {
    if (movement_clock_mode_24h()) { return; }
    clock_indicate(WATCH_INDICATOR_PM, clock_is_pm(date_time));
}

static void clock_indicate_low_available_power(clock_state_t *state) {
    // Set the low battery indicator if battery power is low
    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM) {
        // interlocking arrows imply "exchange" the battery.
        clock_indicate(WATCH_INDICATOR_ARROWS, state->battery_low);
    } else {
        // LAP indicator on classic LCD is an adequate fallback.
        clock_indicate(WATCH_INDICATOR_LAP, state->battery_low);
    }
}

static watch_date_time_t clock_24h_to_12h(watch_date_time_t date_time) {
    date_time.unit.hour %= 12;

    if (date_time.unit.hour == 0) {
        date_time.unit.hour = 12;
    }

    return date_time;
}

static void clock_check_battery_periodically(clock_state_t *state, watch_date_time_t date_time) {
    // check the battery voltage once a day
    if (date_time.unit.day == state->last_battery_check) { return; }

    state->last_battery_check = date_time.unit.day;

    uint16_t voltage = watch_get_vcc_voltage();

    state->battery_low = voltage < CLOCK_FACE_LOW_BATTERY_VOLTAGE_THRESHOLD;

    clock_indicate_low_available_power(state);
}

static void clock_toggle_time_signal(clock_state_t *state) {
    state->time_signal_enabled = !state->time_signal_enabled;
    clock_indicate_time_signal(state);
}

static void clock_display_all(watch_date_time_t date_time) {
    char buf[8 + 1];

    snprintf(
        buf,
        sizeof(buf),
        movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_024H ? "%02d%02d%02d%02d" : "%2d%2d%02d%02d",
        date_time.unit.day,
        date_time.unit.hour,
        date_time.unit.minute,
        date_time.unit.second
    );

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, watch_utility_get_long_weekday(date_time), watch_utility_get_weekday(date_time));
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    watch_display_text(WATCH_POSITION_BOTTOM, buf + 2);
}

static bool clock_display_some(watch_date_time_t current, watch_date_time_t previous) {
    if ((current.reg >> 6) == (previous.reg >> 6)) {
        // everything before seconds is the same, don't waste cycles setting those segments.

        watch_display_character_lp_seconds('0' + current.unit.second / 10, 8);
        watch_display_character_lp_seconds('0' + current.unit.second % 10, 9);

        return true;

    } else if ((current.reg >> 12) == (previous.reg >> 12)) {
        // everything before minutes is the same.

        char buf[4 + 1];

        snprintf(
            buf,
            sizeof(buf),
            "%02d%02d",
            current.unit.minute,
            current.unit.second
        );

        watch_display_text(WATCH_POSITION_MINUTES, buf);
        watch_display_text(WATCH_POSITION_SECONDS, buf + 2);

        return true;

    } else {
        // other stuff changed; let's do it all.
        return false;
    }
}

static void clock_display_clock(clock_state_t *state, watch_date_time_t current) {
    if (!clock_display_some(current, state->date_time.previous)) {
        if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H) {
            clock_indicate_pm(current);
            current = clock_24h_to_12h(current);
        }
        clock_display_all(current);
    }
}

static void clock_display_low_energy(watch_date_time_t date_time) {
    if (movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H) {
        clock_indicate_pm(date_time);
        date_time = clock_24h_to_12h(date_time);
    }
    char buf[8 + 1];

    snprintf(
        buf,
        sizeof(buf),
        movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_024H ? "%02d%02d%02d  " : "%2d%2d%02d  ",
        date_time.unit.day,
        date_time.unit.hour,
        date_time.unit.minute
    );

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, watch_utility_get_long_weekday(date_time), watch_utility_get_weekday(date_time));
    watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    watch_display_text(WATCH_POSITION_BOTTOM, buf + 2);
}

static void clock_toggle_mode_displayed(watch_date_time_t date_time) {
    char buf[2 + 1];
    movement_set_clock_mode_24h(((movement_clock_mode_24h() + 1) % MOVEMENT_NUM_CLOCK_MODES));
    bool in_12h_mode = movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H;
    bool indicate_pm = in_12h_mode && clock_is_pm(date_time);
    if (in_12h_mode) {
        date_time = clock_24h_to_12h(date_time);
    }
    clock_indicate(WATCH_INDICATOR_PM, indicate_pm);
    clock_indicate_24h();
    snprintf(buf, sizeof(buf), movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_024H ? "%02d" : "%2d", date_time.unit.hour);
    watch_display_text(WATCH_POSITION_HOURS, buf);
}

static void clock_start_tick_tock_animation(void) {
    if (!watch_sleep_animation_is_running()) {
        watch_start_indicator_blink_if_possible(WATCH_INDICATOR_COLON, 500);
    }
}

static void clock_stop_tick_tock_animation(void) {
    if (watch_sleep_animation_is_running()) {
        watch_stop_blink();
    }
}

static void display_nighttime(clock_state_t *state, watch_date_time_t date_time) {
    int8_t is_daytime = _get_if_daytime(date_time, &state->rise_set_info);
    if (is_daytime != 0) watch_clear_sleep_indicator_if_possible();
    else watch_set_sleep_indicator_if_possible();
}

void clock_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(clock_state_t));
        memset(*context_ptr, 0, sizeof(clock_state_t));
        clock_state_t *state = (clock_state_t *) *context_ptr;
        state->time_signal_enabled = true;
        state->watch_face_index = watch_face_index;
    }
}

void clock_face_activate(void *context) {
    clock_state_t *state = (clock_state_t *) context;

    clock_stop_tick_tock_animation();

    clock_indicate_time_signal(state);
    clock_indicate_alarm();
    clock_indicate_24h();

    watch_set_colon();

    // this ensures that none of the timestamp fields will match, so we can re-render them all.
    state->date_time.previous.reg = 0xFFFFFFFF;

    if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM ) {
        movement_location_t movement_location = load_location_from_filesystem();
        state->rise_set_info.location.reg = movement_location.reg;
    }
}

bool clock_face_loop(movement_event_t event, void *context) {
    clock_state_t *state = (clock_state_t *) context;
    watch_date_time_t current;

    switch (event.event_type) {
        case EVENT_LOW_ENERGY_UPDATE:
            clock_start_tick_tock_animation();
            clock_display_low_energy(movement_get_local_date_time());
            break;
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            current = movement_get_local_date_time();

            if (watch_get_lcd_type() == WATCH_LCD_TYPE_CUSTOM &&
                (current.reg >> 6) != (state->date_time.previous.reg >> 6)) {
                display_nighttime(state, current);
            }

            clock_display_clock(state, current);

            clock_check_battery_periodically(state, current);

            state->date_time.previous = current;

            break;
        case EVENT_ALARM_LONGER_PRESS:
            if (can_go_to_teriary_face()) {
                clock_toggle_time_signal(state);
                go_to_teriary_face();
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            clock_toggle_time_signal(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (movement_clock_mode_toggle()) {
                clock_toggle_mode_displayed(movement_get_local_date_time());
            }
            break;
        case EVENT_BACKGROUND_TASK:
            // uncomment this line to snap back to the clock face when the hour signal sounds:
            // movement_move_to_face(state->watch_face_index);
            movement_play_signal();
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void clock_face_resign(void *context) {
    (void) context;
}

movement_watch_face_advisory_t clock_face_advise(void *context) {
    movement_watch_face_advisory_t retval = { 0 };
    clock_state_t *state = (clock_state_t *) context;

    if (state->time_signal_enabled) {
        watch_date_time_t date_time = movement_get_local_date_time();
        if (date_time.unit.minute == 0) {
            movement_hourly_chime_t hour_chime_option = movement_get_hourly_chime_times();
            int8_t is_daytime;
            switch (hour_chime_option)
            {
            case MOVEMENT_HC_ALWAYS:
                retval.wants_background_task = true;
                break;
            case MOVEMENT_HC_SUN:
                is_daytime = _get_if_daytime(date_time, &state->rise_set_info);
                if (is_daytime != -1) {
                    retval.wants_background_task = _get_if_daytime(date_time, &state->rise_set_info) == 1;
                    break;
                };
                // fall through
                // if daytime not found to use hardcoded values
            case MOVEMENT_HC_DAYTIME:
                retval.wants_background_task = movement_in_daytime_interval(date_time.unit.hour);
                break;
            default:
                break;
            }
        }
    }

    return retval;
}
