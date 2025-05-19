#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "melody.h"
#include "pico/stdlib.h"
#include "stdio.h"
#include <stdlib.h>

#define BUZZER_PIN 17

int tempo = 88;
int buzzer = 11;

void play_tone(uint slice_num, uint chan, uint frequency, uint duration_ms) {
    if (frequency > 0) {
        uint32_t system_clock = clock_get_hz(clk_sys);
        uint32_t divider = system_clock / (frequency * 2000);
        pwm_set_clkdiv(slice_num, divider);
        pwm_set_wrap(slice_num, 2000);
        pwm_set_chan_level(slice_num, chan, 1000); // 50% duty cycle
    } else {
        pwm_set_chan_level(slice_num, chan, 0); // silent
    }
    sleep_ms(duration_ms);
    pwm_set_chan_level(slice_num, chan, 0); // Stop tone
}

int main() {
    stdio_init_all();
    printf("The Legend of Zelda Theme on PICO\n");

    // set buzzerpin as PWN
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint chan = pwm_gpio_to_channel(BUZZER_PIN);

    // PWM activation
    pwm_set_enabled(slice_num, true);

    int notes = sizeof(melody) / sizeof(melody[0]) / 2;
    int wholenote = (60000 * 4) / tempo;

    printf("Playing The Legend of Zelda Theme...\n");
    for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {
        int divider = melody[thisNote + 1];
        int noteDuration = (wholenote) / abs(divider);

        if (divider < 0) {
            noteDuration *= 1.5;
        }

        play_tone(slice_num, chan, melody[thisNote], noteDuration * 0.9);
        sleep_ms(noteDuration * 0.1);
    }

    printf("Melody finished\n");

    while (1) {
        sleep_ms(1000);
    }

    return 0;
}
