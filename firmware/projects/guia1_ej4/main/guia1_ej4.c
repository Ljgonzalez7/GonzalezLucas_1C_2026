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
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

int8_t  convertToBcdArray (uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
	for(int i=0; i<digits;i++){

		bcd_number [i] = data%10;
		bcd_number[i+1] = (data/10)%10;

	}
	return 0;

}



/*==================[external functions definition]==========================*/
void app_main(void){
	printf("DESARROLLO EJ_4\n");
	uint32_t dato = 15;
	uint8_t digitos = 2;
	uint8_t *numero_bcd [3]={0};
	convertToBcdArray(dato, digitos, numero_bcd);
	
	for (int i=0; i<digitos;i++){
		print(numero_bcd[i]);
		printf("\n");
	}

}
/*==================[end of file]============================================*/