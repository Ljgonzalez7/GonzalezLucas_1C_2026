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
TaskHandle_t led1_task_handle = NULL;
bool medir = true;
bool hold = false;


static gpio_t echo_st, trigger_st; /**<  Inicializacion del pin*/

/*==================[internal functions declaration]=========================*/
void ActualizarLEDS(){
		HcSr04Init(echo_st, trigger_st);
		uint16_t distancia = HcSr04ReadDistanceInCentimeters();
		Medicion();
        
		while(MEDIR==true)
		{
				if(distancia < rango1)
							{
								printf("APAGAR TODOS LOS LEDS\n");
								LedOff(LED_1);
								LedOff(LED_2);
								LedOff(LED_3);

							}
				else if (rango2<distancia<rango3)
				{
					printf("PRENDER 1 Y 2 LEDS\n");
					LedOn(LED_1);
					LedOn(LED_2);
				}
				else if(distancia>rango3)
				{
					printf("PRENDER TODOS LOS LEDS\n");
					LedOn(LED_1);
					LedOn(LED_2);
					LedOn(LED_3);

				}
		}
		
	vTaskDelay(CONFIG_BLINK_PERIOD_LEDS / portTICK_PERIOD_MS);

    }


void Medicion(){
    while(true){
		if(TEC1==1){
			MEDIR!=MEDIR;
		}
		}
}


/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Inicializacion\n");
	LedsInit();
	LcdItsE0803Init();
	LcdItsE0803Write(distancia)

	
}
/*==================[end of file]============================================*/