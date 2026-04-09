/*! @mainpage Guia 1 - Ej3:
 * 
 * @section genDesc General Description
 *
 * Funcion recibe un puntero led y controla el comportamiento de los mismos
 * según la combinación de botones. 
 *
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32       |
 * |:--------------:|:--------------|
 * |      LED 1     |    GPIO_11    |
 * |      LED 2     |    GPIO_12    |
 * |      LED 3     |    GPIO_13    |
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 18/03/2026 | Document creation		                         |
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
/*==================[macros and definitions]=================================*/
/** @def CONFIG_BLINK_PERIOD 
 * @brief periodo de parpadeo del led.
 */
#define CONFIG_BLINK_PERIOD 100
/** @def ON 
 * @brief Estado de encendido led, valor 1.
 */
#define ON 1
/** @def OFF 
 * @brief Estado de apagado led, valor 0.
 */
#define OFF 0
/** @def TOGGLE 
 * @brief Estado de parpadeo led, valor 2.
 */
#define TOGGLE 2

/*==================[internal data definition]===============================*/
/** *@struct leds
 * @brief Estructura que contiene la configuración de comportamiento de un LED.
 */
struct leds
{
	uint8_t mode;
	uint8_t n_led;
	uint8_t n_ciclos;
	uint16_t periodo;
} my_leds, my_leds2, my_leds3;

/*==================[internal functions declaration]=========================*/
/** @fn void funcion_leds(struct *leds)
 * @brief Controla el estado de un LED según los parámetros de la estructura recibida.
 * * @param leds Puntero a la estructura de tipo leds que contiene la configuración.
 */

void funcion_leds(struct leds *leds){
	switch (leds->mode){
		case ON:
			switch (leds->n_led){
				case 1:
					LedOn(LED_1);
					break;
				case 2:
					LedOn(LED_2);
					break;
				case 3:
					LedOn(LED_3);
					break;
			} break;
		
		case OFF:
			switch (leds->n_led){
				case 1:
					LedOff(LED_1);
					break;
				case 2:
					LedOff(LED_2);
					break;
				case 3:
					LedOff(LED_3);
					break;
			} break;
		case TOGGLE:
			for(int i=0;i<leds->n_ciclos;i++){
				switch (leds->n_led){
					case 1:
						LedToggle(LED_1);
						break;
					case 2:
						LedToggle(LED_2);
						break;
					case 3:
						LedToggle(LED_3);
						break;
				}
				for (int j=0;j<(leds->periodo/CONFIG_BLINK_PERIOD);j++){

					vTaskDelay(CONFIG_BLINK_PERIOD/portTICK_PERIOD_MS);

				}
			}
}

}

/*==================[external functions definition]==========================*/
/**
 * @brief Función principal de la aplicación.
 */

void app_main(void){
	LedsInit();
	my_leds.mode=TOGGLE;
	my_leds.n_ciclos=10;
	my_leds.n_led=1;
	my_leds.periodo=500;

	my_leds3.mode=TOGGLE;
	my_leds3.n_ciclos=14;
	my_leds3.n_led=3;
	my_leds3.periodo=500;

	my_leds2.mode=ON;
	my_leds2.n_led=2;

	funcion_leds(&my_leds);
	funcion_leds(&my_leds2);
	funcion_leds(&my_leds3);


}

/*==================[end of file]============================================*/