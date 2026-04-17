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
#include "gpio_mcu.h"
#include "hc_sr04.h"
#include "lcditse0803.h"

/*==================[macros and definitions]=================================*/
/** @def CONFIG_BLINK_PERIOD_LEDS 
 * @brief configuracion del periodo de parpadeo leds.
 */
#define CONFIG_BLINK_PERIOD_LEDS 1000

/** @def rangos 
 * @brief configuracion de umbrales de medición.
 */
#define rango1 10
#define rango2 20
#define rango3 30



/*==================[internal data definition]===============================*/
/** @def leds_task_handle 
 * @brief Definicion que indica el orden de prioridad en el prosecamiento de tareas.
 */
//TaskHandle_t led1_task_handle = NULL; PARA CUANDO USEMOS TAREAS
/** @def medir 
 * @brief variable booleana filtro de medir o parar medición.
 */
bool medir = true;

/** @def hold 
 * @brief variable booleana filtro para mantener dato en pantalla LCD.
 */
bool hold = false;


static gpio_t echo_st, trigger_st; /**<  Inicializacion del pin*/

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

/** @fn void leerTeclas(bool *medir, bool *hold)
 * @brief Función que acciona según lectura de teclas.
 * * @param medir puntero a direccion de memoria de variable booleana medir.
 * * @param hold puntero a direccion de memoria de variable booleana hold.
 */

void leerTeclas(bool *medir, bool *hold) {
    uint8_t teclas = SwitchesRead();
    if (teclas & SWITCH_1) {
        *medir = !(*medir);
        // vTaskDelay(100 / portTICK_PERIOD_MS); // Antirebote simple
    }
    if (teclas & SWITCH_2) {
        *hold = !(*hold);
    }
}

/** @fn void appSensorDistancia()
 * @brief Función que integra el programa de sensado, interacción y mostrado por pantalla.
 */

void appSensorDistancia() {
    while (1) {
        leerTeclas(&medir, &hold); 

        if (medir) {
            uint16_t distancia = HcSr04ReadDistanceInCentimeters();
            actualizarLeds(distancia);

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

	appSensorDistancia();

}
/*==================[end of file]============================================*/