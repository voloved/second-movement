Second Movement - Devolov Fork
===============

# Watch Faces

### Main Faces
  - clock_face
    - Chiming only occurs from 8am to 8pm
    - Able to change 12h and 24h with the alarm button
      - This option is enabled in the settings
  - fast_stopwatch_face
    - LIGhT button doesn't turn off LED by default.
  - countdown_face
    - Default is 5 minutes, not 3.
  - advanced_alarm_face
    - UI much more like Casio Square.
    - Made setting where alarm does not go off on USA holidays.
  - tally_face
    - Able to toggle through the preset starting value of 20 and 40 (for all of my Magic players)
  - sunrise_sunset_face
    - Same as default
  - moon_phase_face,
    - [Able to go back in days by clicking the LIGHT button.](https://github.com/joeycastillo/second-movement/pull/99)
  - activity_logging_face
    - [Able to go back in days by clicking the LIGHT button.](https://github.com/joeycastillo/second-movement/pull/99)
### Start of Secondary Faces
#### Done by holding the MODE button for 0.5 seconds on the clock
  - settings_face
    - LED only has 7 brightness options per color.
      - Max brightness is the same, the granularity of the brightness is just half as big.
    - Able to choose btn in the hours display mode to toggle 12h and 24h in the clock face..
    - Able to turn off the display and chiming, along with set the clock to go into that mode after 5 hours of the watch reading less than 27C (tested to be a good threshold for when the watch is likely not being worn).
    - SHows the beep option first.
  - set_time_face
    - SHows hrs/min/sec beforeshowing the day/mo/yr.
    - Shows the timezone text only.
      - Holding the LIGHT button changes the text to the offset.
      - THe index of the timezone is on the top-right to make searching for your zone easier if you pass it.
  - temperature_logging_face
    - [Merged with temperature_display_face](https://github.com/joeycastillo/second-movement/pull/97)
  - voltage_face
    - [Added voltage logging](https://github.com/joeycastillo/second-movement/pull/97)
  - accelerometer_status_face
    - No changes from default.
### Game Faces
#### Done by holding the ALARM button for 1.5 seconds on the clock face
  - endless_runner_face
    - [Includes everything in this PR.](https://github.com/joeycastillo/second-movement/pull/86)
  - wordle_face
    - Holding the ALARM button for 1.5 seconds choses a good first-word if it's your first guess and you're on the first character.
  - higher_lower_game_face
    - [Includes everything in this PR](https://github.com/joeycastillo/second-movement/pull/87)
    - Getting the same number counts to your score.
  - lander_face
    - [Inclides everything in this PR](https://github.com/joeycastillo/second-movement/pull/88)
  - tarot_face
    - Retains changed settings between uses](https://github.com/joeycastillo/second-movement/pull/100)

# Other Changes
  - [Uses COUNTER32 logic.](https://github.com/joeycastillo/second-movement/pull/65)
  - When the watch wakes to chime, we don't display the seconds on the clock.
  - Chime sound is the beginning of Song of the Storm from Zelda: Ocarina of Time.
  - No ticking animation and all faces show SLEEP indicator when in sleep mode.
  - Added debouncing.
  - Made DST logic need far fewer caching.
  - B and I look like 8 and 1 on the bottom of the face when not in seconds.
  - Added a seriff to 7.
  - [Improved off-axis viewing on custom LCD.](https://github.com/joeycastillo/second-movement/pull/79)
  - 

===============

This is a work-in-progress refactor of the Movement firmware for [Sensor Watch](https://www.sensorwatch.net).


Getting dependencies
-------------------------
You will need to install [the GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/) to build projects for the watch. If you're using Debian or Ubuntu, it should be sufficient to `apt install gcc-arm-none-eabi`.

You will need to fetch the git submodules for this repository too, with `git submodule update --init --recursive` 


Building Second Movement
----------------------------
You can build the default watch firmware with:

```
make BOARD=board_type DISPLAY=display_type
```

where `board_type` is any of:
- sensorwatch_pro
- sensorwatch_green  
- sensorwatch_red (also known as Sensor Watch Lite)
- sensorwatch_blue

and `display_type` is any of:
- classic
- custom

Optionally you can set the watch time when building the firmware using `TIMESET=minute`. 

`TIMESET` can be defined as:
- `year` = Sets the year to the PC's
- `day` = Sets the default time down to the day (year, month, day)
- `minute` = Sets the default time down to the minute (year, month, day, hour, minute)


If you'd like to modify which faces are built and included in the firmware, edit `movement_config.h`. You will get a compilation error if you enable more faces than the watch can store.

Installing firmware to the watch
----------------------------
To install the firmware onto your Sensor Watch board, plug the watch into your USB port and double tap the tiny Reset button on the back of the board. You should see the LED light up red and begin pulsing. (If it does not, make sure you didn’t plug the board in upside down). Once you see the `WATCHBOOT` drive appear on your desktop, type `make install`. This will convert your compiled program to a UF2 file, and copy it over to the watch.

If you want to do this step manually, copy `/build/firmware.uf2` to your watch. 


Emulating the firmware
----------------------------
You may want to test out changes in the emulator first. To do this, you'll need to install [emscripten](https://emscripten.org/), then run:

```
emmake make BOARD=sensorwatch_red DISPLAY=classic
python3 -m http.server -d build-sim
```

Finally, visit [firmware.html](http://localhost:8000/firmware.html) to see your work.
