#pragma once
#include "pico/stdlib.h"
#include <stdio.h>

const uint32_t BUZZER_TRIG_PIN = 17;
int tempo = 88;
uint32_t slice_num = 0;
bool pwm_initialized = false;
