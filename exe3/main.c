/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

SemaphoreHandle_t xSemaphoreLed2;

const int LED_PIN_B = 8;
const int LED_PIN_Y = 13;


typedef struct input {
    int num_led1;
    int num_led2;
} input_t;

QueueHandle_t xQueueInput, xQueueLed1, xQueueLed2;

/**
 * NÃO MEXER!
 */
void input_task(void* p) {
    input_t test_case;

    test_case.num_led1 = 3;
    test_case.num_led2 = 4;
    xQueueSend(xQueueInput, &test_case, 0);

    test_case.num_led1 = 0;
    test_case.num_led2 = 2;
    xQueueSend(xQueueInput, &test_case, 0);

    while (true) {

    }
}

void main_task(void *p){
    input_t input;

    while(1){
        if(xQueueReceive(xQueueInput, &input,portMAX_DELAY)){
            
            xQueueSend(xQueueLed1, &input.num_led1,0);
            xQueueSend(xQueueLed2, &input.num_led2, 0);
            
        }else{
            // printf("erro em receber a fila de input!");
        }
    }
}

void led_1_task(void *p){

    gpio_init(LED_PIN_B);
    gpio_set_dir(LED_PIN_B, GPIO_OUT);
    int num_azul;

    while(1){

        if(xQueueReceive(xQueueLed1, &num_azul, portMAX_DELAY)){
            // printf("num: %d! recebido com sucesso!", input.num_led1);
        }
        for (int i = 0; i<num_azul; i++){
            gpio_put(LED_PIN_B,1);
            vTaskDelay(pdMS_TO_TICKS(250));
            gpio_put(LED_PIN_B,0);
            vTaskDelay(pdMS_TO_TICKS(250));
        }
        
        xSemaphoreGive(xSemaphoreLed2);
    }

}


void led_2_task(void *p){

    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    int num_amarelo;

    while(1){

        if(xSemaphoreTake(xSemaphoreLed2, portMAX_DELAY)){
            
            if(xQueueReceive(xQueueLed2, &num_amarelo, portMAX_DELAY)){
                
                for (int i = 0; i<num_amarelo; i++){
                    gpio_put(LED_PIN_Y,1);
                    vTaskDelay(pdMS_TO_TICKS(250)); // 250 ligado
                gpio_put(LED_PIN_Y,0);
                vTaskDelay(pdMS_TO_TICKS(250)); // 250 desligado
                // periodo 500
                }
            
            }
        }
    }
    
    
}


int main() {
    stdio_init_all();

    /**
     * manter essas duas linhas!
     */
    xQueueInput = xQueueCreate(32, sizeof(input_t));
    xQueueLed1 = xQueueCreate(32, sizeof(int));
    xQueueLed2 = xQueueCreate(32, sizeof(int));

    xTaskCreate(input_task, "Input", 256, NULL, 1, NULL);
    xTaskCreate(main_task, "main", 256, NULL, 1, NULL);
    xTaskCreate(led_1_task, "led1", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "led2", 256, NULL, 1, NULL);

    

    /**
     * Seu código vem aqui!
     */
    xSemaphoreLed2 = xSemaphoreCreateBinary();

    vTaskStartScheduler();

    while (1) {}

    return 0;
}
