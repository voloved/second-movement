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

#ifndef MOVEMENT_CONFIG_H_
#define MOVEMENT_CONFIG_H_

#include "movement_faces.h"

const watch_face_t watch_faces[] = {
    clock_face,
#if defined(I2C_SERCOM) && !defined(BUILD_TO_SHARE)
    step_counter_face,
#endif
    fast_stopwatch_face,
    countdown_face,
    advanced_alarm_face,
    sunrise_sunset_face,
    tally_face,
    probability_face,
    moon_phase_face,
#ifdef I2C_SERCOM
    lis2dw_monitor_face,
    activity_logging_face,
#endif
// Start of Secondary Faces
    settings_face,
    set_time_face,
    voltage_face,
    temperature_logging_face,
#ifdef I2C_SERCOM
    accelerometer_status_face,
#endif
// Start of Teriary Faces
    endless_runner_face,
    wordle_face,
    blackjack_face,
    higher_lower_game_face,
    lander_face,
    simon_face,
#ifdef BUILD_TO_SHARE
    tarot_face,
#else
    party_face,
    festival_schedule_face,
#endif
};

#define MOVEMENT_NUM_FACES (sizeof(watch_faces) / sizeof(watch_face_t))

/* Determines what face to go to from the first face on long press of the Mode button.
 * Also excludes these faces from the normal rotation.
 * In the default firmware, this lets you access temperature and battery voltage with a long press of Mode.
 * Some folks also like to use this to hide the preferences and time set faces from the normal rotation.
 * If you don't want any faces to be excluded, set this to 0 and a long Mode press will have no effect.
 */

#ifdef BUILD_TO_SHARE
#define MOVEMENT_TERIARY_FACE_INDEX (MOVEMENT_NUM_FACES - 7)
#else
#define MOVEMENT_TERIARY_FACE_INDEX (MOVEMENT_NUM_FACES - 8)
#endif

#ifdef I2C_SERCOM
#define MOVEMENT_SECONDARY_FACE_INDEX (MOVEMENT_TERIARY_FACE_INDEX - 5) // or (0)
#else
#define MOVEMENT_SECONDARY_FACE_INDEX (MOVEMENT_TERIARY_FACE_INDEX - 4) // or (0)
#endif

/* Custom hourly chime tune. Check movement_custom_signal_tunes.h for options. */
#define SIGNAL_TUNE_SONG_OF_THE_STORMS

/* Determines the intensity of the led colors
 * Set a hex value 0-3 with 0x0 being off and 0x3 being max intensity
 */
#define MOVEMENT_DEFAULT_RED_COLOR 0x0
#define MOVEMENT_DEFAULT_GREEN_COLOR 0x3
#define MOVEMENT_DEFAULT_BLUE_COLOR 0x0

/* Set to true for 24h mode or false for 12h mode
 * 0: 12Hr
 * 1: 24Hr
 * 2: Toggle with Alarm Btn
 */
#define MOVEMENT_DEFAULT_24H_MODE 0

/* Enable or disable the sound on mode button press */
#define MOVEMENT_DEFAULT_BUTTON_SOUND false

#ifdef WATCH_BUZZER_IS_BOOSTED
#define MOVEMENT_DEFAULT_BUTTON_VOLUME WATCH_BUZZER_VOLUME_SOFT
#define MOVEMENT_DEFAULT_SIGNAL_VOLUME WATCH_BUZZER_VOLUME_LOUD
#else
#define MOVEMENT_DEFAULT_BUTTON_VOLUME WATCH_BUZZER_VOLUME_LOUD
#define MOVEMENT_DEFAULT_SIGNAL_VOLUME WATCH_BUZZER_VOLUME_LOUD
#endif

/* Set the timeout before switching back to the main watch face
 * Valid values are:
 * 0: 60 seconds
 * 1: 2 minutes
 * 2: 5 minutes
 * 3: 30 minutes
 */
#define MOVEMENT_DEFAULT_TIMEOUT_INTERVAL 0

/* Set the timeout before switching to low energy mode
 * Valid values are:
 * 0: Never
 * 1: 5 seconds
 * 2: 10 minutes
 * 3: 1 hour
 * 4: 6 hours
 * 5: 12 hours
 * 6: 1 day
 * 7: 7 days
 */
#define MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL 2

/*
 * If true and we're in LE mode and it's the top of the hour
 * and the temp is below #DEFAULT_TEMP_ASSUME_WEARING but not zero,
 * then turn off the screen and other tasks.
 * Only runs if Temperature Logging Face is installed.
*/
#ifdef BUILD_TO_SHARE
#define MOVEMENT_DEFAULT_TURN_SCREEN_OFF_AFTER_LE MOVEMENT_LE_SCREEN_OFF_DISABLE
#else
#define MOVEMENT_DEFAULT_TURN_SCREEN_OFF_AFTER_LE MOVEMENT_LE_SCREEN_OFF_ENABLE
#endif

/* Set the led duration
 * Valid values are:
 * 0: No LED
 * 1: 1 second
 * 2: 3 seconds
 * 3: 5 seconds
 */
#define MOVEMENT_DEFAULT_LED_DURATION 1

/* Sets how steps are counted when on the clock_face
 * Valid values are:
 * MOVEMENT_SC_OFF: Don't count steps on clock_face
 * MOVEMENT_SC_ALWAYS: Always count steps on clock_face
 * MOVEMENT_SC_DAYTIME: Count steps between MOVEMENT_STEP_COUNT_START and MOVEMENT_STEP_COUNT_END
 * MOVEMENT_SC_NOT_INSTALLED: The LIS2DW isn't installed (the code handles this without it needing to be manally set)
 */
#define MOVEMENT_DEFAULT_COUNT_STEPS MOVEMENT_SC_OFF

/* If the settings are set to use this start and end hor,
    We only count steps when the step counter face is on.
*/
#define MOVEMENT_STEP_COUNT_START 5
#define MOVEMENT_STEP_COUNT_END 22

/* Set when hourly chiming will occur
 * Valid values are:
 * MOVEMENT_HC_ALWAYS: Always chime
 * MOVEMENT_HC_DAYTIME: begin chiming at MOVEMENT_DAYTIME_START and don't chime at MOVEMENT_DAYTIME_END
 * MOVEMENT_HC_SUN: Chime between sunrise and sunset
 */
#define MOVEMENT_DEFAULT_HOURLY_CHIME MOVEMENT_HC_DAYTIME

#define MOVEMENT_DAYTIME_START 8
#define MOVEMENT_DAYTIME_END 20

#ifndef BUILD_TO_SHARE
/* The latitude and longitude used for the wearers location
 * Set signed values in 1/100ths of a degree
 * Set lat and long for Raleigh (3578, -7864)
 * Double JJ Ranch (4354, -8636)
 */
#define MOVEMENT_DEFAULT_LATITUDE 3578
#define MOVEMENT_DEFAULT_LONGITUDE -7864
#endif

#define MOVEMENT_TEMPERATURE_ASSUME_WEARING 27 //C
#define MOVEMENT_HOURS_BEFORE_DEEPSLEEP 5

/* Optionally debounce button presses (disable by default).
 * A value of 4 is a good starting point if you have issues
 * with multiple button presses firing.
*/
#define MOVEMENT_DEBOUNCE_TICKS 4

#endif // MOVEMENT_CONFIG_H_
