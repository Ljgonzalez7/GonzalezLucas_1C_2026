/*! @mainpage Guia2 - Proyecto 4
 *
 * @section genDesc General Description
 *
 * Proyecto Osciloscopio.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 13/05/2026 | Document creation		                         |
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
#include "gpio_mcu.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"


/*==================[macros and definitions]=================================*/

/** @def timer_tarea1_US
 * @brief Variable que controla el timer de la tarea 1 (disparador) [us]
 */
#define TIMER_TAREA_1US 2000

/** @def BAUD_RATE
 * @brief velocidad de transmisión de datos por segundo. (9600 bajo, 115200 alto)
 */
#define BAUD_RATE 115200


/*==================[internal data definition]===============================*/

/** @def uart_task_handle 
 * @brief Definicion que indica el orden de prioridad en el procesamiento del uart.
 */

TaskHandle_t Uart_task_handle = NULL;

/** @def valor_adc 
 * @brief Variable global para guardar la lectura.
 */
uint16_t valor_adc = 0; 

/*==================[internal functions declaration]=========================*/

/**
 * @brief Función invocada en la interrupción del timer, que envía una notificación a la tarea UART
 *  para disparar la conversión AD.
 */
void TimerInterrupcion(void *param){
    vTaskNotifyGiveFromISR(Uart_task_handle, pdFALSE);    /* Envía una notificación a la tarea UART para interrumpirla */
}

/** @fn void Tarea_Uart(* pvParameter)
* @brief Tarea encargada de la conversión AD y almacenamiento en variable global valor_adc.
 * * @param p_config estructura del uart.
 */
void Tarea_Uart(void *pvParameter) {

	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY); 
		AnalogInputReadSingle(CH1, &valor_adc);
        UartSendString(UART_PC, ">adc:");        
        UartSendString(UART_PC, (char*)UartItoa(valor_adc, 10));
        UartSendString(UART_PC, "\r\n");
    }
}

/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Inicializacion\n");

	analog_input_config_t config1 = {			
		.input = CH1,			/*!< Inputs: CH0, CH1, CH2, CH3 */
		.mode = ADC_SINGLE,	/*!< Mode: single read or continuous read */
		.func_p = NULL,			/*!< Pointer to callback function for convertion end (only for continuous mode) */
		.param_p = NULL,			/*!< Pointer to callback function parameters (only for continuous mode) */
		.sample_frec = 0	/*!< Sample frequency min: 20kHz - max: 2MHz (only for continuous mode)  */
	};

	AnalogInputInit(&config1);

    /* Inicialización de timers */
    timer_config_t timer_1 = {
        .timer = TIMER_A, 
        .period = TIMER_TAREA_1US, 
        .func_p = TimerInterrupcion, 
        .param_p = NULL
    };

    TimerInit(&timer_1);

	static serial_config_t config_uart_entrada = { 
        .port = UART_PC,	/*!< port */
	    .baud_rate = BAUD_RATE,		/*!< baudrate (bits per second) */
	    .func_p = NULL,
        .param_p = NULL			/*!< Pointer to callback function to call when receiving data (= UART_NO_INT if not requiered)*/	
    };
	
    UartInit(&config_uart_entrada);

    xTaskCreate(&Tarea_Uart, "UART", 2048, NULL, 5, &Uart_task_handle);
	
	TimerStart(timer_1.timer);


}

/*==================[end of file]============================================*/