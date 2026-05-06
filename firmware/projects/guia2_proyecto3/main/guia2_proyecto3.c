/*! @mainpage Guia 2 - Proyecto 1:
 * Descripcion
 * @section genDesc General Description
 *
 
 *
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32       |
 * |:--------------:|:--------------|
 * |    Segmento A  |    GPIO_20    |
 * |    Segmento B  |    GPIO_21    |
 * |    Segmento C  |    GPIO_22    |
 * |    Segmento D  |    GPIO_23    |
 * |    Digito Uni  |    GPIO_9     |
 * |    Digito Dec  |    GPIO_18    |
 * |    Digito Cen  |    GPIO_19    |
 * |    GND         |    GND        |
 * |    ECHO        |    GPIO_3     |
 * |    TRIGGER     |    GPIO_2     |
 * |    5V          |    5V         |



 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 15/04/2026 | Document creation		                         |
 *
 * @author Lucas Gonzalez (lucas.gonzalez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "gpio_mcu.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "timer_mcu.h"
#include "uart_mcu.h"

/*==================[macros and definitions]=================================*/
/** @def CONFIG_BLINK_PERIOD_LEDS 
 * @brief configuracion del periodo de parpadeo leds.
 */
#define CONFIG_BLINK_PERIOD_LEDS 1000

/** @def timer_tarea_1US
 * @brief Variable que controla el timer de la tarea 1 (lectura de teclas) [us]
 */
#define timer_tarea_1US 1000000

/** @def rangos 
 * @brief configuracion de umbrales de medición.
 */
#define rango1 10
#define rango2 20
#define rango3 30
#define BAUD_RATE 9600

/*==================[internal data definition]===============================*/

/** @def uart_task_handle 
 * @brief Definicion que indica el orden de prioridad en el procesamiento del uart.
 */

TaskHandle_t uart_task_handle = NULL;

/** @def TareaSensorDistancia_task_handle 
 * @brief Definicion que indica el orden de prioridad en el procesamiento de tarea medir.
 */

TaskHandle_t TareaSensorDistancia_task_handle = NULL;

/** @def medir 
 * @brief variable booleana filtro de medir o parar medición.
 */
bool medir = true;

/** @def hold 
 * @brief variable booleana filtro para mantener dato en pantalla LCD.
 */
bool hold = false;
/**
 * @brief List of UART ports available in ESP-EDU
 */
typedef enum uart_ports{
	UART_PC,				/*!< UART connected PC through USB port (indicated with UART) (also maped to TX: GPIO16, RX: GPIO17) */
	UART_CONNECTOR,			/*!< UART connected to J2 connector (TX: GPIO18, RX: GPIO19) */
} uart_mcu_port_t;

/**
 * @brief Serial port configuration struct
 */
typedef struct {			
	uart_mcu_port_t port;	/*!< port */
	uint32_t baud_rate;		/*!< baudrate (bits per second) */
	void *func_p;			/*!< Pointer to callback function to call when receiving data (= UART_NO_INT if not requiered)*/
	void *param_p;			/*!< Pointer to callback function parameters */
} serial_config_t;

/** @def distancia 
 * @brief variable para almacenar la distancia de la medición.
 */

uint16_t distancia = 0; 

/*==================[internal functions declaration]=========================*/
/** @fn void actualizarLeds(uint16_t distancia)
* @brief Función que cambia el estado de los leds según distancia medida.
 * * @param distancia valor de distancia medido en cm.
 */
void actualizarLeds(uint16_t distancia) {
	if (distancia < rango1) {
        LedOff(LED_1); 
		LedOff(LED_2); 
		LedOff(LED_3);
    } 
    else if (distancia >= rango1 && distancia < rango2) {
        LedOn(LED_1); 
		LedOff(LED_2); 
		LedOff(LED_3);
    } 
    else if (distancia >= rango2 && distancia < rango3) {
        LedOn(LED_1); 
		LedOn(LED_2); 
		LedOff(LED_3);
    } 
    else {
        LedOn(LED_1); 
		LedOn(LED_2); 
		LedOn(LED_3);
    }
	
}


/** @fn void TareaSensorDistancia(void *pvParameter)
 * @brief Tarea que integra el programa de sensado, interacción y mostrado por pantalla.
 */

static void TareaSensorDistancia(void *pvParameter) {    
	while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    /* La tarea espera en este punto hasta recibir una notificación */

        if (medir) {
            distancia = HcSr04ReadDistanceInCentimeters();
            actualizarLeds(distancia);
			printf("Distancia detectada: %d cm\n", distancia); 

            if (!hold) {
                LcdItsE0803Write(distancia);
            }
        } else {
            LcdItsE0803Off();
            LedOff(LED_1); 
			LedOff(LED_2); 
			LedOff(LED_3);
        }

    }
}

/**
 * @brief Función invocada en la interrupción del timer, que envía una notificación a la tarea 1
 *  para medir o no distancia, activar los leds o la pantalla LCD.
 */
void TimerInterrupcion(void* param){
    vTaskNotifyGiveFromISR(TareaSensorDistancia_task_handle, pdFALSE);    /* Envía una notificación a la tarea 1 para interrumpirla */
}

/**
 * @brief Función que atiende a la Tecla 1, que activa o desactiva la medición de distancia.
 */
void switch1_interrupcion (void* param){
    medir =! medir;   /* Cambia el estado de medir */
}

/**
 * @brief Función que atiende a la Tecla 2, que cambia el estado del hold (mantener o no el resultado).
 */
void switch2_interrupcion (void* param){
    hold =! hold;                       /* Cambia el estado del hold */
}

/** @fn void enviar_a_pc(uint16_t distancia)
* @brief Función que envia las mediciones a la terminal de PC.
 * * @param distancia valor de distancia medida.
 */
void uart_task(void *param) {
    while(1){
        UartSendString(UART_PC, (char*)UartItoa(distancia,10));
        vTaskDelay(CONFIG_BLINK_PERIOD_LEDS / portTICK_PERIOD_MS);
    }
}


/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Inicializacion\n");
	LedsInit();
	LcdItsE0803Init();
	SwitchesInit();
	HcSr04Init(GPIO_3, GPIO_2); 
	

    /* Inicialización de timers */
    timer_config_t timer_1 = {
        .timer = TIMER_A, 
        .period = timer_tarea_1US, 
        .func_p = TimerInterrupcion, 
        .param_p = NULL
    };

    TimerInit(&timer_1);

	printf("Ejecucion de programa\n");
	xTaskCreate(&TareaSensorDistancia, "SensorDist", 2048, NULL, 5, &TareaSensorDistancia_task_handle);
    
    /* Interrupciones por switch*/
    SwitchActivInt(SWITCH_1, switch1_interrupcion, NULL);
    SwitchActivInt(SWITCH_2, switch2_interrupcion, NULL);
    
    /* Inicialización del conteo de timers */
    TimerStart(timer_1.timer);

    serial_config_t config_uart = { 
        .port = UART_PC,	/*!< port */
	    .baud_rate = BAUD_RATE,		/*!< baudrate (bits per second) */
	    .func_p = NULL,
        .param_p = NULL			/*!< Pointer to callback function to call when receiving data (= UART_NO_INT if not requiered)*/	
    };

    UartInit(&config_uart);

    xTaskCreate(&uart_task, "UART", 512, &config_uart,5,&uart_task_handle);
}

/*==================[end of file]============================================*/