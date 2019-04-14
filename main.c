/*
 * Pi Calculator
 *
 * Created: 20.03.2018 18:32:07
 * Author : Nico
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

#include "ButtonHandler.h"
#include "event_groups.h"
#include "timers.h"
#include <stdio.h>
#include <math.h>
#include <string.h>



extern void vApplicationIdleHook( void );
void PiCalc(void *pvParameters);
void EventBit(void *pvParameters);
void MyTimer(void *pvParameters);
void Interface(void *pvParameters);
void vButtonTask(void *pvParameters);
void Pause(void *pvParameters);
void Count_Time(void *pvParameters);

TimerHandle_t Timer1;
EventGroupHandle_t EventGroup;
EventBits_t action;
EventBits_t stop;
TaskHandle_t PiCalc_task;
TaskHandle_t Interface_task;
TaskHandle_t Pause_task;
TaskHandle_t Count_Time_task;

#define StartBit 1<<0
#define PauseBit 1<<1
#define WeiterBit 1<<2 
#define ResetBit 1<<4
#define TimeBit 1<<5

int sec = 0;
int count = 0;
double dmyPi = 0.0;
double i = 0.0;
char Pi_string[20];
char I_string [20];


void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	Timer1=xTimerCreate( "MyTimer", 10/portTICK_RATE_MS, pdTRUE, (void*)0, MyTimer);
	EventGroup = xEventGroupCreate();
	xTaskCreate( PiCalc, (const char *) "Count_Time", configMINIMAL_STACK_SIZE, NULL, 1, &PiCalc_task );
	xTaskCreate( Interface, (const char *) "Interface", configMINIMAL_STACK_SIZE, (void*)0, 1, &Interface_task);
	xTaskCreate(vButtonTask, (const char *) "btTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate( Pause, (const char *) "Count_Time", configMINIMAL_STACK_SIZE, NULL, 1, &Pause_task);
	xTaskCreate( Count_Time, (const char *) "Count_Time", configMINIMAL_STACK_SIZE, NULL, 1, &Count_Time_task );
	
	
	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"Pi Calculation V1.0");
	
	vTaskStartScheduler();
	return 0;
}

void MyTimer (TimerHandle_t Timer1)
{
	
	xEventGroupSetBits(EventGroup,TimeBit);
		
}
void Count_Time (void *pvParameters)
{
	for (;;)
	{
		xEventGroupWaitBits( EventGroup, TimeBit, pdTRUE, pdFALSE, portMAX_DELAY);
		count++;
		if (count == 100)
		{
			sec++;
			count = 0;
		}
	}
}

void Pause (void *pvparameters)
{
	for (;;)
	{
		stop= xEventGroupWaitBits( EventGroup, PauseBit|WeiterBit|ResetBit, pdTRUE, pdFALSE, portMAX_DELAY);
		if((stop & PauseBit) !=0 )
		{
			vTaskSuspend (PiCalc_task);
		}
		else if ((stop & WeiterBit) !=0 )
		{
			vTaskResume(PiCalc_task);
		}
		else if ((stop & ResetBit) !=0 )
		{
			dmyPi = 0.0;
			i = 0.0;
			sec = 0;
		}
	}
}
	
void PiCalc (void *pvParameters)
{
	for (;;)
	{		
		action= xEventGroupWaitBits( EventGroup, StartBit, pdTRUE, pdFALSE, portMAX_DELAY);
		if ((action & StartBit) !=0 )
		{
			dmyPi += 1.0;
			for ( i; i <300000 ; i++)  
			{ 
				dmyPi = dmyPi - 1/(3+i*4) + 1/(5+i*4);
				if (i == 125500)
				{
					xTimerStop(Timer1,0);
				}
			}
			
		}
	}
}

void Interface (void *pvParameters)
{
	for (;;)
	{
		dtostrf(dmyPi*4,1,7,Pi_string);
		dtostrf(i,1,0,I_string);
		vDisplayWriteStringAtPos(1,0,"Pi=%s",Pi_string);
		vDisplayWriteStringAtPos(2,0,"Iterations=%s",I_string);
		vDisplayWriteStringAtPos(3,0,"Duration:%d",sec);
		vTaskDelay(500/portTICK_RATE_MS);
	}
}

void vButtonTask(void *pvParameters) {
	initButtons();
	vTaskDelay(3000);
	vDisplayClear();
	for(;;) {
		updateButtons();
		
		if(getButtonPress(BUTTON1) == SHORT_PRESSED) {
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0, "Start Calculation");
			xEventGroupSetBits(EventGroup,StartBit);
			xTimerStart(Timer1,0);
			//vTaskResume(PiCalc_task);
		}
		if(getButtonPress(BUTTON2) == SHORT_PRESSED) {
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0, "Pause Calculation");
			xEventGroupSetBits(EventGroup,PauseBit);
			xTimerStop(Timer1,0);
		}
		if(getButtonPress(BUTTON3) == SHORT_PRESSED) {
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0, "Start Calculation");
			xEventGroupSetBits(EventGroup,WeiterBit);
			xTimerStart(Timer1,0);
		}
		if(getButtonPress(BUTTON4) == SHORT_PRESSED) {
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0, "Reset");
			xEventGroupSetBits(EventGroup,ResetBit);
			xTimerReset(Timer1,0);
			xTimerStop(Timer1,0);
		}
		vTaskDelay((1000/BUTTON_UPDATE_FREQUENCY_HZ)/portTICK_RATE_MS);
	}
}
