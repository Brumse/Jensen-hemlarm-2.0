#pragma once

#include <stdio.h>
/*
NOTES for adding functions:
        add arlarmsound when trigger alarm.
        add button to reset alarm.
*/

const uint TRIG_PIN = 2;
const uint ECHO_PIN = 3;
const uint LED_PIN = 15;
const uint CLEAR_BUTTON_PIN = 16;
// set threshold for alarm
const float ALARM_THRESHOLD = 10.0f;
