/*! @mainpage Proyecto Integrador
 *
 * @section genDesc General Description
 *
 * Proyecto integrador: software que permite optimizar el test de dorsiflexión de tobillo utilizando
 * sensores de distancia y sensor de compensación del valgo de rodilla. 
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
#include "freertos/queue.h"  // <-- Agregado para manejo seguro de colas
#include "gpio_mcu.h"
#include "led.h"
#include "switch.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_LEDS 500
#define TIMER_SENSADO_US         40000     /* 40 ms (Frecuencia de muestreo 25 Hz) */
#define PERIODO_TELEMETRIA_MS    50        /* Envío de datos a la PC cada 50 ms */
#define DELAY_MONITOR            20        /* Delay entre textos monitor ms */
#define CHEQUEO_TAREAS_CTRL_MS   300       /* Tasa de revisión para reportes/control */
#define BAUD_RATE                250000    

#define RANGO1 12
#define RANGO2 5
#define DIST_MIN_RESET 30

/*==================[internal data definition]===============================*/
volatile bool inicio = false;
volatile bool medir = false;
volatile bool reporte = false;
volatile bool calentando = false;
volatile bool pierna_derecha = true;

uint16_t distancia_frontal = 0; 
uint16_t distancia_frontal_min_der = DIST_MIN_RESET; 
uint16_t distancia_frontal_min_izq = DIST_MIN_RESET;
uint16_t distancia_lateral = 0;

// Handles de FreeRTOS
TaskHandle_t SensorDistanciaFrontal_task_handle = NULL;
TaskHandle_t Calentamiento_task_handle = NULL;
QueueHandle_t cola_uart_teclas = NULL; // <-- Cola para comunicar la ISR de la UART de forma segura

/*==================[internal functions declaration]=========================*/
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
    else if (dist_frontal <= RANGO2 && dist_frontal > 0) { // <-- Protección ante lecturas en 0
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

static void TareaCalentamientoArticular(void *pvParameter) {    
    static bool calentamiento_listo = false; // <-- Bandera local para saber si ya se hizo en este ciclo

    while (1) {
        /* Si se encendió el sistema, no se midió todavía y no se hizo el calentamiento */
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
                
                calentando = false;       // El calentamiento físico terminó
                calentamiento_listo = true; // Ya se completó el minuto para esta sesión
            }
        }
        
        /* Si el usuario apaga el sistema con 'O', reseteamos todo para el próximo paciente */
        if (!inicio) {
            calentando = false;
            calentamiento_listo = false; 
        }

        vTaskDelay(CHEQUEO_TAREAS_CTRL_MS / portTICK_PERIOD_MS); 
    }
}

void TimerInterrupcion(void *param){
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Notificamos y forzamos cambio de contexto inmediato si el scheduler lo requiere
    vTaskNotifyGiveFromISR(SensorDistanciaFrontal_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ISRs de los Switches físicos
void switch1_interrupcion (void *param){
    inicio = !inicio;   
    calentando = false;
    medir = false;
    if (!inicio) {
        distancia_frontal_min_der = DIST_MIN_RESET; 
        distancia_frontal_min_izq = DIST_MIN_RESET; 
    }
}

void switch2_interrupcion (void *param){
    if (inicio && !calentando) {
        medir = !medir;
    }
}

void TareaTelemetriaUart(void *pvParameter) {
    uint16_t distancia_a_enviar = 0;

    while(1){
        if (inicio && medir && !calentando && !reporte) { 
            
            // Si la distancia medida supera el límite o da un error (0), la clavamos en el máximo
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

static void TareaReporte(void *pvParameter) {    
    while (1) {
        if (reporte && !medir) {
            // 1. Encabezado
            UartSendString(UART_PC, "\r\n==================================================\r\n");
            UartSendString(UART_PC, "        INFORME CLINICO - BIOMECANICA WBLT        \r\n");
            UartSendString(UART_PC, "\r\n==================================================\r\n");
            vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS); // <-- Un respiro de 20ms para vaciar el buffer de hardware

            // 2. Pierna Derecha
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
            vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS); // <-- Otro respiro mecánico para la UART

            // 3. Pierna Izquierda
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

            // 4. Análisis de Asimetría
            if (distancia_frontal_min_der != DIST_MIN_RESET && distancia_frontal_min_izq != DIST_MIN_RESET) {
                int16_t asimetria = distancia_frontal_min_der - distancia_frontal_min_izq;
                if (asimetria < 0) asimetria = -asimetria;

                UartSendString(UART_PC, "--------------------------------------------------\r\n");
                UartSendString(UART_PC, " -- Diferencia entre miembros: ");
                UartSendString(UART_PC, (char*)UartItoa(asimetria, 10));
                UartSendString(UART_PC, " cm.\r\n");

                if (asimetria > 2) {
                    UartSendString(UART_PC, "    [ALERTA CLINICA]: Asimetria significativa \r\n");
                    UartSendString(UART_PC, "    [ALERTA CLINICA]: Asimetria significativa (mayor a 2 cm).\r\n");
                    UartSendString(UART_PC, "    Alto riesgo de lesion por compensacion.\r\n");
                } else {
                    UartSendString(UART_PC, "    [EVALUACION]: Deficit Bilateral Normal.\r\n");
                }
            }
            vTaskDelay(DELAY_MONITOR / portTICK_PERIOD_MS); 

            // 5. Cierre
            UartSendString(UART_PC, "\r\n ==================================================\r\n");
            UartSendString(UART_PC, " Fin del reporte. Listo para una nueva evaluacion.\r\n");
            UartSendString(UART_PC, "==================================================\r\n\r\n");

            // Reseteamos las variables para la próxima prueba limpia
            distancia_frontal_min_der = DIST_MIN_RESET;
            distancia_frontal_min_izq = DIST_MIN_RESET; 
            reporte = false; 
        }

        vTaskDelay(CHEQUEO_TAREAS_CTRL_MS / portTICK_PERIOD_MS); 
    }
}

// ESTA FUNCIÓN ES UNA ISR. ¡Debe ser ultra rápida y no puede imprimir strings!
void Funcion_manejo_teclas(){
    uint8_t tecla;
    UartReadByte(UART_PC, &tecla);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Insertamos el byte leído en la cola de forma segura desde la ISR
    xQueueSendFromISR(cola_uart_teclas, &tecla, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Nueva Tarea Asincrónica dedicada a procesar lo que llega por la cola de la UART
static void TareaProcesarTeclas(void *pvParameter) {
    uint8_t tecla_recibida;
    while(1) {
        // Se queda bloqueada eficientemente consumiendo 0% CPU hasta que entre un carácter
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
                if (inicio) { // <-- Le sacamos el "&& !calentando"
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
                        // Si intenta sacar el reporte en vivo, le avisamos por consola
                        UartSendString(UART_PC, "ADVERTENCIA: Detenga la medicion (Tecla M) antes de generar el reporte.\r\n");
                    } else {
                        reporte = true; // Si la medición estaba pausada, habilita el reporte
                    }
                }
            }
            else if(tecla_recibida == 'p' || tecla_recibida == 'P') {
                if (inicio) {
                    pierna_derecha = !pierna_derecha; 
                    if (pierna_derecha) {
                        UartSendString(UART_PC, "-> Evaluando: PIERNA DERECHA\r\n");
                    } else {
                        UartSendString(UART_PC, "-> Evaluando: PIERNA IZQUIERDA\r\n");
                    }
                }
            }
        }
    }
}

/*==================[external functions definition]==========================*/
void app_main(void){
    printf("Inicializacion\n");
    LedsInit();
    SwitchesInit();
    LcdItsE0803Init();
    HcSr04Init(GPIO_3, GPIO_2); 

    // Inicializamos la cola de FreeRTOS con capacidad para 10 pulsaciones
    cola_uart_teclas = xQueueCreate(10, sizeof(uint8_t));

    /* Inicialización de timers */
    timer_config_t timer_1 = {
        .timer = TIMER_A, 
        .period = TIMER_SENSADO_US, 
        .func_p = TimerInterrupcion, 
        .param_p = NULL
    };
    TimerInit(&timer_1);

    printf("Ejecucion de programa\n");
    // Jerarquía de prioridades balanceada correctamente
    xTaskCreate(&TareaSensorDistanciaFrontal, "SensorDist", 2048, NULL, 5, &SensorDistanciaFrontal_task_handle);
    xTaskCreate(&TareaCalentamientoArticular, "CalentamientoArt", 2048, NULL, 4, &Calentamiento_task_handle);
    xTaskCreate(&TareaProcesarTeclas, "ProcesarTeclas", 3072, NULL, 4, NULL);
    xTaskCreate(&TareaTelemetriaUart, "Telemetria", 2048, NULL, 3, NULL);
    xTaskCreate(&TareaReporte, "ReporteTask", 3072, NULL, 4, NULL); // Prioridad adecuada para que no sature
    
    /* Interrupciones por switch */
    SwitchActivInt(SWITCH_1, switch1_interrupcion, NULL);
    SwitchActivInt(SWITCH_2, switch2_interrupcion, NULL);
    
    /* Inicialización del conteo de timers */
    TimerStart(timer_1.timer);

    static serial_config_t config_uart_entrada = { 
        .port = UART_PC,    
        .baud_rate = BAUD_RATE,     
        .func_p = Funcion_manejo_teclas,
        .param_p = NULL         
    };
    UartInit(&config_uart_entrada);
}
/*==================[end of file]============================================*/