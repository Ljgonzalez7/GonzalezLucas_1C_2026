/*! @mainpage Template
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpio_mcu.h"


/*==================[macros and definitions]=================================*/
#define CANTIDAD_BITS 4

/*==================[internal data definition]===============================*/
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;


/*==================[internal functions declaration]=========================*/

int8_t convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	for(int i=0; i<digits;i++){

		bcd_number [i] = data%10;
		data/=10;

	}
	return 0;

}

void cambio_estado_gpio(uint8_t bcd_number, gpioConf_t *vector_estructuras)
{
	for (int i=0;i<CANTIDAD_BITS;i++)
	{
		if((bcd_number&1<<i)){
			GPIOOn(vector_estructuras[i].pin);
		}
		else {
			GPIOOff(vector_estructuras[i].pin);
		}
	}
}	

void nueva_funcion(uint32_t dato, uint8_t digitos_salida, gpioConf_t *vector_estructuras, gpioConf_t *vector_mapeo)
{
	uint8_t bcd_array[digitos_salida];
	convertToBcdArray(dato, digitos_salida, bcd_array);

	for (int i = 0; i<digitos_salida; i++)
	{
		cambio_estado_gpio(bcd_array[i], vector_estructuras);
		GPIOOn(vector_mapeo[i].pin);
		GPIOOff(vector_mapeo[i].pin);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
	printf("DESARROLLO EJ_4\n");
	uint32_t dato = 15;
	uint8_t digitos = 3;
	uint8_t numero_bcd [3]={0};
	convertToBcdArray(dato, digitos, numero_bcd);
	
	for (int i=0; i<digitos;i++){
		printf("%d", numero_bcd[i]);
		printf("\n");
	}

	printf("DESARROLLO EJ_5\n");

	gpioConf_t vector[]=
	{
		{GPIO_20,GPIO_OUTPUT},
		{GPIO_21,GPIO_OUTPUT},
		{GPIO_22,GPIO_OUTPUT},
		{GPIO_23,GPIO_OUTPUT}
	};

	gpioConf_t vector_map[]=
	{
		{GPIO_9,GPIO_OUTPUT},
		{GPIO_18,GPIO_OUTPUT},
		{GPIO_19,GPIO_OUTPUT},
	};

	for (int i=0; i<CANTIDAD_BITS; i++)
	{
		GPIOInit(vector[i].pin,vector[i].dir);
	}
	
	for (int i = 0; i < digitos; i++) GPIOInit(vector_map[i].pin, vector_map[i].dir);
		
	printf("\n");

	//cambio_estado_gpio(numero_bcd[0], vector);

	nueva_funcion(dato, digitos, vector, vector_map);


}
/*==================[end of file]============================================*/