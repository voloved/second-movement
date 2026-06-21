Second Movement - Jolt Fork
===============

# Watch Faces

### Main Faces
  - clock_face
  - step_counter_face
  - fast_stopwatch_face
  - countdown_face
  - advanced_alarm_face
  - tally_face
  - sunrise_sunset_face
  - moon_phase_face,
### Start of Secondary Faces
#### Done by holding the MODE button for 0.5 seconds on the clock
  - settings_face
    - Able to turn off the display and chiming, along with set the clock to go into that mode after 5 hours of the watch reading less than 27C (tested to be a good threshold for when the watch is likely not being worn).
  - set_time_face
  - temperature_logging_face
  - voltage_face
  - accelerometer_status_face
### Game Faces
#### Done by pressing START button holding the ALARM button for 1.5 seconds on the clock face
  - black_jack_face
  - endless_runner_face
  - wordle_face
  - higher_lower_game_face
  - lander_face
  - simon_face
  - tarot_face

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

This is the successor refactor of the Movement firmware for [Sensor Watch](https://www.sensorwatch.net).


Getting dependencies
-------------------------
You will need to install [the GNU Arm Embedded Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads/) to build projects for the watch. If you're using Debian or Ubuntu, it should be sufficient to `apt install gcc-arm-none-eabi`.

You will need to fetch the git submodules for this repository too, with `git submodule update --init --recursive` 


Building Your Own Movement
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
- sensorwatch_jolt

and `display_type` is any of:
- classic
- custom
- jolt

If you're using this fork to build for the Jolt PCB (used for the G-Shock), running `make` on its own without selecting a board or diplsay type will work.

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
