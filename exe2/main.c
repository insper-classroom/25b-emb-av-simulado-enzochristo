
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

#define LED_AZUL 9
#define LED_AMARELO 5
#define BTN 28

volatile int clicou;
volatile int estourou_azul = 0, estourou_amarelo = 0;
volatile int alarm_fired = 0;

void btn_callback(uint gpio, uint32_t events){

    if(events == 0x4){
        clicou = !clicou;
    }
}

bool timer_1_callback(repeating_timer_t *rt) {
    estourou_azul = 1;
    return true; // continua repetindo
}

bool timer_2_callback(repeating_timer_t *rt) {
    estourou_amarelo = 1;
    return true; // continua repetindo
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    alarm_fired = 1;
    return 0; // não repete
}

int main() {
    stdio_init_all();
    int ativou_timer = 0, ativou_alarme = 0;
    int led_status_azul = 0, led_status_amarelo = 0;

    repeating_timer_t timer_azul, timer_amarelo;

    gpio_init(LED_AMARELO);
    gpio_init(LED_AZUL);
    gpio_init(BTN);

    gpio_set_dir(LED_AMARELO, GPIO_OUT);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_set_dir(BTN, GPIO_IN);

    gpio_pull_up(BTN);

    gpio_set_irq_enabled_with_callback(BTN, 
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                       true, 
                                       &btn_callback);



    while (true) {

        if(clicou){
            if(!ativou_alarme){
                add_alarm_in_ms(5000, alarm_callback, NULL, false);
                ativou_alarme = 1;
            }

            if(!ativou_timer){

                add_repeating_timer_ms(150, timer_1_callback,NULL, &timer_azul);
                add_repeating_timer_ms(500, timer_2_callback,NULL, &timer_amarelo);
                led_status_amarelo = 1;
                led_status_azul = 1;
                gpio_put(LED_AMARELO, led_status_amarelo);
                gpio_put(LED_AZUL, led_status_azul);
                ativou_timer = 1;
            }

            if(estourou_azul){
                
                printf("estourou azul");
                led_status_azul = !led_status_azul;
                gpio_put(LED_AZUL, led_status_azul);
                estourou_azul = 0;
            }

            if (estourou_amarelo){
                led_status_amarelo = !led_status_amarelo;
                printf("led status amarelo: %d", led_status_amarelo);
                gpio_put(LED_AMARELO, led_status_amarelo);
                estourou_amarelo = 0;   
            }

            if(alarm_fired){

                cancel_repeating_timer(&timer_azul);
                cancel_repeating_timer(&timer_amarelo);

                gpio_put(LED_AMARELO,0);
                gpio_put(LED_AZUL, 0);

                estourou_amarelo = 0;
                estourou_azul = 0;
            }

        }else{

            cancel_repeating_timer(&timer_azul);
            cancel_repeating_timer(&timer_amarelo);

            gpio_put(LED_AMARELO,0);
            gpio_put(LED_AZUL, 0);

            estourou_amarelo = 0;
            estourou_azul = 0;
        }
        
    }

}
