#include "melody.h"
#include "pico/stdlib.h" 
#include "hardware/pwm.h" 
#include "hardware/gpio.h" 
#include "hardware/clocks.h" 
#include <stdlib.h>   
#include "pico/time.h"

int tempo = 88;               
uint32_t slice_num = 0;      
bool pwm_initialized = false;
/*
int melody[] = {
        NOTE_AS4,-2,  NOTE_F4,8,  NOTE_F4,8,  NOTE_AS4,8,//1
        NOTE_GS4,16,  NOTE_FS4,16,  NOTE_GS4,-2,
        NOTE_AS4,-2,  NOTE_FS4,8,  NOTE_FS4,8,  NOTE_AS4,8,
        NOTE_A4,16,  NOTE_G4,16,  NOTE_A4,-2,
        REST,1,

        NOTE_AS4,4,  NOTE_F4,-4,  NOTE_AS4,8,  NOTE_AS4,16,  NOTE_C5,16, NOTE_D5,16, NOTE_DS5,16,//7
        NOTE_F5,2,  NOTE_F5,8,  NOTE_F5,8,  NOTE_F5,8,  NOTE_FS5,16, NOTE_GS5,16,
        NOTE_AS5,-2,  NOTE_AS5,8,  NOTE_AS5,8,  NOTE_GS5,8,  NOTE_FS5,16,
        NOTE_GS5,-8,  NOTE_FS5,16,  NOTE_F5,2,  NOTE_F5,4,

        NOTE_DS5,-8, NOTE_F5,16, NOTE_FS5,2, NOTE_F5,8, NOTE_DS5,8, //11
        NOTE_CS5,-8, NOTE_DS5,16, NOTE_F5,2, NOTE_DS5,8, NOTE_CS5,8,
        NOTE_C5,-8, NOTE_D5,16, NOTE_E5,2, NOTE_G5,8,
        NOTE_F5,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,8, NOTE_F4,16,NOTE_F4,8,

        NOTE_AS4,4,  NOTE_F4,-4,  NOTE_AS4,8,  NOTE_AS4,16,  NOTE_C5,16, NOTE_D5,16, NOTE_DS5,16,//15
        NOTE_F5,2,  NOTE_F5,8,  NOTE_F5,8,  NOTE_F5,8,  NOTE_FS5,16, NOTE_GS5,16,
        NOTE_AS5,-2, NOTE_CS6,4,
        NOTE_C6,4, NOTE_A5,2, NOTE_F5,4,
        NOTE_FS5,-2, NOTE_AS5,4,
        NOTE_A5,4, NOTE_F5,2, NOTE_F5,4,

        NOTE_FS5,-2, NOTE_AS5,4,
        NOTE_A5,4, NOTE_F5,2, NOTE_D5,4,
        NOTE_DS5,-2, NOTE_FS5,4,
        NOTE_F5,4, NOTE_CS5,2, NOTE_AS4,4,
        NOTE_C5,-8, NOTE_D5,16, NOTE_E5,2, NOTE_G5,8,
        NOTE_F5,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,16,NOTE_F4,8, NOTE_F4,16,NOTE_F4,8
    };
void play_tone(uint32_t slice_num, uint8_t chan, uint32_t frequency, uint duration_ms) {
    if (frequency > 0) {
        uint32_t system_clock = clock_get_hz(clk_sys);
        uint32_t divider = system_clock / (frequency * 2000);
        pwm_set_clkdiv(slice_num, divider);
        pwm_set_wrap(slice_num, 2000);
        pwm_set_chan_level(slice_num, chan, 1000);
    } else {
        pwm_set_chan_level(slice_num, chan, 0); // silent
    }
}

static alarm_id_t stop_tone_callback(alarm_id_t id, void *user_data) {
    pwm_set_chan_level(buzzer_slice_num, buzzer_chan, 0);
    return 0; 
}
static bool play_next_note_callback(struct repeating_timer *t) {
    if (!buzzer_active) {
        cancel_repeating_timer(&melody_timer);
        note_index = 0;
        melody_playing = false; // Uppdatera flaggan när uppspelningen stoppas
        return false;
    }

    int notes_in_melody = sizeof(melody) / sizeof(melody[0]) / 2;
    if (note_index < notes_in_melody * 2) {
        int divider = melody[note_index + 1];
        int noteDuration = (wholenote) / abs(divider);
        if (divider < 0) {
            noteDuration *= 1.5;
        }


	play_tone(buzzer_slice_num, buzzer_chan, melody[note_index], noteDuration * 0.9);
        add_alarm_in_ms(noteDuration * 0.9, (alarm_callback_t)stop_tone_callback, NULL, false);

        note_index += 2;
        return true;
    } else {
        buzzer_active = false;
        melody_playing = false; // Uppdatera flaggan när melodin är slut
        note_index = 0;
        return false;
    }
}
void play_melody_async() {
    note_index = 0;
    int notes_in_melody = sizeof(melody) / sizeof(melody[0]) / 2;
    wholenote = (60000 * 4) / tempo;

    add_repeating_timer_ms(1, play_next_note_callback, NULL, &melody_timer);
    play_next_note_callback(NULL); // Spela första noten omedelbart
    melody_playing = true;
}
// buzzern init
void init_buzzer() {
    gpio_set_function(BUZZER_TRIG_PIN, GPIO_FUNC_PWM);
    buzzer_slice_num = pwm_gpio_to_slice_num(BUZZER_TRIG_PIN);
    buzzer_chan = pwm_gpio_to_channel(BUZZER_TRIG_PIN);
    pwm_set_enabled(buzzer_slice_num, true);
    pwm_set_chan_level(buzzer_slice_num, buzzer_chan, 0); 
}
*/
