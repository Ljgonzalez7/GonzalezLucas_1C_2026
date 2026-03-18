/*! @mainpage guia1_ejercicio3
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
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
#define CONFIG_BLINK_PERIOD 100
#define ON 1
#define OFF 0
#define TOGGLE 2
/*==================[internal data definition]===============================*/
struct leds
{
	uint8_t mode;
	uint8_t n_led;
	uint8_t n_ciclos;
	uint16_t periodo;
} my_leds, my_leds2, my_leds3;

/*==================[internal functions declaration]=========================*/
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

/*==================[internal functions declaration]=========================*/
//void funcion_con_puntero(struct my_leds *puntero_led){
//	if (puntero_led->mode==ON){
//		if(puntero_led->n_led==1){
//			LedOn(LED_1);
//		}
//			LedOn(LED_2);
//		}
//		else if(puntero_led->n_led==3){
//			LedOn(LED_3);
//		}
//	}
//	else if(puntero_led->mode==OFF){
//		if(puntero_led->n_led==1){
//			LedOff(LED_1);
//		}
//		else if(puntero_led->n_led==2){
//			LedOff(LED_2);
//		}
//		else if(puntero_led->n_led==3){
//			LedOff(LED_3);
//		}
//	}
//	else if(puntero_led->mode==TOGGLE){

//		for(int i=0;i<puntero_led->n_ciclos;i++){
//			if(puntero_led->n_led==1){
//					LedToggle(LED_1);
//				}
//				else if(puntero_led->n_led==2){
//					LedToggle(LED_2);
//				}
//				else if(puntero_led->n_led==3){
//					LedToggle(LED_3);
//				}
//			for (int j=0;j<(puntero_led->periodo/CONFIG_BLINK_PERIOD);j++){

//				vTaskDelay(CONFIG_BLINK_PERIOD/portTICK_PERIOD_MS);

//			}
//		}	
//	}
//}
/*==================[external functions definition]==========================*/
//void app_main(void){
//LedsInit();

//my_leds.mode=TOGGLE;
//my_leds.n_ciclos=10;
//my_leds.n_led=1;
//my_leds.periodo=500;

//funcion_con_puntero(&my_leds);
	
//}
/*==================[end of file]============================================*/