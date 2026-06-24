/*! @mainpage Proyecto Integrador - González Lucas
 *
 * @section genDesc General Description
 *
 * Proyecto integrador: Sistema de sensado para evaluar movilidad articular en miembro inferior.
 * Permite optimizar el test de dorsiflexión de tobillo utilizando sensores de distancia, pantalla LCD, 
 * leds sincronizados, timers y comunicación UART. Ofrece un reporte del estudio y un registro histórico.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
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
 * *
 * @section changelog Changelog
 *
 * |   Date     | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 03/06/2026 | Document creation                              |
 *
 * @author Lucas González (lucas.gonzalez@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"  
#include "gpio_mcu.h"
#include "led.h"
#include "switch.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/
/** @def CONFIG_BLINK_PERIOD_LEDS
 * @brief Periodo de parpadeo de leds durante calentamiento (ms).
 */
#define CONFIG_BLINK_PERIOD_LEDS 500

/** @def TIMER_SENSADO_US
 * @brief Periodo de disparo del timer (40 ms implica frecuencia de muestreo 25 Hz).
 */
#define TIMER_SENSADO_US         40000     

/** @def PERIODO_TELEMETRIA_MS
* @brief Intervalo de envío de datos gráficos a la PC (ms).
 */
#define PERIODO_TELEMETRIA_MS    50 

/** @def DELAY_MONITOR
* @brief Retardo estructural entre cadenas de la UART para el buffer del ESP32-C6 (ms).
 */
#define DELAY_MONITOR            45      

/** @def CHEQUEO_TAREAS_CTRL_MS
* @brief Tasa de revisión para reportes/control de tareas (ms).
 */
#define CHEQUEO_TAREAS_CTRL_MS   300       

/** @def BAUD_RATE
 * @brief velocidad de transmisión de datos por segundo.
 */
#define BAUD_RATE                115200    

/** @def RANGO1
 * @brief Umbral superior para clasificación de distancia (cm).
 */
#define RANGO1 12

/** @def RANGO2
 * @brief Umbral inferior para clasificación de distancia (cm).
 */
#define RANGO2 5

/** @def DIST_MIN_RESET
 * @brief Valor inicial para la distancia mínima (cm).
 */
#define DIST_MIN_RESET 30

/** @def MAX_PACIENTES
 * @brief Número máximo de pacientes a registrar en la sesión.
 */
#define MAX_PACIENTES 10

/*==================[internal data definition]===============================*/
/** @var inicio 
 * @brief Variable booleana filtro de iniciar o apagar el sistema.
 */
volatile bool inicio = false;

/** @var medir 
 * @brief Variable booleana filtro de medir o parar medición.
 */
volatile bool medir = false;

/** @var reporte 
 * @brief Variable booleana filtro de generar reporte.
 */
volatile bool reporte = false;

/** @var calentando 
 * @brief Variable booleana filtro acerca del estado de calentamiento.
 */
volatile bool calentando = false;

/** @var pierna_derecha
 * @brief Variable booleana filtro indicador acerca de la pierna que se está evaluando (true = derecha, false = izquierda).
 */
volatile bool pierna_derecha = true;

/** @var distancia_frontal 
 * @brief Variable que almacena la distancia frontal medida (inicial en 0 cm).
 */
uint16_t distancia_frontal = 0; 

/** @var distancia_frontal_min_der
 * @brief variable que almacena la distancia mínima frontal medida para la pierna derecha (inicial en 30 cm).
 */
uint16_t distancia_frontal_min_der = DIST_MIN_RESET; 

/** @var distancia_frontal_min_izq
 * @brief Variable que almacena la distancia mínima frontal medida para la pierna izquierda (inicial en 30 cm).
 */
uint16_t distancia_frontal_min_izq = DIST_MIN_RESET;

/** @var distancia_lateral
 * @brief Variable que almacena la distancia lateral medida (inicial en 0 cm).
 */
uint16_t distancia_lateral = 0;

/** @var asimetria 
 * @brief Variable de cálculo de déficit bilateral absoluto entre miembros (cm).
 */
int16_t asimetria = 0;

/** @struct registro_test_t
 * @brief Estructura de abstracción para persistencia local de los estudios en RAM.
 */
typedef struct {
    uint8_t id_paciente;
    uint16_t min_derecha;
    uint16_t min_izquierda;
    uint16_t asimetria;
} registro_test_t;

registro_test_t historial_clinico[MAX_PACIENTES]; /**< Vector de estructuras de almacenamiento volátil */

uint8_t contador_pacientes = 0; /**< Contador acumulativo de registros guardados */

// Handles de FreeRTOS
TaskHandle_t SensorDistanciaFrontal_task_handle = NULL;
TaskHandle_t Calentamiento_task_handle = NULL;
QueueHandle_t cola_uart_teclas = NULL; // <-- Cola para comunicar la ISR de la UART de forma segura

/*==================[internal functions declaration]=========================*/

/** @fn void actualizarLeds(uint16_t dist_frontal)
* @brief Función que cambia el estado de los leds según distancia medida.
 * * @param dist_frontal valor de distancia medido en cm.
 */
void actualizarLeds(uint16_t dist_frontal) {
    if (dist_frontal > RANGO1) {
        LedOff(LED_1);
        LedOff(LED_2);
        LedOn(LED_3);  
    } 
    else if (dist_frontal <= RANGO1 && dist_frontal > RANGO2) {
        LedOff(LED_1); 
        LedOn(LED_2); 
        LedOff(LED_3);
    } 
    else if (dist_frontal <= RANGO2 && dist_frontal > 0) { 
        LedOn(LED_1); 
        LedOff(LED_2); 
        LedOff(LED_3);
    } 
    else {
        LedOff(LED_1); 
        LedOff(LED_2); 
        LedOff(LED_3);
    }
}

/** @fn static void TareaSensorDistanciaFrontal(void *pvParameter)
 * @brief Tarea que integra el programa de sensado, interacción y mostrado por pantalla.
 */
static void TareaSensorDistanciaFrontal(void *pvParameter) {    
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (inicio && medir && !calentando) {
            distancia_frontal = HcSr04ReadDistanceInCentimeters();
            actualizarLeds(distancia_frontal);

            if (pierna_derecha) {
                if (distancia_frontal < distancia_frontal_min_der && distancia_frontal > 0) {
                    distancia_frontal_min_der = distancia_frontal;
                }
                LcdItsE0803Write(distancia_frontal_min_der); 
            } 
            else {
                if (distancia_frontal < distancia_frontal_min_izq && distancia_frontal > 0) {
                    distancia_frontal_min_izq = distancia_frontal;
                }
                LcdItsE0803Write(distancia_frontal_min_izq); 
            }
        }
        else if (!inicio) {
            LcdItsE0803Off();
            LedOff(LED_1); 
            LedOff(LED_2); 
            LedOff(LED_3);
        }
    }
}

/** @fn static void TareaCalentamientoArticular(void *pvParameter)
 * @brief Tarea que controla el calentamiento articular por un minuto.
 */
static void TareaCalentamientoArticular(void *pvParameter) {    
    static bool calentamiento_listo = false; 

    while (1) {
        if (inicio && !medir && !calentamiento_listo) {
            printf("--- ARRANCANDO MINUTO DE CALENTAMIENTO ---\n");
            UartSendString(UART_PC, "Estado: Calentamiento en curso...\r\n");
            calentando = true;
            LcdItsE0803Write(111);

            for (int i = 0; i < 120; i++) {
                if (!inicio) break; 

                LedToggle(LED_1);
                LedToggle(LED_2);
                LedToggle(LED_3);

                vTaskDelay(CONFIG_BLINK_PERIOD_LEDS / portTICK_PERIOD_MS); 
            }

            if (inicio) { 
                printf("--- CALENTAMIENTO FINALIZADO. INICIE EL TEST ---\n");
                UartSendString(UART_PC, "Estado: Test En Espera, presione M para medir...\r\n");
                
                LedOff(LED_1); 
                LedOff(LED_2); 
                LedOff(LED_3);
                
                calentando = false;       
                calentamiento_listo = true;
            }
        }
        
        if (!inicio) {
            calentando = false;
            calentamiento_listo = false; 
        }

        vTaskDelay(CHEQUEO_TAREAS_CTRL_MS / portTICK_PERIOD_MS); 
    }
}

/**
 * @brief Función invocada en la interrupción del timer.
 */
void TimerInterrupcion(void *param){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Notificamos y forzamos cambio de contexto inmediato si el scheduler lo requiere
    vTaskNotifyGiveFromISR(SensorDistanciaFrontal_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Función invocada en la interrupción por switch1.
 */
void switch1_interrupcion (void *param){
    inicio = !inicio;   
    calentando = false;
    medir = false;
    if (!inicio) {
        distancia_frontal_min_der = DIST_MIN_RESET; 
        distancia_frontal_min_izq = DIST_MIN_RESET; 
    }
}

/**
 * @brief Función invocada en la interrupción por switch2.
 */
void switch2_interrupcion (void *param){
    if (inicio && !calentando) {
        medir = !medir;
    }
}

/** @fn static void TareaTelemetriaUart(void *pvParameter)
 * @brief Tarea que envia datos de telemetria por UART.
 */
void TareaTelemetriaUart(void *pvParameter) {
    uint16_t distancia_a_enviar = 0;

    while(1){
        if (inicio && medir && !calentando && !reporte) { 
            
            if (distancia_frontal > DIST_MIN_RESET|| distancia_frontal == 0) {
                distancia_a_enviar = DIST_MIN_RESET;
            } else {
                distancia_a_enviar = distancia_frontal;
            }

            UartSendString(UART_PC, ">frente:");
            UartSendString(UART_PC, (char*)UartItoa(distancia_a_enviar, 10));
            UartSendString(UART_PC, "\r\n");
        }
        vTaskDelay(PERIODO_TELEMETRIA_MS / portTICK_PERIOD_MS); 
    }
}

/** @fn static void TareaReporte(void *pvParameter)
 * @brief Tarea que controla el reporte del estudio.
 */
static void TareaReporte(void *pvParameter) {    
    while (1) {
        if (reporte && !medir) {
            UartSendString(UART_PC, "\r\n==================================================\r\n");
            UartSendString(UART_PC, "        INFORME CLINICO - BIOMECANICA WBLT        \r\n");
            UartSendString(UART_PC, "\r\n==================================================\r\n");
            vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS); 

            UartSendString(UART_PC, "\r\n -- Maxima Dorsiflexion PIERNA DERECHA: ");
            if (distancia_frontal_min_der == DIST_MIN_RESET) {
                UartSendString(UART_PC, "No se registraron mediciones validas.\r\n");
            } else {
                UartSendString(UART_PC, (char*)UartItoa(distancia_frontal_min_der, 10));
                UartSendString(UART_PC, " cm a la pared.\r\n");
                
                if (distancia_frontal_min_der <= RANGO2) {
                    UartSendString(UART_PC, "    [EVALUACION]: Movilidad OPTIMA (Rango Excelente).\r\n");
                } else if (distancia_frontal_min_der <= RANGO1) {
                    UartSendString(UART_PC, "    [EVALUACION]: Movilidad REGULAR (Restriccion leve).\r\n");
                } else {
                    UartSendString(UART_PC, "    [EVALUACION]: Movilidad ACORTADA (Restriccion severa).\r\n");
                }
            }
            vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS); 

            UartSendString(UART_PC, " -- Maxima Dorsiflexion PIERNA IZQUIERDA: ");
            if (distancia_frontal_min_izq == DIST_MIN_RESET) {
                UartSendString(UART_PC, "No se registraron mediciones validas.\r\n");
            } else {
                UartSendString(UART_PC, (char*)UartItoa(distancia_frontal_min_izq, 10));
                UartSendString(UART_PC, " cm a la pared.\r\n");
                
                if (distancia_frontal_min_izq <= RANGO2) {
                    UartSendString(UART_PC, "    [EVALUACION]: Movilidad OPTIMA (Rango Excelente).\r\n");
                } else if (distancia_frontal_min_izq <= RANGO1) {
                    UartSendString(UART_PC, "    [EVALUACION]: Movilidad REGULAR (Restriccion leve).\r\n");
                } else {
                    UartSendString(UART_PC, "    [EVALUACION]: Movilidad ACORTADA (Restriccion severa).\r\n");
                }
            }
            vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS); 

    
            if (distancia_frontal_min_der != DIST_MIN_RESET && distancia_frontal_min_izq != DIST_MIN_RESET) {
                asimetria = distancia_frontal_min_der - distancia_frontal_min_izq;
                if (asimetria < 0) asimetria = -asimetria;

                UartSendString(UART_PC, "--------------------------------------------------\r\n");
                UartSendString(UART_PC, " -- Diferencia entre miembros: ");
                UartSendString(UART_PC, (char*)UartItoa(asimetria, 10));
                UartSendString(UART_PC, " cm.\r\n");
                vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS); 

                if (asimetria > 2) {
                    UartSendString(UART_PC, "    [ALERTA CLINICA]: Asimetria significativa (mayor a 2 cm).\r\n");
                    UartSendString(UART_PC, "    Alto riesgo de lesion por compensacion.\r\n");
                } else {
                    UartSendString(UART_PC, "    [EVALUACION]: Deficit Bilateral Normal.\r\n");
                }
            }
            vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS); 

            UartSendString(UART_PC, "\r\n ==================================================\r\n");
            UartSendString(UART_PC, " Fin del reporte. Listo para una nueva evaluacion.\r\n");
            UartSendString(UART_PC, "==================================================\r\n\r\n");


            if (contador_pacientes < MAX_PACIENTES) {
                historial_clinico[contador_pacientes].id_paciente = contador_pacientes + 1;
                historial_clinico[contador_pacientes].min_derecha = distancia_frontal_min_der;
                historial_clinico[contador_pacientes].min_izquierda = distancia_frontal_min_izq;
                historial_clinico[contador_pacientes].asimetria = asimetria;
                contador_pacientes++;
            }

            distancia_frontal_min_der = DIST_MIN_RESET;
            distancia_frontal_min_izq = DIST_MIN_RESET; 
            reporte = false; 
        }

        vTaskDelay(CHEQUEO_TAREAS_CTRL_MS / portTICK_PERIOD_MS); 
    }
}

/** @fn void Funcion_manejo_teclas(void)
 * @brief ISR de la UART que lee bytes entrantes del teclado y los deposita en la cola de FreeRTOS.
 */void Funcion_manejo_teclas(){
    uint8_t tecla;
    UartReadByte(UART_PC, &tecla);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(cola_uart_teclas, &tecla, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/** @fn static void TareaProcesarTeclas(void *pvParameter)
 * @brief Consumidor asincrónico de la cola de caracteres; ejecuta las máquinas de estado lógicas.
 * @param pvParameter Puntero a parámetros de FreeRTOS (no utilizado).
 */
 static void TareaProcesarTeclas(void *pvParameter) {
    uint8_t tecla_recibida;
    while(1) {
        if (xQueueReceive(cola_uart_teclas, &tecla_recibida, portMAX_DELAY) == pdTRUE) {
            
            if(tecla_recibida == 'o' || tecla_recibida == 'O') {
                inicio = !inicio;   
                calentando = false;
                medir = false;
                if (!inicio) {
                    distancia_frontal_min_der = DIST_MIN_RESET;
                    distancia_frontal_min_izq = DIST_MIN_RESET;
                    UartSendString(UART_PC, "Sistema: APAGADO.\r\n");
                } else {
                    UartSendString(UART_PC, "Sistema: ENCENDIDO.\r\n");
                }
            }
            else if(tecla_recibida == 'm' || tecla_recibida == 'M') {
                if (inicio) { 
                    medir = !medir;
                    if(medir) {
                        UartSendString(UART_PC, "Medicion: INICIADA.\r\n");
                    } else {
                        UartSendString(UART_PC, "Medicion: PAUSADA.\r\n");
                    }
                }
            }
            else if(tecla_recibida == 'r' || tecla_recibida == 'R') {
                if (inicio) {
                    if (medir) {
                        UartSendString(UART_PC, "ADVERTENCIA: Detenga la medicion (Tecla M) antes de generar el reporte.\r\n");
                    } else {
                        reporte = true; 
                    }
                }
            }
           else if(tecla_recibida == 'p' || tecla_recibida == 'P') {
                if (inicio) {
                    if (medir) {
                        UartSendString(UART_PC, "ADVERTENCIA: Detenga la medicion (Tecla M) antes de cambiar de pierna.\r\n");
                    } else {
                        pierna_derecha = !pierna_derecha; 
                        if (pierna_derecha) {
                            UartSendString(UART_PC, "-> Evaluando: PIERNA DERECHA\r\n");
                        } else {
                            UartSendString(UART_PC, "-> Evaluando: PIERNA IZQUIERDA\r\n");
                        }
                    }
                }
            }
            else if(tecla_recibida == 'h' || tecla_recibida == 'H') {
                if (inicio) {
                    if (medir) {
                        UartSendString(UART_PC, "ADVERTENCIA: Detenga la medicion antes de consultar el historial.\r\n");
                    } else {
                        UartSendString(UART_PC, "\r\n==================================================\r\n");
                        UartSendString(UART_PC, "          HISTORIAL CLINICO DE EVALUACIONES       \r\n");
                        UartSendString(UART_PC, "==================================================\r\n");
                        vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS);

                        if (contador_pacientes == 0) {
                            UartSendString(UART_PC, " No hay registros cargados en esta sesion.\r\n");
                        } else {
                            for (int i = 0; i < contador_pacientes; i++) {
                                UartSendString(UART_PC, "\r\n Paciente #");
                                UartSendString(UART_PC, (char*)UartItoa(historial_clinico[i].id_paciente, 10));
                                UartSendString(UART_PC, " -> Der: ");
                                UartSendString(UART_PC, (char*)UartItoa(historial_clinico[i].min_derecha, 10));
                                UartSendString(UART_PC, " cm | Izq: ");
                                UartSendString(UART_PC, (char*)UartItoa(historial_clinico[i].min_izquierda, 10));
                                UartSendString(UART_PC, " cm | Asimetria: ");
                                UartSendString(UART_PC, (char*)UartItoa(historial_clinico[i].asimetria, 10));
                                UartSendString(UART_PC, " cm\r\n");
                                vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS);
                            }
                        }
                        UartSendString(UART_PC, "==================================================\r\n\r\n");
                    }
                }
            }
        }
    }
}

/*==================[external functions definition]==========================*/
void app_main(void){
    printf("Inicializacion\n");

    /* Inicialización de periféricos */
    LedsInit();
    SwitchesInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2); 

    /* Inicialización del buffer de comunicación entre hilos */
    cola_uart_teclas = xQueueCreate(10, sizeof(uint8_t));

    /* Configuración e inicialización de timer */
    timer_config_t timer_1 = {
        .timer = TIMER_A, 
        .period = TIMER_SENSADO_US, 
        .func_p = TimerInterrupcion, 
        .param_p = NULL
    };
    TimerInit(&timer_1);

    printf("Ejecucion de programa\n");
    
    /* Creación de tareas */
    xTaskCreate(&TareaSensorDistanciaFrontal, "SensorDist", 2048, NULL, 5, &SensorDistanciaFrontal_task_handle);
    xTaskCreate(&TareaCalentamientoArticular, "CalentamientoArt", 2048, NULL, 4, &Calentamiento_task_handle);
    xTaskCreate(&TareaProcesarTeclas, "ProcesarTeclas", 3072, NULL, 4, NULL);
    xTaskCreate(&TareaTelemetriaUart, "Telemetria", 2048, NULL, 3, NULL);
    xTaskCreate(&TareaReporte, "ReporteTask", 3072, NULL, 5, NULL); 
    
    /* Interrupciones por switch */
    SwitchActivInt(SWITCH_1, switch1_interrupcion, NULL);
    SwitchActivInt(SWITCH_2, switch2_interrupcion, NULL);
    
    /* Inicialización del conteo de timers */
    TimerStart(timer_1.timer);

    /* Configuracion e inicialización de la UART */
    static serial_config_t config_uart_entrada = { 
        .port = UART_PC,    
        .baud_rate = BAUD_RATE,     
        .func_p = Funcion_manejo_teclas,
        .param_p = NULL         
    };
    UartInit(&config_uart_entrada);
}
/*==================[end of file]============================================*/