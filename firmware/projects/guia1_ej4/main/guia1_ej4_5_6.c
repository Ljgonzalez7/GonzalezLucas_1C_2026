/*! @mainpage Guia 1 - Ej4_5_6:
 * Conversor de dato numérico a BCD para mostrar en pantalla LCD.
 * @section genDesc General Description
 *
 * Funcion recibe un dato numérico led, lo convierte a código BCD y devuelve por 
 * referencia un arreglo con el número BCD.
 * Otra funcion pasa el número BCD a cambios de estado de 4 GPIOs.
 * Otra función final almacena cada cambio de estado de los 4 GPIOs en 3 GPIOs finales,
 * posibilitando su visualización en unidad, decena y centena por pantalla.
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
 * | 08/04/2026 | Document creation		                         |
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
/** @def CANTIDAD_BITS 
 * @brief número de bits que tendrá el número.
 */
#define CANTIDAD_BITS 4

/*==================[internal data definition]===============================*/
/** *@struct gpioConf_t
 * @brief Estructura que contiene la configuración de GPIO.
 */

typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;


/*==================[internal functions declaration]=========================*/
/** @fn int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t * bcd_number)
 * @brief Función que convierte el número a un arreglo de BCD.
 * * @param data número a convertir.
 * * @param digits cantidad de dígitos que tendrá el número.
 * * @param bcd_number arreglo de salida con el número en formato BCD.
 */

int8_t convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	for(int i=0; i<digits;i++){

		bcd_number [i] = data%10;
		data/=10;

	}
	return 0;

}

/*==================[internal functions declaration]=========================*/
/** @fn void cambio_estado_gpio(uint32_t bcd_number, gpioConf_t *vector_gpio)
 * @brief Función que cambia el estado del GPIO.
 * * @param bcd_number arreglo con el número en formato BCD.
 * * @param vector_gpio puntero a la dirección de memoria del gpio.
 */

void cambio_estado_gpio(uint8_t bcd_number, gpioConf_t *vector_gpio)
{
	for (int i=0;i<CANTIDAD_BITS;i++)
	{
		if((bcd_number&1<<i)){
			GPIOOn(vector_gpio[i].pin);
		}
		else {
			GPIOOff(vector_gpio[i].pin);
		}
	}
}	

/** @fn void mostrar_en_pantalla(uint32_t dato, uint8_t digitos_salida, gpioConf_t *vector_gpio, gpioConf_t *vector_mapeo)
 * @brief Función que mapea el vector de GPIO generado con cada GPIO final (unidad, decena, centena)
 * y la muestra por la pantalla LCD.
 * 
 * * @param dato número a convertir.
 * * @param digitos_salida cantidad de dígitos que tendrá el número.
 * * @param vector_gpio puntero a la dirección de memoria del gpio (arreglo de 4 gpio).
 * * @param vector_mapeo puntero a la dirección de memoria del gpio mapeado (cada GPIO (3) recibe un arreglo gpio (4)).
 */
void mostrar_en_pantalla(uint32_t dato, uint8_t digitos_salida, gpioConf_t *vector_gpio, gpioConf_t *vector_mapeo)
{
	uint8_t bcd_array[digitos_salida];
	convertToBcdArray(dato, digitos_salida, bcd_array);

	for (int i = 0; i<digitos_salida; i++)
	{
		cambio_estado_gpio(bcd_array[i], vector_gpio);
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
	
	for (int i=0; i<CANTIDAD_BITS; i++)
	{
		GPIOInit(vector[i].pin,vector[i].dir);
	}

	
	cambio_estado_gpio(numero_bcd[0], vector);

	printf("DESARROLLO EJ_6\n");

	gpioConf_t vector_map[]=
	{
		{GPIO_9,GPIO_OUTPUT},
		{GPIO_18,GPIO_OUTPUT},
		{GPIO_19,GPIO_OUTPUT},
	};

	for (int i = 0; i < digitos; i++) GPIOInit(vector_map[i].pin, vector_map[i].dir);
		
	printf("\n");


	mostrar_en_pantalla(dato, digitos, vector, vector_map);


}
/*==================[end of file]============================================*/