/*! @mainpage Guia 2 - Proyecto 1:
 * Descripcion
 * @section genDesc General Description
 * 
 * Proyecto inicial donde se utiliza manejo de tareas para la medición de distancia mediante un sensor
 * de ultrasonido HSR04. Se coordina el encendido de leds de acuerdo a la distancia medida y se muestra
 * por pantalla LCD.
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
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "gpio_mcu.h"
#include "hc_sr04.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/
/** @def CONFIG_BLINK_PERIOD_LEDS 
 * @brief configuracion del periodo de parpadeo leds.
 */
#define CONFIG_BLINK_PERIOD_LEDS 1000

/** @def CONFIG_RETARDO_TECLA 
 * @brief configuracion del retardo para la deteccion de tecla pulsada.
 */
#define CONFIG_RETARDO_TECLA 100

/** @def RANGOS 
 * @brief configuracion de umbrales de medición.
 */
#define RANGO1 10
#define RANGO2 20
#define RANGO3 30



/*==================[internal data definition]===============================*/

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

/** @fn void TarealeerTeclas(void *pvParameter)
 * @brief Tarea que acciona según lectura de teclas.
 */

static void TareaLeerTeclas(void *pvParameter) {
    while (1) {
        uint8_t teclas = SwitchesRead();
        
        if (teclas & SWITCH_1) {
            medir = !medir;
        }
        if (teclas & SWITCH_2) {
            hold = !hold;
        }
        vTaskDelay(CONFIG_RETARDO_TECLA / portTICK_PERIOD_MS); 
    }
}

/** @fn void TareaSensorDistancia(void *pvParameter)
 * @brief Tarea que integra el programa de sensado, interacción y mostrado por pantalla.
 */

void TareaSensorDistancia(void *pvParameter) {    
	while (1) {
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

        vTaskDelay(CONFIG_BLINK_PERIOD_LEDS/ portTICK_PERIOD_MS);
    }
}


/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Inicializacion\n");
	LedsInit();
	LcdItsE0803Init();
	SwitchesInit();
	HcSr04Init(GPIO_3, GPIO_2); 
	
	printf("Ejecucion de programa\n");
    xTaskCreate(&TareaLeerTeclas, "LecturaTeclas", 2048, NULL, 5, NULL);
	xTaskCreate(&TareaSensorDistancia, "SensorDist", 2048, NULL, 5, NULL);
}
/*==================[end of file]============================================*/