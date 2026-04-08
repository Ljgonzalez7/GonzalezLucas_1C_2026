/*! @mainpage guia1_ejercicio4_5_6
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
 * | 14/08/2024 | Document creation		                         |
 *
 * @author Lucas Gonzalez (lucas.gonzalez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <gpio_mcu.h>
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;


/*==================[internal functions declaration]=========================*/
int8_t  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	for(int i=0;i<digits;i++)
	{
		bcd_number[i]=data%10*(i+1);
	}
	return 0;
}

void cambio_estado_gpio(uint8_t bcd_number,gpioConf_t vector_estructuras)
{
	for (int i=0;i<3;i++)
	{
		if((bcd_number&1<<i)){
			GPIOOn(vector_estructuras.pin);
		}
		else {
			GPIOOff(vector_estructuras.pin);
		}
	}
}	

/*==================[external functions definition]==========================*/
void app_main(void){
	printf("Prueba de funcion\n");
	uint32_t dato_real=452;
	uint8_t cantidad_digitos=3;
	uint8_t arreglo1[4]={0};

	convertToBcdArray(dato_real,cantidad_digitos,arreglo1);

	for(int i=0;i<cantidad_digitos;i++)
	{
		printf ("Datos %d\n",arreglo1[i]);
	}
	
	gpioConf_t vector[]=
	{
		{GPIO_20,GPIO_OUTPUT},
		{GPIO_21,GPIO_OUTPUT},
		{GPIO_22,GPIO_OUTPUT},
		{GPIO_23,GPIO_OUTPUT}
	};

	for (int i=0; i<cantidad_digitos; i++)
	{
		GPIOInit(vector[i].pin,vector[i].dir);
	}
	

	cambio_estado_gpio(arreglo1[0],*vector);

}
/*==================[end of file]============================================*/