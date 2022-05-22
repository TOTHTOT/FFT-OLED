/*
 * @Description: 
 * @Author: TOTHTOT
 * @Date: 2022-05-22 15:46:32
 * @LastEditTime: 2022-05-22 19:29:36
 * @LastEditors: TOTHTOT
 * @FilePath: \USER\main.c
 */
/* SYSTEM */
#include "sys.h"
#include "delay.h"
/* HARDWARE */
#include "led.h"
#include "dsp.h"
#include "fft.h"
#include "timer.h"
#include "oled.h"
/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"
#include "list.h"
#include "queue.h"
#include "semphr.h"

//任务优先级
#define START_TASK_PRIO 1
//任务堆栈大小
#define START_STK_SIZE 128
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

//任务优先级
#define LED0_TASK_PRIO 2
//任务堆栈大小
#define LED0_STK_SIZE 50
//任务句柄
TaskHandle_t LED0Task_Handler;
//任务函数
void led0_task(void *pvParameters);

//任务优先级
#define LED1_TASK_PRIO 3
//任务堆栈大小
#define LED1_STK_SIZE 50
//任务句柄
TaskHandle_t LED1Task_Handler;
//任务函数
void led1_task(void *pvParameters);

//任务优先级
#define FFT_TASK_PRIO 6
//任务堆栈大小
#define FFT_STK_SIZE 150
//任务句柄
TaskHandle_t FFTTask_Handler;
//任务函数
void FFT_task(void *pvParameters);

//任务优先级
#define OLED_TASK_PRIO 5
//任务堆栈大小
#define OLED_STK_SIZE 250
//任务句柄
TaskHandle_t OLEDTask_Handler;
//任务函数
void oled_task(void *pvParameters);


/*快速傅里叶变换定义*/
#define NPT 256

long IBufLedArray_tmp[NPT/4];

u16 NPT_Cnt=0;

/*定义二值信号量句柄*/
SemaphoreHandle_t FFT_Semaphore;
SemaphoreHandle_t FFTMag_Semaphore;
SemaphoreHandle_t OLED_Semaphore;
/*....................................*/

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); //设置系统中断优先级分组4
	AllClock_Init();								//初始化系统时钟
	ALL_NVIC_Configuration();						//中断配置
	delay_init();									//延时函数初始化
	uart_init(921600);								//初始化串口
	LED_Init();										//初始化LED
	OLED_Init();									//初始化OLED
	OLED_Clear();									//清屏
	OLED_Set_Pos(0, 0);								//设置光标位置
	OLED_WR_Byte(0x01,OLED_DATA);
	// TIM2_Configuration(72 - 1, 44 - 1);				// 100 10k 50 20k  采用率
	// TIM4_Configuration(2500 - 1, 9 - 1);			// 10ms 屏幕刷新
	//创建开始任务
	xTaskCreate((TaskFunction_t)start_task,			 //任务函数
				(const char *)"start_task",			 //任务名称
				(uint16_t)START_STK_SIZE,			 //任务堆栈大小
				(void *)NULL,						 //传递给任务函数的参数
				(UBaseType_t)START_TASK_PRIO,		 //任务优先级
				(TaskHandle_t *)&StartTask_Handler); //任务句柄
	vTaskStartScheduler();							 //开启任务调度
}

//开始任务任务函数
void start_task(void *pvParameters)
{
	taskENTER_CRITICAL(); //进入临界区
	/*初始化信号量*/
	FFT_Semaphore = xSemaphoreCreateBinary();
	if (FFT_Semaphore == NULL)
	{
		printf("FFT_Semaphore Creat err!\r\n");
	}
	FFTMag_Semaphore = xSemaphoreCreateBinary();
	if (FFTMag_Semaphore == NULL)
	{
		printf("FFTMag_Semaphore Creat err!\r\n");
	}
	OLED_Semaphore = xSemaphoreCreateBinary();
	if (OLED_Semaphore == NULL)
	{
		printf("FFTMag_Semaphore Creat err!\r\n");
	}
	//创建LED0任务
	xTaskCreate((TaskFunction_t)led0_task,
				(const char *)"led0_task",
				(uint16_t)LED0_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)LED0_TASK_PRIO,
				(TaskHandle_t *)&LED0Task_Handler);
/* 	//创建LED1任务
	xTaskCreate((TaskFunction_t)led1_task,
				(const char *)"led1_task",
				(uint16_t)LED1_STK_SIZE,
				(void *)NULL,
				(UBaseType_t)LED1_TASK_PRIO,
				(TaskHandle_t *)&LED1Task_Handler); */
	//创建任务
	xTaskCreate((TaskFunction_t )FFT_task,     	
							(const char*    )"FFT_task",   	
							(uint16_t       )FFT_STK_SIZE, 
							(void*          )NULL,				
							(UBaseType_t    )FFT_TASK_PRIO,	
							(TaskHandle_t*  )&FFTTask_Handler);
	//创建任务
	xTaskCreate((TaskFunction_t )oled_task,     	
							(const char*    )"oled_task",   	
							(uint16_t       )OLED_STK_SIZE, 
							(void*          )NULL,				
							(UBaseType_t    )OLED_TASK_PRIO,	
							(TaskHandle_t*  )&OLEDTask_Handler);
	vTaskDelete(StartTask_Handler); //删除开始任务
	taskEXIT_CRITICAL();			//退出临界区
}

//LED0任务函数
void led0_task(void *pvParameters)
{
	while (1)
	{
		LED0 = ~LED0;
		vTaskDelay(500);
	}
}

//LED1任务函数
void led1_task(void *pvParameters)
{
	while (1)
	{
		LED1 = 0;
		vTaskDelay(200);
		LED1 = 1;
		vTaskDelay(800);
	}
}

/**
 * @name: FFT_task
 * @msg: 计算fft
 * @param undefined
 * @return {*}
 */
void FFT_task(void *pvParameters)
{
	printf("FFT_task start!\r\n");
	while (1)
	{
		//等待信号量
		// if (xSemaphoreTake(FFT_Semaphore, portMAX_DELAY) == pdTRUE)
		// {

		printf("FFT_task running!\r\n");
		taskENTER_CRITICAL(); //进入临界区

		// 填入数据
		InitBufInArray();
		// 计算FFT
		cr4_fft_256_stm32(lBufOutArray, lBufInArray, NPT);
		//计算幅值
		GetPowerMag();
		taskEXIT_CRITICAL(); //退出临界区

		//释放信号量,oled更新
		xSemaphoreGive(OLED_Semaphore);
		// }
		delay_ms(1000);
	}
}

/**
 * @name: oled_task
 * @msg: oled任务,显示各种数据
 * @param undefined
 * @return {*}
 */
void oled_task(void *pvParameters)
{
	u16 i = 0;
	printf("oled_task start!\r\n");
	while (1)
	{
		//等待信号量
		if (xSemaphoreTake(OLED_Semaphore, portMAX_DELAY) == pdTRUE)
		{
			/* for (i = 0; i < NPT; i++)
			{
				printf("%6d\r\n", (s32)(lBufInArray[i] >> 16));
			} */
			//显示数据
			printf("  序号   频率   幅值   实部   虚部\r\n");
			for (i = 0; i < NPT / 2; i++)
			{
				// printf("%6d,%6d,%6d,%6d,%6d\r\n", i, Fs * i / NPT, (u32)lBufMagArray[i], (s16)(lBufOutArray[i] & 0x0000ffff), (s16)(lBufOutArray[i] >> 16));
				printf("%6d,%6d,%6d,%6d,%6d\r\n", i, Fs * i / NPT, (u32)lBufMagArray[i], (s16)(lBufOutArray[i] & 0x0000ffff), (s16)(lBufOutArray[i] >> 16));
			}
			lBufMagArray[0] = lBufMagArray[1];
			for (i = 0; i < NPT / 4; i++)
			{
				IBufLedArray_tmp[i] = (((lBufMagArray[2 * i] + lBufMagArray[2 * i + 1]) / 2) * (32 / 100.0));
				if (IBufLedArray_tmp[i] > 31)
					IBufLedArray_tmp[i] = 31;
				oled_draw_line(i, 0, IBufLedArray_tmp[i]);
			}
		}
		delay_ms(10);
	}
}
