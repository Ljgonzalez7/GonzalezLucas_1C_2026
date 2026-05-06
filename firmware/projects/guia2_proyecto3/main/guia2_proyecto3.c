/*! @mainpage Guia 2 - Proyecto 3:
 * Descripcion
 * @section genDesc General Description
 *
 * Edicion del proyecto 2, empleo de UART puerto serie para ver por monitor la medición
 * del sensor de ultrasonido.
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
 * | 06/05/2026 | Document creation		                         |
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
#define TIMER_TAREA_1US 1000000

/** @def RANGOS
 * @brief configuracion de umbrales de medición.
 */
#define RANGO1 10
#define RANGO2 20
#define RANGO3 30

/** @def BAUD_RATE
 * @brief velocidad de transmisión de datos por segundo. (9600 bajo, 115200 alto)
 */
#define BAUD_RATE 9600

/*==================[internal data definition]===============================*/

/** @def uart_task_handle 
 * @brief Definicion que indica el orden de prioridad en el procesamiento del uart.
 */

TaskHandle_t Uart_task_handle = NULL;

/** @def SensorDistancia_task_handle 
 * @brief Definicion que indica el orden de prioridad en el procesamiento de tarea sensar.
 */

TaskHandle_t SensorDistancia_task_handle = NULL;

/** @def medir 
 * @brief variable booleana filtro de medir o parar medición.
 */
volatile bool medir = true;

/** @def hold 
 * @brief variable booleana filtro para mantener dato en pantalla LCD.
 */
volatile bool hold = false;


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
	if (distancia < RANGO1) {
        LedOff(LED_1); 
		LedOff(LED_2); 
		LedOff(LED_3);
    } 
    else if (distancia >= RANGO1 && distancia < RANGO2) {
        LedOn(LED_1); 
		LedOff(LED_2); 
		LedOff(LED_3);
    } 
    else if (distancia >= RANGO2 && distancia < RANGO3) {
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
void TimerInterrupcion(void *param){
    vTaskNotifyGiveFromISR(SensorDistancia_task_handle, pdFALSE);    /* Envía una notificación a la tarea 1 para interrumpirla */
}

/**
 * @brief Función que atiende a la Tecla 1, que activa o desactiva la medición de distancia.
 */
void switch1_interrupcion (void *param){
    medir =! medir;   /* Cambia el estado de medir */
}

/**
 * @brief Función que atiende a la Tecla 2, que cambia el estado del hold (mantener o no el resultado).
 */
void switch2_interrupcion (void *param){
    hold =! hold;                       /* Cambia el estado del hold */
}

/** @fn void Tarea_Uart(uint16_t distancia)
* @brief Función que envia las mediciones a la terminal de PC.
 * * @param p_config estructura del uart, detalla el puerto.
 */
void Tarea_Uart(void *pvParameter) {
    serial_config_t *p_config = (serial_config_t *)pvParameter;
    while(1){
        UartSendString(p_config->port, (char*)UartItoa(distancia,10));
        UartSendString(p_config->port, " cm\r\n");
        vTaskDelay(CONFIG_BLINK_PERIOD_LEDS / portTICK_PERIOD_MS);
    }
}
//PARA CUANDO PRECISA LECTURA DE TECLAS
//void TeclasUART(void *pvParameters){
//    uint8_t tecla;
//    UartReadByte(UART_PC, &tecla);
//    if(tecla == 'o') medir = !medir;
//    if(tecla == 'h') hold = !hold;
//}
/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Inicializacion\n");
	LedsInit();
    SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_3, GPIO_2); 
	
    /* Inicialización de timers */
    timer_config_t timer_1 = {
        .timer = TIMER_A, 
        .period = TIMER_TAREA_1US, 
        .func_p = TimerInterrupcion, 
        .param_p = NULL
    };

    TimerInit(&timer_1);

	printf("Ejecucion de programa\n");
	xTaskCreate(&TareaSensorDistancia, "SensorDist", 2048, NULL, 5, &SensorDistancia_task_handle);
    
    /* Interrupciones por switch*/
    SwitchActivInt(SWITCH_1, switch1_interrupcion, NULL);
    SwitchActivInt(SWITCH_2, switch2_interrupcion, NULL);
    
    /* Inicialización del conteo de timers */
    TimerStart(timer_1.timer);

    static serial_config_t config_uart = { 
        .port = UART_PC,	/*!< port */
	    .baud_rate = BAUD_RATE,		/*!< baudrate (bits per second) */
	    .func_p = NULL,
        .param_p = NULL			/*!< Pointer to callback function to call when receiving data (= UART_NO_INT if not requiered)*/	
    };

    UartInit(&config_uart);

    xTaskCreate(&Tarea_Uart, "UART", 2048, (void*) &config_uart,5,&Uart_task_handle);
    
    //Para cuando implemente el control por teclas
    //config_uart.func_p = TeclasUART;

}

/*==================[end of file]============================================*/