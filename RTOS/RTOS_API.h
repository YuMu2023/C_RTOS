#ifndef _RTOS_API_H_
#define _RTOS_API_H_

#include "RTOS_Define.h"
#include "rtos_heap.h"


//使用rtos_heap.h的函数均需要进入临界区


//线程结构体
typedef struct RTOS_THREAD{
	void *psp;//栈指针
	void *entry;//线程入口地址
	void *stackAddr;//线程栈地址
	rtos_uint32_t stackSize;//栈大小
	rtos_uint32_t id;//线程id，暂时无用
	rtos_uint32_t delayTime;//非阻塞延时时间
	rtos_uint32_t pri;//优先级
	struct RTOS_THREAD *next;
	struct RTOS_THREAD *prev;
}RTOS_Thread, *RTOS_pThread;


//信号量结构体
typedef struct RTOS_SEMAPHORE{
	volatile rtos_int32_t value;//资源量
	struct RTOS_THREAD *waitList;//等待列表
}RTOS_Semaphore, *RTOS_pSemaphohre;


//消息队列（循环队列）
typedef struct RTOS_MESSAAGEQUEUE{
	rtos_int32_t front;//队头下标
	rtos_int32_t rear;//队尾下一个位置下标
	rtos_int32_t size;//队列总大小，Byte
	rtos_uint32_t aSize;//队列单个元素的大小，Byte
	RTOS_Semaphore lock;//锁
	RTOS_Semaphore resources;//资源信号量，用于统计资源量以及等待资源的挂载点
	rtos_uint8_t *data;//数据地址
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

