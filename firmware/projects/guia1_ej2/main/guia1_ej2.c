/*! @mainpage Guia1 - Ej2:
 * Modificación de proyecto blinking_switch.

 * @section genDesc General Description
 *
 * Programa para que parpadeen leds según los botones pulsados.
 *
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
 * | 11/03/2026 | Document creation		                         |
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
#define CONFIG_BLINK_PERIOD 1000 
/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void){
	uint8_t teclas;
	LedsInit();
	SwitchesInit();
    while(1)    {
    	teclas  = SwitchesRead();
    	switch(teclas){
    		case SWITCH_1:
    			LedToggle(LED_1);
    		break;
    		case SWITCH_2:
    			LedToggle(LED_2);
    		break;
			case (SWITCH_1 | SWITCH_2):
				LedToggle(LED_3);
			break;
    	}
	}
}