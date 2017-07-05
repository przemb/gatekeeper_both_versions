#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "croutine.h"


void ledInit(void);
void uartConfig(void);
void sendChar(char c);
void sendString(const char* s);
void vFlashCoRoutine(CoRoutineHandle_t xHandle, UBaseType_t uxIndex);
void vApplicationIdleHook(void);


// RTOS task
void vStepperMotorTask(void *p);
void vBlinkAndWriteGreenTask(void *p);
void vBlinkAndWriteRedTask(void *p);
void vGatekeeperTask(void *p);

QueueHandle_t xWriteQueue;
SemaphoreHandle_t xSemaphoreGateKeeper;

int main(void){
	
	ledInit();
	uartConfig();
	
	xSemaphoreGateKeeper = xSemaphoreCreateMutex();
	xWriteQueue = xQueueCreate(5, sizeof(char *));
	
	if((xSemaphoreGateKeeper != NULL) && (xWriteQueue != NULL)){
		xTaskCreate(vBlinkAndWriteGreenTask, "Print Green", 128, NULL, 1, NULL);
		xTaskCreate(vBlinkAndWriteRedTask, "Print Red", 128, NULL, 1, NULL);
		xTaskCreate(vGatekeeperTask, "Gatekeeper", 128, NULL, 1, NULL);
		
		
		// Create two coroutines (indexes 0 and 1)
		xCoRoutineCreate(vFlashCoRoutine, 0, 0);
		xCoRoutineCreate(vFlashCoRoutine, 0, 1);

		// Start RTOS scheduler
		vTaskStartScheduler();
	}
	for(;;);
}


void ledInit(){
    GPIO_InitTypeDef  gpio;
    // Configure PA5, push-pull output
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_8| GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &gpio);
	GPIO_Init(GPIOC, &gpio);
}



void vBlinkAndWriteGreenTask(void *p){
	static char * myString = "green\n";
	for (;;){
        xQueueSendToBack(xWriteQueue, &myString, 0);
		GPIOA->ODR ^= GPIO_Pin_5;
		vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vBlinkAndWriteRedTask(void *p){
	static char * myString = "red\n";
	for (;;){
		xQueueSendToBack(xWriteQueue, &myString, 0);
        GPIOC->ODR ^= GPIO_Pin_0;
		vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void vGatekeeperTask(void *p){
	char * pcMessageToPrint;
	
	for(;;){
		xQueueReceive( xWriteQueue, &pcMessageToPrint, portMAX_DELAY);
		sendString(pcMessageToPrint);
	}
}

void vStepperMotorTask(void *p){
	
	
	
}


void vApplicationIdleHook( void ){
        vCoRoutineSchedule();
}


void vFlashCoRoutine(xCoRoutineHandle xHandle, unsigned portBASE_TYPE uxIndex){
	static char * myStrings[2] = {"CoroutineGREEN\n", "CoroutineRED\n"};
	static GPIO_TypeDef * myPorts[2] = {GPIOA, GPIOC};
	static uint16_t myPins[2] = {GPIO_Pin_5, GPIO_Pin_0};
	
	
	crSTART(xHandle);

	while(1){
		if(xSemaphoreTake(xSemaphoreGateKeeper, 1000)){
			sendString(myStrings[uxIndex]);
			//myPorts[uxIndex]->ODR ^= myPins[uxIndex];
			xSemaphoreGive(xSemaphoreGateKeeper);
			crDELAY(xHandle, pdMS_TO_TICKS(100));			
		}
	}
	crEND();
}


void uartConfig(){
	GPIO_InitTypeDef gpio;
	USART_InitTypeDef uart;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE); //alternatywne linie IO
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE); //USART2

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_2;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);

	gpio.GPIO_Pin = GPIO_Pin_3;
	gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &gpio);

	USART_StructInit(&uart);
	uart.USART_BaudRate = 9600;
	USART_Init(USART2, &uart);

	USART_Cmd(USART2, ENABLE);
}

void sendString(const char * s){
	while (*s)
		sendChar(*s++);
}

void sendChar(char c){
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	USART_SendData(USART2, c);
}

