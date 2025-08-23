/*
 * MIT License
 *
 * Copyright (c) 2025 Joey Castillo
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
#include "settings_face.h"
#include "watch.h"

static void clock_setting_display(uint8_t subsecond) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "CLOCK", "CL");
    if (subsecond % 2) {
        if (movement_clock_mode_toggle()) watch_display_text(WATCH_POSITION_BOTTOM, "Btn");
        else if (movement_clock_mode_24h()) watch_display_text(WATCH_POSITION_BOTTOM, "24h");
        else watch_display_text(WATCH_POSITION_BOTTOM, "12h");
    }
}

static void clock_setting_advance(void) {
    if (movement_clock_mode_toggle()) {
        movement_set_clock_mode_toggle(false);
        movement_set_clock_mode_24h(MOVEMENT_CLOCK_MODE_12H);
        return;
    }
    movement_clock_mode_t next_mode = !movement_clock_mode_24h();
    // movement_clock_mode_t next_mode = (movement_clock_mode_24h() + 1) % MOVEMENT_NUM_CLOCK_MODES;
    // Uncomment the above line if you're using leading zeroes....you cretin.
    if (next_mode == 0)
        movement_set_clock_mode_toggle(true);
    movement_set_clock_mode_24h(next_mode);
}

static void beep_btn_setting_display(uint8_t subsecond) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "BTN", "BT");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "beep  ", " beep ");
    if (subsecond % 2) {
        if (movement_button_should_sound()) {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " Y");
        } else {
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " N");
        }
    }
}

static void beep_btn_setting_advance(void) {
    if (!movement_button_should_sound())
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    movement_set_button_should_sound(!movement_button_should_sound());
    beep_btn_setting_display(1);
}

static void beep_vol_setting_display(uint8_t subsecond) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "VOL", "VO");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "beep  ", " beep ");
    if (subsecond % 2) {
        if (movement_button_volume() == WATCH_BUZZER_VOLUME_LOUD) {
            // H for HIGH
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " H");
        }
        else {
            // L for LOW
            watch_display_text(WATCH_POSITION_TOP_RIGHT, " L");
        }
    }
}

static void beep_vol_setting_advance(void) {
    watch_buzzer_volume_t volume = !movement_button_volume();
    movement_set_button_volume(volume);
    watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, volume);
    beep_vol_setting_display(1);
}

static void timeout_setting_display(uint8_t subsecond) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "TMOUt", "TO");
    if (subsecond % 2) {
        switch (movement_get_fast_tick_timeout()) {
            case 0:
                watch_display_text(WATCH_POSITION_BOTTOM, "60 SeC");
                break;
            case 1:
                watch_display_text(WATCH_POSITION_BOTTOM, "2 n&in");
                break;
            case 2:
                watch_display_text(WATCH_POSITION_BOTTOM, "5 n&in");
                break;
            case 3:
                watch_display_text(WATCH_POSITION_BOTTOM, "30n&in");
                break;
        }
    }
}

static void timeout_setting_advance(void) {
    movement_set_fast_tick_timeout((movement_get_fast_tick_timeout() + 1));
}

static void low_energy_setting_display(uint8_t subsecond) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "LoEne", "LE");
    if (subsecond % 2) {
        switch (movement_get_low_energy_timeout()) {
            case 0:
                watch_display_text(WATCH_POSITION_BOTTOM, " Never");
                break;
            case 1:
                watch_display_text(WATCH_POSITION_BOTTOM, " 5 sec");
                break;
            case 2:
                watch_display_text(WATCH_POSITION_BOTTOM, "10n&in");
                break;
            case 3:
                watch_display_text(WATCH_POSITION_BOTTOM, "1 hour");
                break;
            case 4:
                watch_display_text(WATCH_POSITION_BOTTOM, "6 hour");
                break;
            case 5:
                watch_display_text(WATCH_POSITION_BOTTOM, "12 hr");
                break;
            case 6:
                watch_display_text(WATCH_POSITION_BOTTOM, " 1 day");
                break;
            case 7:
                watch_display_text(WATCH_POSITION_BOTTOM, " 7 day");
                break;
        }
    }
}

static void low_energy_setting_advance(void) {
    movement_set_low_energy_timeout((movement_get_low_energy_timeout() + 1));
}

static void low_energy_deep_sleep_setting_display(uint8_t subsecond) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "DPS", "LE");
    watch_display_text_with_fallback(WATCH_POSITION_TOP_RIGHT, "LP", "ds");
    if (subsecond % 2) {
        switch (movement_get_low_energy_screen_off_setting()) {
            case MOVEMENT_LE_SCREEN_OFF_DISABLE:
                watch_display_text(WATCH_POSITION_BOTTOM, "   OFF");
                break;
            case MOVEMENT_LE_SCREEN_OFF_ENABLE:
                watch_display_text(WATCH_POSITION_BOTTOM, "   ON ");
                break;
            case MOVEMENT_LE_SCREEN_OFF_NOW:
                watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "   Now", "  Nowj");
                break;
            default:
                break;
        }
    }
    else {
        watch_display_text(WATCH_POSITION_BOTTOM, "      ");
    }
}

static void low_energy_deep_sleep_setting_advance(void) {
    movement_low_energy_screen_off_t next_mode = (movement_get_low_energy_screen_off_setting() + 1) % MOVEMENT_LE_SCREEN_OFF_MODES;
    movement_set_low_energy_screen_off_setting(next_mode);
}

static void led_duration_setting_display(uint8_t subsecond) {
    char buf[8];

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LED", "LT");
    if (subsecond % 2) {
        if (movement_get_backlight_dwell() == 0) {
            watch_display_text(WATCH_POSITION_BOTTOM, "instnt");
        } else if (movement_get_backlight_dwell() == 0b111) {
            watch_display_text(WATCH_POSITION_BOTTOM, "no LEd");
        } else {
            sprintf(buf, " %1d SeC", (movement_get_backlight_dwell() * 2 - 1) % 10);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
        }
    }
}

static void led_duration_setting_advance(void) {
    movement_set_backlight_dwell(movement_get_backlight_dwell() + 1);
    if (movement_get_backlight_dwell() > 3) {
        // set all bits to disable the LED
        movement_set_backlight_dwell(0b111);
    }
}

static void red_led_setting_display(uint8_t subsecond) {
    char buf[8];
    movement_color_t color = movement_backlight_color();

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LED", "LT");
    watch_display_text(WATCH_POSITION_BOTTOM, " red  ");
    if (subsecond % 2) {
        sprintf(buf, "%2d", color.red);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
}

static void red_led_setting_advance(void) {
    movement_color_t color = movement_backlight_color();
    color.red++;
    movement_set_backlight_color(color);
}

static void green_led_setting_display(uint8_t subsecond) {
    char buf[8];
    movement_color_t color = movement_backlight_color();

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LED", "LT");
    watch_display_text(WATCH_POSITION_BOTTOM, " green");
    if (subsecond % 2) {
        sprintf(buf, "%2d", color.green);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
}

static void green_led_setting_advance(void) {
    movement_color_t color = movement_backlight_color();
    color.green++;
    movement_set_backlight_color(color);
}

static void blue_led_setting_display(uint8_t subsecond) {
    char buf[8];
    movement_color_t color = movement_backlight_color();

    watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "LED", "LT");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "blue  ", " blue ");
    if (subsecond % 2) {
        sprintf(buf, "%2d", color.blue);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    }
}

static void blue_led_setting_advance(void) {
    movement_color_t color = movement_backlight_color();
    color.blue++;
    movement_set_backlight_color(color);
}

#ifdef BUILD_GIT_HASH
static void  git_hash_setting_display(uint8_t subsecond) {
    (void) subsecond;
    char buf[8];
    // BUILD_GIT_HASH will already be truncated to 6 characters in the makefile, but this is to be safe.
    sprintf(buf, "%.6s", BUILD_GIT_HASH);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "Bu{d ", "bU");
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

static void git_hash_setting_advance(void) {
    return;
}
#endif

void settings_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(settings_state_t));
        settings_state_t *state = (settings_state_t *)*context_ptr;
        int8_t current_setting = 0;
        state->current_page = 0;
        state->retain_curr_pos = false;
        state->num_settings = 7; // baseline, without LED settings
#ifdef BUILD_GIT_HASH
        state->num_settings++;
#endif
#ifdef WATCH_RED_TCC_CHANNEL
        state->num_settings++;
#endif
#ifdef WATCH_GREEN_TCC_CHANNEL
        state->num_settings++;
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
        state->num_settings++;
#endif

        state->settings_screens = malloc(state->num_settings * sizeof(settings_screen_t));
        state->settings_screens[current_setting].display = beep_btn_setting_display;
        state->settings_screens[current_setting].advance = beep_btn_setting_advance;
        current_setting++;
        state->settings_screens[current_setting].display = beep_vol_setting_display;
        state->settings_screens[current_setting].advance = beep_vol_setting_advance;
        current_setting++;
        state->settings_screens[current_setting].display = clock_setting_display;
        state->settings_screens[current_setting].advance = clock_setting_advance;
        current_setting++;
        state->settings_screens[current_setting].display = timeout_setting_display;
        state->settings_screens[current_setting].advance = timeout_setting_advance;
        current_setting++;
#ifndef MOVEMENT_LOW_ENERGY_MODE_FORBIDDEN
        state->settings_screens[current_setting].display = low_energy_setting_display;
        state->settings_screens[current_setting].advance = low_energy_setting_advance;
        current_setting++;
        state->settings_screens[current_setting].display = low_energy_deep_sleep_setting_display;
        state->settings_screens[current_setting].advance = low_energy_deep_sleep_setting_advance;
        state->screen_off_screen = current_setting;
        current_setting++;
#endif
        state->settings_screens[current_setting].display = led_duration_setting_display;
        state->settings_screens[current_setting].advance = led_duration_setting_advance;
        current_setting++;
        state->led_color_start = current_setting;
#ifdef WATCH_RED_TCC_CHANNEL
        state->settings_screens[current_setting].display = red_led_setting_display;
        state->settings_screens[current_setting].advance = red_led_setting_advance;
        current_setting++;
#else
        (void)red_led_setting_display;
        (void)red_led_setting_advance;
#endif
#ifdef WATCH_GREEN_TCC_CHANNEL
        state->settings_screens[current_setting].display = green_led_setting_display;
        state->settings_screens[current_setting].advance = green_led_setting_advance;
        current_setting++;
#else
        (void)green_led_setting_display;
        (void)green_led_setting_advance;
#endif
#ifdef WATCH_BLUE_TCC_CHANNEL
        state->settings_screens[current_setting].display = blue_led_setting_display;
        state->settings_screens[current_setting].advance = blue_led_setting_advance;
        current_setting++;
#else
        (void)blue_led_setting_display;
        (void)blue_led_setting_advance;
#endif
    state->led_color_end = current_setting;
#ifdef BUILD_GIT_HASH
        state->settings_screens[current_setting].display = git_hash_setting_display;
        state->settings_screens[current_setting].advance = git_hash_setting_advance;
        current_setting++;
#endif
    }
}

void settings_face_activate(void *context) {
    settings_state_t *state = (settings_state_t *)context;
    if (!state->retain_curr_pos || state->current_page >= state->num_settings) {
        state->current_page = 0;
    }
    state->retain_curr_pos = false;
    movement_request_tick_frequency(4); // we need to manually blink some pixels
}

bool settings_face_loop(movement_event_t event, void *context) {
    settings_state_t *state = (settings_state_t *)context;

    switch (event.event_type) {
        case EVENT_LIGHT_BUTTON_DOWN:
            if (movement_get_low_energy_screen_off_setting() == MOVEMENT_LE_SCREEN_OFF_NOW 
                && state->current_page == state->screen_off_screen) {
                state->retain_curr_pos = true;
                movement_request_deep_sleep_on_next_tick();
            }else {
                state->current_page = (state->current_page + 1) % state->num_settings;
            }
            // fall through
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            watch_clear_display();
            state->settings_screens[state->current_page].display(event.subsecond);
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_force_led_off();
            movement_move_to_next_face();
            return true;
        case EVENT_ALARM_BUTTON_UP:
            state->settings_screens[state->current_page].advance();
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    if (state->current_page >= state->led_color_start && state->current_page < state->led_color_end) {
        movement_color_t color = movement_backlight_color();
        movement_force_led_on(movement_get_color_val(color.red),
                              movement_get_color_val(color.green),
                              movement_get_color_val(color.blue));
        return false;
    } else {
        movement_force_led_off();
        return true;
    }
}

void settings_face_resign(void *context) {
    (void) context;
    movement_force_led_off();
    movement_store_settings();
}
