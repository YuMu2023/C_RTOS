#ifndef _RTOS_API_H_
#define _RTOS_API_H_

#include "RTOS_Define.h"
#include "rtos_heap.h"

typedef struct RTOS_THREAD{
	void *psp;
	void *entry;
	void *stackAddr;
	rtos_uint32_t stackSize;
	rtos_uint32_t id;
	rtos_uint32_t delayTime;
	rtos_uint32_t pri;
	struct RTOS_THREAD *next;
	struct RTOS_THREAD *prev;
}RTOS_Thread, *RTOS_pThread;

typedef struct RTOS_SEMAPHORE{
	volatile rtos_int32_t value;
	struct RTOS_THREAD *waitList;
}RTOS_Semaphore, *RTOS_pSemaphohre;

typedef struct RTOS_MESSAAGEQUEUE{
	rtos_int32_t front;
	rtos_int32_t rear;
	rtos_int32_t size;
	rtos_uint32_t aSize;
	RTOS_Semaphore lock;
	RTOS_Semaphore resources;
	rtos_uint8_t *data;
}RTOS_messageQueue, *RTOS_pMessageQueue;

void RTOS_CreateStaticIdleThread(RTOS_Thread* thread, void *entry,
								void *stackAddr, rtos_uint32_t stackSize);
void RTOS_CreateStaticThread(RTOS_Thread* thread, void *entry,
									void *stackAddr, rtos_uint32_t stackSize,
									rtos_uint32_t id, rtos_uint32_t pri);
void RTOS_DeleteStaticThread(RTOS_Thread* thread);
RTOS_Thread* RTOS_CreateDynamicThread(void *entry, rtos_uint32_t stackSize,
									rtos_uint32_t id, rtos_uint32_t pri);
void RTOS_DeleteDynamicThread(RTOS_Thread* thread);
									
									
void RTOS_Schedule();
void RTOS_DeleyMs(rtos_uint32_t ms);

void RTOS_EnterCritical();
void RTOS_ExitCritical();

void RTOS_InitSemaphore(RTOS_Semaphore* s, rtos_int32_t value);
void RTOS_P(RTOS_Semaphore* s);
void RTOS_V(RTOS_Semaphore* s);

void RTOS_Init();
void RTOS_Start();
void RTOS_SetPri(RTOS_Thread* thread, rtos_uint32_t pri);
								
void RTOS_InitMessageQueue(RTOS_messageQueue* queue, void* ram, rtos_uint32_t aSize, rtos_int32_t size);
rtos_uint32_t RTOS_SendMessageTo(RTOS_messageQueue* queue, const void* const data);
rtos_uint32_t RTOS_ReceviceMessageFrom(RTOS_messageQueue* queue, void* data);
rtos_uint32_t RTOS_WaitMessageFrom(RTOS_messageQueue* queue, void* data);
rtos_uint32_t RTOS_SendMessageFromIntTo(RTOS_messageQueue* queue, const void* const data);


#endif//_RTOS_API_H_

