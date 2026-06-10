/*! @mainpage Proyecto Integrador
 *
 * @section genDesc General Description
 *
 * Proyecto integrador: software que permite optimizar el test de dorsiflexión de tobillo utilizando
 * sensores de distancia y sensor de compensación del valgo de rodilla. 
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
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
 * |    ALARMA      |    GPIO_?     |  
 * 
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 03/06/2026 | Document creation		                         |
 *
 * @author Lucas González (lucas.gonzalez@ingenieria.uner.edu.ar)
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
#include "led.h"
#include "switch.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/
/** @def CONFIG_BLINK_PERIOD_LEDS 
 * @brief configuracion del periodo de parpadeo leds.
 */
#define CONFIG_BLINK_PERIOD_LEDS 1000

/** @def timer_sensado_40ms
 * @brief Variable que controla el timer de la tarea sensado [us]
 */
#define TIMER_SENSADO_US 40000     /* 40 ms de período (Frecuencia de muestreo 25 Hz) */


/** @def timer_general_1min
 * @brief Variable que controla el timer de calentamiento [us]
 */
#define TIEMPO_CALENTAMIENTO_MS  60000  /* 60000 ms = 1 Minuto */


/** @def baud_rate 
 * @brief Variable de velocidad de transmisión a la PC
 */
#define BAUD_RATE  115200    /* 115200 para evitar retardos en graficadores de PC */

/** @def RANGOS FRONTALES
 * @brief configuracion de umbrales de medición frontal.
 */
#define RANGO1 12
#define RANGO2 5

/*==================[internal data definition]===============================*/

/** @def hold 
 * @brief variable booleana filtro para iniciar o finalizar el estudio.
 */
volatile bool inicio = false;

/** @def medir 
 * @brief variable booleana filtro de medir o parar medición.
 */
volatile bool medir = false;

/** @def medir 
 * @brief variable booleana para generar el reporte del estudio.
 */
volatile bool reporte = false;

/** @def calentando 
 * @brief variable booleana para verificar el calentamiento previo al estudio.
 */
volatile bool calentando = false;

/** @def distancia_frontal 
 * @brief variable para almacenar la distancia frontal de la medición.
 */

uint16_t distancia_frontal = 0; 

/** @def distancia_frontal_min 
 * @brief variable para almacenar la distancia frontal minima.
 */

uint16_t distancia_frontal_min = 30; 

/** @def distancia_lateral 
 * @brief variable para almacenar la distancia lateral de la medición.
 */
uint16_t distancia_lateral = 0;


/** @def uart_task_handle 
 * @brief Definicion que indica el orden de prioridad en el procesamiento del uart.
 */

TaskHandle_t Uart_task_handle = NULL;

/** @def SensorDistanciaFrontal_task_handle 
 * @brief Definicion que indica el orden de prioridad en el procesamiento de tarea sensar.
 */

TaskHandle_t SensorDistanciaFrontal_task_handle = NULL;

/** @def Calentamiento_task_handle
 * @brief Definicion que indica el orden de prioridad en el procesamiento de tarea sensar.
 */

TaskHandle_t Calentamiento_task_handle = NULL;

/*==================[internal functions declaration]=========================*/
/** @fn void actualizarLeds(uint16_t distancia_frontal)
* @brief Función que cambia el estado de los leds según distancia frontal medida.
 * * @param distancia valor de distancia medido en cm.
 */
void actualizarLeds(uint16_t distancia_frontal) {
	if (distancia_frontal > RANGO1) {
        LedOff(LED_1);
		LedOff(LED_2);
        LedOn(LED_3);  
    } 
    else if (distancia_frontal < RANGO1 && distancia_frontal > RANGO2) {
        LedOff(LED_1); 
		LedOn(LED_2); 
		LedOff(LED_3);
    } 
    else if (distancia_frontal <= RANGO2) {
        LedOn(LED_1); 
		LedOff(LED_2); 
		LedOff(LED_3);
    } 
    else {
        LedOff(LED_1); 
		LedOff(LED_2); 
		LedOff(LED_3);
    }
	
}

/** @fn void TareaSensorDistanciaFrontal(void *pvParameter)
 * @brief Tarea que integra el programa de sensado, interacción y mostrado por pantalla.
 */

static void TareaSensorDistanciaFrontal(void *pvParameter) {    
	while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);    /* La tarea espera en este punto hasta recibir una notificación */

        if (medir) {
            distancia_frontal = HcSr04ReadDistanceInCentimeters();
            actualizarLeds(distancia_frontal);
			printf("Distancia detectada: %d cm\n", distancia_frontal); 

            if (distancia_frontal<distancia_frontal_min) {
                distancia_frontal_min=distancia_frontal;
            }
            
            LcdItsE0803Write(distancia_frontal_min);     
            
            } 
            else {
            LcdItsE0803Off();
            LedOff(LED_1); 
			LedOff(LED_2); 
			LedOff(LED_3);
        }

    }

}

/**
 * @fn TareaCalentamientoArticular
 * @brief Tarea encargada de controlar el flujo del estudio y el tiempo de calentamiento.
 */
static void TareaCalentamientoArticular(void *pvParameter) {    
    while (1) {
        /* Si se presionó 'O' y el sistema debe iniciar */
        if (inicio && !calentando && !medir) {
            printf("--- ARRANCANDO MINUTO DE CALENTAMIENTO ---\n");
            UartSendString(UART_PC, "Estado: Calentamiento en curso...\r\n");
            calentando = true;

            /* Indicación visual en el LCD de que está en preparación (ej. rayas o animación) */
            LcdItsE0803Write(111); // Podés mandar un código visual al LCD

            /* NUEVA LÓGICA: Bucle de tiempo con vTaskDelay para el blinkeo */
            // 120 iteraciones de 500ms = 60 segundos (1 minuto)
            for (int i = 0; i < 120; i++) {
                /* Si el usuario apaga el estudio en el medio con 'O', salimos del bucle */
                if (!inicio) break; 

                LedToggle(LED_1);
                LedToggle(LED_2);
                LedToggle(LED_3);

                /* La tarea se duerme de forma eficiente por 500 ms */
                vTaskDelay(500 / portTICK_PERIOD_MS); 
            }

            /* Al terminar el bucle (pasó el minuto), apagamos banderas y activamos el test */
            if (inicio) { 
                calentando = false;
                printf("--- CALENTAMIENTO FINALIZADO. INICIE EL TEST ---\n");
                UartSendString(UART_PC, "Estado: Test En espera, presione M para medir...\r\n");
                
                /* Dejamos los LEDs en un estado conocido antes de empezar */
                LedOff(LED_1); LedOff(LED_2); LedOff(LED_3);
            }
        }
        
        vTaskDelay(200 / portTICK_PERIOD_MS); /* Tasa de chequeo de la tarea */
    }
}

/**
 * @brief Función invocada en la interrupción del timerUART, que envía una notificación a la tarea UART
 *  para disparar la conversión AD.
 */
void TimerInterrupcionUart(void *param){
    vTaskNotifyGiveFromISR(Uart_task_handle, pdFALSE);    /* Envía una notificación a la tarea UART para interrumpirla */
}

/**
 * @brief Función invocada en la interrupción del timer, que envía una notificación a la tarea sensado
 *  para medir o no distancia, activar los leds o la pantalla LCD.
 */
void TimerInterrupcion(void *param){
    vTaskNotifyGiveFromISR(SensorDistanciaFrontal_task_handle, pdFALSE);    /* Envía una notificación a la tarea sensado para interrumpirla */
}

/**
 * @brief Función que atiende a la Tecla 1, que inicia o finaliza el estudio.
 */
void switch1_interrupcion (void *param){
    inicio =! inicio;   /* Cambia el estado de inicio */
}

/**
 * @brief Función que atiende a la Tecla 2, que activa o desactiva la medición de distancia.
 */
void switch2_interrupcion (void *param){
    medir =! medir;                       /* Cambia el estado de medir */
}

/**
 * @brief Función que atiende a la Tecla 3, que permite generar el reporte del estudio.
 */
void switch3_interrupcion (void *param){
    reporte =! reporte;                       /* Cambia el estado del reporte */
}

/** /
 * @fn TareaTelemetriaUart
 * @brief Envía los datos limpios al graficador de la PC.
 */
void TareaTelemetriaUart(void *pvParameter) {
    while(1){
        if (medir) {
            /* Formato compatible con el graficador de puerto serie (prefijo '>') */
            UartSendString(UART_PC, ">frente:");
            UartSendString(UART_PC, (char*)UartItoa(distancia_frontal, 10));
            UartSendString(UART_PC, "\r\n");
        }
        vTaskDelay(50 / portTICK_PERIOD_MS); /* 20 muestras por segundo en el plotter de la PC */
    }
}

/** /
 * @fn TareaReporteEstudio
 * @brief Elabora el reporte final del estudio.
 */
static void TareaReporte(void *pvParameter) {    
    while (1) {
        /* Se activa cuando la bandera 'reporte' pasa a true (por Tecla 'R') */
        if (reporte) {
            // 1. Pausamos momentáneamente la medición para que no se mezclen los datos en la terminal
            medir = false;

            // 2. Enviamos el encabezado del informe
            UartSendString(UART_PC, "\r\n==================================================\r\n");
            UartSendString(UART_PC, "        INFORME CLÍNICO - BIOMECÁNICA WBLT        \r\n");
            UartSendString(UART_PC, "==================================================\r\n");
            
            // 3. Resultado de movilidad (Dorsiflexión)
            UartSendString(UART_PC, " -> Máxima Dorsiflexión Lograda: ");
            if (distancia_frontal_min == 30) {
                UartSendString(UART_PC, "No se registraron mediciones válidas.\r\n");
            } else {
                UartSendString(UART_PC, (char*)UartItoa(distancia_frontal_min, 10));
                UartSendString(UART_PC, " cm a la pared.\r\n");
                
                // Clasificación clínica rápida según el resultado
                if (distancia_frontal_min <= RANGO2) {
                    UartSendString(UART_PC, "    [EVALUACIÓN]: Movilidad ÓPTIMA (Rango Excelente).\r\n");
                } else if (distancia_frontal_min > RANGO2&& distancia_frontal_min <= RANGO1) {
                    UartSendString(UART_PC, "    [EVALUACIÓN]: Movilidad REGULAR (Restricción leve).\r\n");
                } else {
                    UartSendString(UART_PC, "    [EVALUACIÓN]: Movilidad ACORTADA (Restricción severa).\r\n");
                }
            }

            // 4. Cierre del documento
            UartSendString(UART_PC, "==================================================\r\n");
            UartSendString(UART_PC, " Fin del reporte. Listo para una nueva evaluación.\r\n");
            UartSendString(UART_PC, "==================================================\r\n\r\n");

            // 5. Reseteamos las variables para una nueva prueba limpia
            distancia_frontal_min = 30; 
            reporte =! reporte; 
            
            // Restauramos el estado de medición que tenía antes de pedir el reporte
            medir =! medir; 
        }

        vTaskDelay(300 / portTICK_PERIOD_MS); /* Tasa de chequeo pasiva */
    }
}

void Funcion_manejo_teclas(){
    uint8_t tecla;
    UartReadByte(UART_PC, &tecla);
    if(tecla == 'o' || tecla == 'O') {
        inicio = !inicio;
    }
    if(tecla == 'm' || tecla == 'M') {
        medir = !medir;
    }
	if(tecla == 'r' || tecla == 'R') {
        reporte = !reporte;
    }
}


/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Inicializacion\n");
	LedsInit();
    SwitchesInit();
	LcdItsE0803Init();
	HcSr04Init(GPIO_3, GPIO_2); 
   // HcSr04Init(GPIO_5, GPIO_4); Ver para el 2do sensor ultrasonido

	/* Inicialización de timers */
    timer_config_t timer_1 = {
        .timer = TIMER_A, 
        .period = TIMER_SENSADO_US, 
        .func_p = TimerInterrupcion, 
        .param_p = NULL
    };

    TimerInit(&timer_1);

	printf("Ejecucion de programa\n");
    xTaskCreate(&TareaCalentamientoArticular, "CalentamientoArt", 2048, NULL, 4, &Calentamiento_task_handle);
	xTaskCreate(&TareaSensorDistanciaFrontal, "SensorDist", 2048, NULL, 5, &SensorDistanciaFrontal_task_handle);
    
    /* Interrupciones por switch / Solo inicio y medicion */
    SwitchActivInt(SWITCH_1, switch1_interrupcion, NULL);
    SwitchActivInt(SWITCH_2, switch2_interrupcion, NULL);
    
    /* Inicialización del conteo de timers */
    TimerStart(timer_1.timer);

    static serial_config_t config_uart_entrada = { 
    .port = UART_PC,	/*!< port */
	.baud_rate = BAUD_RATE,		/*!< baudrate (bits per second) */
	.func_p = Funcion_manejo_teclas,
    .param_p = NULL			/*!< Pointer to callback function to call when receiving data (= UART_NO_INT if not requiered)*/	
    };
    
    UartInit(&config_uart_entrada);

    xTaskCreate(&TareaTelemetriaUart, "UART", 2048, (void*) &config_uart_entrada,5,&Uart_task_handle);
    xTaskCreate(&TareaReporte, "ReporteTask", 2048, NULL, 3, NULL);
}

/*==================[end of file]============================================*/

