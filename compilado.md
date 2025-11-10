# Guia de Estudos - Computação Embarcada
## Raspberry Pi Pico - GPIO e Programação em C

---

## 1. CONCEITOS FUNDAMENTAIS

### GPIO (General Purpose Input/Output)
Usado sempre que quisermos controlar ou ler o estado de um pino digital por meio de código.

**Sinal digital:** Aquele que pode ser interpretado como 0 ou 1.

**Atuadores:** Tudo aquilo que o microcontrolador consegue controlar
**Sensores:** Tudo que gera informações para o controlador.

### Especificações da Raspberry Pi Pico
- **Tensão máxima:** 3.3V por pino
- **Amperagem máxima:** 50mA por pino de entrada/saída

---

## 2. CONFIGURAÇÃO BÁSICA DE GPIO

### Inicialização e Configuração

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int LED_PIN = 5; // definir o pino

// Inicializar o pino
gpio_init(LED_PIN);

// Definir direção
gpio_set_dir(PIN, GPIO_OUT); // Saída
gpio_set_dir(PIN, GPIO_IN);  // Entrada
```

**Importante:** Entrada e saída sempre referentes ao componente. Entrada se os fios estiverem chegando na entrada do componente. Saída se o fio estiver saindo do componente.

### Operações com GPIO

**Para SAÍDA:**
```c
gpio_put(PIN, 1); // ativar
gpio_put(PIN, 0); // desativar
```

**Para ENTRADA:**
```c
int status = gpio_get(PIN); // lê o valor que está sendo inputado no pino
```

---

## 3. EXEMPLOS PRÁTICOS

### LED (Saída)
```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int LED_PIN = 5;

int main() {
    stdio_init_all();

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
    }
}
```

### Sensor PIR (Entrada sem pull-up)
```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int PIR_PIN = 3; 

int main() {
    stdio_init_all();

    gpio_init(PIR_PIN);
    gpio_set_dir(PIR_PIN, GPIO_IN);
    //gpio_pull_up(PIR_PIN); // Não usado para PIR

    while (true) {
        if (gpio_get(PIR_PIN))
            printf("Presença detectada \n");
    }
}
```

### Botão (Entrada com pull-up)
```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

const int BTN_PIN = 26; 

int main() {
   stdio_init_all();

   gpio_init(BTN_PIN);
   gpio_set_dir(BTN_PIN, GPIO_IN);
   gpio_pull_up(BTN_PIN);

   while (true) {
       if (!gpio_get(BTN_PIN))  // Note o '!' (negação)
           printf("Botão apertado! \n");
   }
}
```

**Por que usar `!gpio_get(BTN_PIN)`?**
- Com pull_up, o valor fica "setado" sempre com valor 1 (botão em repouso)
- Quando clicamos, ele vai para valor 0 (botão pressionado)
- Em alta: `1` (Botão em repouso) → `!1` = 0, NÃO entra no if
- Em baixa: `0` (Clicou no botão) → `!0` = 1, ENTRA no if

---

## 4. CONTROLE DE ESTADOS

### Exemplo de Toggle com Botão
```c
if(!gpio_get(PIN_GREEN_BUTTON)){
    if(estado_verde){
        gpio_put(PIN_GREEN_LED, estado_verde);
        estado_verde = false;
    } else {
        gpio_put(PIN_GREEN_LED, estado_verde);
        estado_verde = true;
    }
    while(!gpio_get(PIN_GREEN_BUTTON)){
        // fica dentro do while até que a condição seja falsa
    }     
}
```

**Estado booleano:** Garante que o código vai ficar alterando efetivamente de estado, sendo uma forma fácil e efetiva de fazer o controle.

---

## 5. INTERRUPÇÕES (IRQ - Interruption Request)

### Conceitos Fundamentais
- **IRQ:** Nome dado quando uma interrupção é requisitada pelo hardware
- Permite que o hardware notifique o software sobre um evento que necessita de atenção imediata
- Após a interrupção, o programa volta ao estado normal de onde foi interrompido

### Tipo Volatile
**REGRA IMPORTANTE:** Todas as variáveis que são de interrupção TEM que ser do tipo `volatile`.

```c
volatile char g_but_flag; // variável global para IRQ
```

### Bordas (Edges)
Referem-se a mudanças no estado de um sinal elétrico:

- **`GPIO_IRQ_EDGE_FALL`**: Descida (Botão apertado) - Fall edge
  - event: `0x04` (0000 0100 em binário)
  
- **`GPIO_IRQ_EDGE_RISE`**: Subida (Botão solto) - Rise edge  
  - event: `0x08` (0000 1000 em binário)
  
- **Ambas:** `GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE`
  - event: `0x04 + 0x08`

### Exemplo Básico de IRQ
```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LED_PIN 5 
#define BTN_PIN 26 

volatile int g_status = 0;

void gpio_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {         // fall edge
        g_status = 1;
    } else if (events == 0x8) {  // rise edge
        // ação para rise edge
    }
}

int main() {
    stdio_init_all();

    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);
    gpio_set_irq_enabled_with_callback(BTN_PIN, 
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                       true, 
                                       &gpio_callback);
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    int led_status = 0;
    while (true) {
        if (g_status){
            g_status = 0; // limpa flag da IRQ
            led_status = !led_status;
            gpio_put(LED_PIN, led_status);
        }
    }
}
```

### Múltiplos Callbacks
Só podemos fazer um único callback para os GPIOs da Pico. Devemos usar a informação do argumento `gpio` para saber qual pino estamos lidando dentro do callback.

```c
const int BTN_PIN_R = 28; 
const int BTN_PIN_G = 26; 

volatile int g_flag_r = 0;
volatile int g_flag_g = 0;

void btn_callback(uint gpio, uint32_t events) {
    if (gpio == BTN_PIN_R) {     
        g_flag_r = 1;
    } else if (gpio == BTN_PIN_G) {
        g_flag_g = 1;
    }
}

int main() {
    // ... inicializações ...

    // Primeiro callback define a função
    gpio_set_irq_enabled_with_callback(BTN_PIN_R,
                                       GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &btn_callback);
    
    // Segunda IRQ usa callback já configurado
    gpio_set_irq_enabled(BTN_PIN_G,
                         GPIO_IRQ_EDGE_FALL,
                         true);

    while (true) {
        if(g_flag_r || g_flag_g) {
            printf("IRQ 0: %d | IRQ 1: %d\n", g_flag_r, g_flag_g);
            
            // limpa flags
            g_flag_r = 0;
            g_flag_g = 0;
        }
    }
}
```

---

## 6. TIMERS

### Tempo Absoluto
Base de tempo fornecida pelo periférico do Timer, independente do programa:

```c
#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    while(1){
        uint32_t start_ms = to_ms_since_boot(get_absolute_time());
        uint64_t start_us = to_us_since_boot(get_absolute_time());
        sleep_ms(100);
    }
}
```

### Timer Repetitivo
Cria um timer que chama uma função de callback a cada `x` Hz:

```c
#include <stdio.h>
#include "pico/stdlib.h"

volatile int g_timer_0 = 0;

bool timer_0_callback(repeating_timer_t *rt) {
    g_timer_0 = 1;
    return true; // continua repetindo
}

int main() {
    stdio_init_all();

    int timer_0_hz = 5;
    repeating_timer_t timer_0;

    if (!add_repeating_timer_us(1000000 / timer_0_hz, 
                                timer_0_callback,
                                NULL, 
                                &timer_0)) {
        printf("Failed to add timer\n");
    }

    while(1){
        if(g_timer_0){
            printf("Hello timer 0 \n");
            g_timer_0 = 0;
        }
    }
}
```

**Para cancelar:** `cancel_repeating_timer(&timer)`

### Alarm (Evento Único)
Usado para gerar um evento não periódico, disparado uma única vez:

```c
#include <stdio.h>
#include "pico/stdlib.h"

volatile bool timer_fired = false;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    timer_fired = true;
    return 0; // não repete
}

int main() {
    stdio_init_all();

    // Chama alarm_callback em 300 ms
    alarm_id_t alarm = add_alarm_in_ms(300, alarm_callback, NULL, false);

    if (!alarm) {
        printf("Failed to add timer\n");
    }

    while(1){
        if(timer_fired){
            timer_fired = 0;
            printf("Hello from alarm!");
        }
    }
}
```

---

## 7. RTC (REAL TIME CLOCK)

### API Principal
- `void rtc_init(void)`: Inicializa o sistema RTC
- `bool rtc_set_datetime(datetime_t *t)`: Define o RTC para o tempo especificado
- `bool rtc_get_datetime(datetime_t *t)`: Obtém o tempo atual do RTC
- `bool rtc_running(void)`: O RTC está em execução?
- `void rtc_set_alarm(datetime_t *t, rtc_callback_t user_callback)`: Define alarme
- `void rtc_enable_alarm(void)`: Ativa o alarme do RTC
- `void rtc_disable_alarm(void)`: Desativa o alarme do RTC

### Configuração CMake
```cmake
target_link_libraries(main pico_stdlib hardware_rtc)
```

### Includes Necessários
```c
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
```

### Exemplo com Alarme
```c
#include <stdio.h>
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

static volatile bool fired = false;

static void alarm_callback(void) {
    fired = true;
}

int main() {
    stdio_init_all();
    printf("RTC Alarm Repeat!\n");

    // Inicia na quarta-feira 13 de janeiro 2021 11:20:00
    datetime_t t = {
        .year  = 2020,
        .month = 01,
        .day   = 13,
        .dotw  = 3, // 0 é domingo, então 3 é quarta-feira
        .hour  = 11,
        .min   = 20,
        .sec   = 00
    };

    // Inicia o RTC
    rtc_init();
    rtc_set_datetime(&t);

    // Alarme a cada minuto
    datetime_t alarm = {
        .year  = -1,
        .month = -1,
        .day   = -1,
        .dotw  = -1,
        .hour  = -1,
        .min   = -1,
        .sec   = 00  // dispara quando os segundos chegarem a 00
    };

    rtc_set_alarm(&alarm, &alarm_callback);

    while(1){
        if (fired) {
            fired = 0;
            datetime_t t = {0};
            rtc_get_datetime(&t);
            char datetime_buf[256];
            char *datetime_str = &datetime_buf[0];
            datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
            printf("Alarm Fired At %s\n", datetime_str);
        }
    }
}
```

---

## 8. FREERTOS - SISTEMA OPERACIONAL REAL-TIME

### Tasks
São funções que **não** retornam nada e **possuem** laço infinito.

```c
void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    int delay = 250;
    while (true) {
        gpio_put(LED_PIN_R, 1);
        vTaskDelay(pdMS_TO_TICKS(delay));
        gpio_put(LED_PIN_R, 0);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}
```

### vTaskDelay
A função `vTaskDelay()` faz com que o RTOS libere processamento para outras tarefas durante o tempo especificado em ticks.

### Criando Tasks
Usamos `xTaskCreate` na `main`:

```c
int main() {
    stdio_init_all();
    printf("Start RTOS \n");
    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);
    vTaskStartScheduler();
    
    // Programa nunca deve chegar aqui!
    while (true);
}
```

### Parâmetros de xTaskCreate:
- **TaskFunction_t pvTaskCode:** Qual a função da task
- **const char const pcName:** String que representa o nome da tarefa
- **uint16_t usStackDepth:** Tamanho da pilha atribuída à tarefa
- **void pvParameters*:** Ponteiro para parâmetros da tarefa
- **UBaseType_t uxPriority:** Prioridade da tarefa
- **TaskHandle_t pvCreatedTask*:** Identificador da tarefa criada

### Ticks no RTOS
O Tick define quantas vezes por segundo o escalonador irá executar o algoritmo de mudança de tarefas.

- **Exemplo:** RTOS com tick de 10ms = 100 vezes por segundo
- **Frequência máxima recomendada:** 1000 Hz para ARM

```c
int ticks = 100; // equivalente a 0,1 segundo
vTaskDelay(pdMS_TO_TICKS(ticks));
```

---

## 9. SEMÁFOROS

### Conceito
Fundamentais para sincronização em programação concorrente. Gerenciam o acesso a recursos compartilhados entre multitarefas, funcionando como um contador.

### Comparação: Bare-metal vs RTOS

**Antes (Bare-metal):**
```c
void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        flag_f_r = 1;
    }
} 

void main(void) {
    while(1) { 
        if(flag_f_r) {
            // faz alguma coisa
            flag_f_r = 0;
        }
    }
}
```

**Com Semáforos:**
```c
void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        xSemaphoreGiveFromISR(xSemaphore, 0);
    }
} 

void task_main(void) {
    while(1) { 
        if(xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(100))) {
            // faz alguma coisa 
        } else { 
            // cai aqui se o semáforo não for liberado em 100 ms!
        }
    }
}
```

### ISR (Interrupt Service Routine)
Usado para interagir com hardware e software. Para saber que estamos lidando com interrupção, adicionamos `FromISR` no final:

**Hardware (callback):**
```c
bool timer_0_callback(repeating_timer_t *rt) {
    xSemaphoreGiveFromISR(xSemaphore, 0);
    return true;
}
```

**Software (task):**
```c
void task_1(void) {
    xSemaphoreGive(xSemaphore); // Sem FromISR
}
```

### Instruções Úteis
- **Criar variável global:** `SemaphoreHandle_t xSemaphore;`
- **Criar semáforo (na main):** `xSemaphore = xSemaphoreCreateBinary();`
- **Liberar semáforo:**
  - ISR: `xSemaphoreGiveFromISR(xSemaphore, 0);`
  - Task: `xSemaphoreGive(xSemaphore);`
- **Esperar pelo semáforo:** `xSemaphoreTake(xSemaphore, pdMS_TO_TICKS(500))`

### Exemplo Task-Task
```c
SemaphoreHandle_t xSemaphore_r;

void btn_1_task(void *p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    while (true) {
        if (!gpio_get(BTN_PIN_R)) {
            while (!gpio_get(BTN_PIN_R)) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
            xSemaphoreGive(xSemaphore_r);
        }
    }
}

void led_1_task(void *p) {
    gpio_init(LED_PIN_R);
    gpio_set_dir(LED_PIN_R, GPIO_OUT);

    while (true) {
        if (xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(500)) == pdTRUE) {
            gpio_put(LED_PIN_R, 1);
            vTaskDelay(pdMS_TO_TICKS(250));
            gpio_put(LED_PIN_R, 0);
            vTaskDelay(pdMS_TO_TICKS(250));
        }
    }
}
```

---

## 10. FILAS (QUEUE)

### Conceito
Maneira de enviar dados entre tarefas. Diferente de semáforos, permite enviar informações além de booleanas. Utilizam FIFO (First In, First Out).

### Comparação com PRA3
**Callback com Fila:**
```c
void gpio_callback(uint gpio, uint32_t events) {
    int time = 0;
    if (events == 0x4) { // fall edge
        time = to_us_since_boot(get_absolute_time());
    } else if (events == 0x8) { // rise edge
        time = to_us_since_boot(get_absolute_time());
    }
    xQueueSendFromISR(xQueueTime, &time, 0);
}
```

**Lendo da Fila:**
```c
void task_1(void){
    int gpio;
    while(1) {
        if (xQueueReceive(xQueueBtn, &gpio, pdMS_TO_TICKS(100))) {
            printf("Botão pressionado pino %d", gpio);
        } else {
            // cai aqui se não chegar dado em 100 ms!
        }
    }
}
```

### Parâmetros de xQueueReceive:
- **xQueueBtn:** Fila que desejamos ler
- **&gpio:** Local que vamos armazenar o valor lido  
- **pdMS_TO_TICKS(100):** Tempo que iremos esperar pelo dado

### Instruções para Uso:
- **Criar variável global:** `QueueHandle_t xQueueButId`
- **Criar fila na main:** `xQueueButId = xQueueCreate(32, sizeof(char));`
- **Colocar dados:**
  - ISR: `xQueueSendFromISR(xQueueButId, &id, 0);`
  - Task: `xQueueSend(xQueueButId, &id, 0);`
- **Receber dados:** `xQueueReceive(xQueueButId, &id, pdMS_TO_TICKS(500))`

### Exemplo Task-Task com Fila
```c
QueueHandle_t xQueueData;
 
static void task_1(void *pvParameters){
    int var = 3;
    while(1) {
        xQueueSend(xQueueData, &var, 0);
        vTaskDelay(100);
    }
}

static void task_2(void *pvParameters){
    int var = 0;
    while(1) {
        if(xQueueReceive(xQueueData, &var, pdMS_TO_TICKS(100))){
            printf("var: %d\n", var);
        }
    }
}
```

### Exemplo IRQ-Task com Fila
```c
QueueHandle_t xQueueButId;
 
void btn_callback(uint gpio, uint32_t events) {
    char id;
    if (events == 23) {         
        id = 23;
    } else if (gpio == 22) {  
        id = 22;
    }
    xQueueSendFromISR(xQueueButId, &id, 0);
}

static void task_led(void *pvParameters){
    char id;
    for (;;) {
        if(xQueueReceive(xQueueButId, &id, pdMS_TO_TICKS(100))){
            for (int i = 0; i < 10; i++) {
                gpio_put(id, i/2);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
}

void main(void) {
    xQueueButId = xQueueCreate(32, sizeof(char));
    
    if (xQueueButId == NULL)
        printf("falha em criar a fila \n");
}
```

---

## 11. HEADER FILES (.h)

### Conceito
Arquivos `.h` possibilitam melhor estrutura do programa, organizando o código de forma mais modular.

### O que NÃO pode ter no arquivo `.h`:
- Funções (apenas declarações)
- Redefinição de tipos ou funções

### Exemplo de Estrutura

**main.c**
```c
#include "bar.h"

void main(){
    // código principal
}
```

**bar.h**
```c
#ifndef BAR_H
#define BAR_H

int bar(int b);

#endif
```

**bar.c**
```c
#include "bar.h"

int bar(int b){
    // implementação da função
    return b * 2;
}
```

### CMakeList.txt
```cmake
add_executable(main main.c bar.c)
```

---

## 12. CONCEITOS IMPORTANTES

### Variáveis Globais
- **Uso recomendado:** Somente para passar informações de uma interrupção (ISR) para a função main

### Delays de Software
A SDK da Pico possui funções de delays:

- **`sleep`:** Coloca o core para dormir (economiza energia)
- **`busy_wait`:** "Gasta" clocks para dar tempo necessário (controle preciso)

**Quando usar cada um:**
- **busy_wait:** Controle preciso de tempo, mas consome mais energia
- **sleep:** Quando precisão não é crítica e quer economizar energia

### Uso de const
Útil para variáveis que não mudam e especialmente importante em funções com ponteiros para garantir controle sobre alterações de memória.

### Pooling vs Interrupções
**Pooling (não recomendado):**
```c
while(1){
    if(gpio_get(BTN))
        gpio_set(LED, 1);
    else
        gpio_set(LED, 0);
}
```
O CORE fica constantemente trabalhando, desperdiçando recursos.

**Interrupções (recomendado):**
Permite que o hardware notifique o software sobre eventos, liberando o processador para outras tarefas.

---

## 13. EXEMPLO COMPLETO COM CONTROLE DE LED

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

const int LED_PIN_Y = 10;
const int BTN_PIN_R = 26;
const int BTN_PIN_B = 19;

int volatile flag_b = 0;
int volatile flag_r = 0;
int volatile alarm_counter = 0;
int volatile timer_counter = 0;

void btn_callback(uint gpio, uint32_t events){
    if(events == 0x4){
        if (gpio == BTN_PIN_B){
            flag_b = 1;
        } else {
            flag_r = !flag_r;
        }
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data){
    alarm_counter = 1;
    return 0;
}

bool timer_callback(repeating_timer_t *t){
    timer_counter += 1;
    return true;
}

int main()
{
    stdio_init_all();

    gpio_init(LED_PIN_Y);
    gpio_init(BTN_PIN_B);
    gpio_init(BTN_PIN_R);

    gpio_set_dir(LED_PIN_Y, GPIO_OUT);
    gpio_set_dir(BTN_PIN_B, GPIO_IN);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);

    gpio_pull_up(BTN_PIN_B);
    gpio_pull_up(BTN_PIN_R);

    int timer_started = 0, alarm_started = 0;

    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, 
                                       true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_B, GPIO_IRQ_EDGE_FALL, true);

    alarm_id_t alarm;
    repeating_timer_t timer;

    while (1) {
        if(flag_r){
            if(!alarm_started){
                alarm_started = 1;
                alarm = add_alarm_in_ms(1000, alarm_callback, NULL, false);
            }

            if (alarm_counter == 1){
                cancel_alarm(alarm);
                if (!timer_started){
                    timer_started = 1;
                    add_repeating_timer_ms(1000, timer_callback, NULL, &timer);
                } else {
                    if (timer_counter % 2 != 0){
                        gpio_put(LED_PIN_Y, 1);
                    } else if(timer_counter % 2 == 0){
                        gpio_put(LED_PIN_Y, 0);
                    }
                }
            }
        } else {
            cancel_repeating_timer(&timer);
            timer_started = 0;
            alarm_started = 0;
            alarm_counter = 0;
            timer_counter = 0;
            gpio_put(LED_PIN_Y, 0);
        }

        if(flag_b){
            alarm_started = 1;
            timer_started = 1;
            cancel_alarm(alarm);
            cancel_repeating_timer(&timer);
            gpio_put(LED_PIN_Y, 0);
            flag_b = 0;
            flag_r = 0;
        }
    }
    return 0;
}
```

---

## 14. DICAS IMPORTANTES PARA A PROVA

1. **Sempre usar `volatile`** para variáveis modificadas em IRQ
2. **Pull-up em botões:** Lógica invertida (`!gpio_get())