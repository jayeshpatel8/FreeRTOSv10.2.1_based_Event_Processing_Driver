#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* External Dependency */
/* Adapt all of this API and data strucure as per your RTOS & HW Platform */

/* FreeRTOS Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "event_groups.h"
#include "semphr.h"

#ifndef FALSE
#define TRUE 1U
#define FALSE 0U
#endif
#define true TRUE
#define false FALSE
#define ON 1U
#define OFF 0U

#define T_INTERRUPT_LINE_TIMEOUT 10
#define FAST_MEM_DATA_SECTION
#define OS_SUCCESS 1
#define OS_FALSE 0
#define OS_SEM_TIMEOUT_SUPPORT 100
#define OS_INFINITE portMAX_DELAY
#define OS_EVENT_AUTO_RESET 1

#define OsEventCreate(a,b,c) *a = xEventGroupCreate(),OS_SUCCESS //OS_SUCCESS
#define OsEventWait(a,b,c) 		xEventGroupWaitBits( *a,	/* The event group that contains the event bits being queried. */ \
								0x1,		/* The bit to wait for. */ \
								pdTRUE,		/* Clear the bit on exit. */ \
								pdFALSE,		/* Wait for all the bits (only one in this case anyway). */ \
								c) /* Block indefinitely to wait for the condition to be met. */ //OS_SUCCESS

#define OsEventSet(a) OS_SUCCESS;xEventGroupSetBits(*a,0x1)

#define OsIrqCreate(...) OS_SUCCESS
#define OsIrqUnmask(...) OS_SUCCESS
#define OsIrqMask(...) OS_SUCCESS

#define main_TASK_PRIORITY ( tskIDLE_PRIORITY + 2 )
#define OsThreadCreate(a,b,c,d) xTaskCreate(c,b,configMINIMAL_STACK_SIZE,d,main_TASK_PRIORITY,a) //OS_SUCCESS
#define OsThreadStart(...) OS_SUCCESS //vTaskStartScheduler(),OS_SUCCESS //OS_SUCCESS
#define OsThreadGetCurrent(a) OS_SUCCESS;*a = xTaskGetCurrentTaskHandle()//OS_SUCCESS

#define OsSemCreate(a,b,c,d) OS_SUCCESS; *a = xSemaphoreCreateBinary() //OS_SUCCESS
#define OsSemRelease(a) xSemaphoreGive(*a);
#define OsSemObtain(a,b,c) xSemaphoreTake(*a,1000)
#define OsSemDelete(a) vSemaphoreDelete(*a)

/* For Multi Core */
#define os_atomic_add_U32(a,b) *a+=b
#define os_atomic_sub_U32(a,b) *a-=b
#define os_data_sync_barrier(...)
#define os_spinlock_obtain(...)
#define os_spinlock_release(...)
#define os_spinlock_init(...)

#define memcpy_s(a,b,c,d) memcpy(a,c,d)

typedef uint8_t U8;
typedef uint32_t U32;

//typedef uint8_t BOOL;

typedef uint16_t U16;
typedef int32_t S32;

#define OsEvent EventGroupHandle_t 

#define OsThread TaskHandle_t

#define  OsSem SemaphoreHandle_t

typedef struct {
    U32 data;
}spinlock_t;

typedef struct {
    U32 data;
}OsIrqIsr;

typedef struct {
    BOOL mode;
}t_base_cfg;

extern void Test_simulate_SW_TIMER_interrupt_generation(void);