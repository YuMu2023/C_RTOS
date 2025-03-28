#include "RTOS_API.h"
#include "RTOS_CortexM3.h"

rtos_uint32_t RTOS_IS_FIRST_SCHEDULE_ADDR = 1;
rtos_uint32_t RTOS_CURRENT_THREAD_SP_ADDR = 0;
rtos_uint32_t RTOS_NEXT_THREAD_SP_ADDR = 0;
rtos_uint32_t RTOS_CURRENT_THREAED_ADDR = 0;
rtos_uint32_t RTOS_NEXT_THREAD_ADDR = 0;
rtos_uint32_t RTOS_START_IDLE_THREAD_ADDR = 0;
rtos_uint32_t RTOS_IDLE_THREAD_ENTRY = 0;

RTOS_pThread waitList = RTOS_NULL;
RTOS_pThread waitListEnd = RTOS_NULL;

RTOS_pThread priReadyList[RTOS_PRI_NUM];
RTOS_pThread priReadyListEnd[RTOS_PRI_NUM];

void* RTOS_ThreadStackInit(void* entry, void* stackAddr, rtos_uint32_t stackSize){
	pCORTEX_M3 regGroup;
	regGroup = (pCORTEX_M3)(((rtos_uint32_t)stackAddr & (0xFFFFFFF8)) + stackSize - sizeof(CORTEX_M3));
	int i = 0;
	for(;i<16;i++){
		((rtos_uint32_t*)regGroup)[i] = 0;
	}
	regGroup->pc = (rtos_uint32_t)entry;
	regGroup->psr = 0x01000000ul;
	return (void*)regGroup;
}

void RTOS_CreateStaticThread(RTOS_Thread* thread, void *entry, void *stackAddr, rtos_uint32_t stackSize, rtos_uint32_t id, rtos_uint32_t pri){
	thread->entry = entry;
	thread->stackAddr = stackAddr;
	thread->stackSize = stackSize;
	thread->id = id;
	thread->delayTime = 0;
	thread->pri = pri;
	thread->psp = RTOS_ThreadStackInit(entry, stackAddr, stackSize);
	thread->next = RTOS_NULL;
	thread->prev = RTOS_NULL;
	if(priReadyList[pri] == RTOS_NULL){
		priReadyList[pri] = thread;
		priReadyListEnd[pri] = thread;
	}else{
		priReadyListEnd[pri]->next = thread;
		thread->prev = priReadyListEnd[pri];
		priReadyListEnd[pri] = thread;
	}
}

void RTOS_Start_Thread(){
	RTOS_IS_FIRST_SCHEDULE_ADDR = 0;
	void(*thread_entry)() = (void*)RTOS_IDLE_THREAD_ENTRY;
	thread_entry();
}

void RTOS_CreateStaticIdleThread(RTOS_Thread* thread, void *entry, void *stackAddr, rtos_uint32_t stackSize){
	RTOS_START_IDLE_THREAD_ADDR = (rtos_uint32_t)thread;
	RTOS_IDLE_THREAD_ENTRY = (rtos_uint32_t)entry;
	thread->entry = RTOS_Start_Thread;
	thread->stackAddr = stackAddr;
	thread->stackSize = stackSize;
	thread->id = 0;
	thread->delayTime = 0;
	thread->pri = RTOS_PRI_NUM + 1;
	thread->psp = RTOS_ThreadStackInit(RTOS_Start_Thread, stackAddr, stackSize);
	thread->next = RTOS_NULL;
	thread->prev = RTOS_NULL;
}

void RTOS_DeleteStaticThread(RTOS_Thread* thread){
	if(thread == ((RTOS_pThread)RTOS_START_IDLE_THREAD_ADDR)){
		return;
	}
	if(thread == ((RTOS_pThread)RTOS_CURRENT_THREAED_ADDR)){
		return;
	}
	if(thread == waitList){
		waitList = waitList->next;
	}else if(thread == waitListEnd && waitList != RTOS_NULL){
		waitListEnd = waitListEnd->prev;
	}
	if(thread == priReadyList[thread->pri]){
		priReadyList[thread->pri] = thread->next;
	}else if(thread == priReadyListEnd[thread->pri] && priReadyList[thread->pri] != RTOS_NULL){
		priReadyListEnd[thread->pri] = thread->prev;
	}
	if(thread->next != RTOS_NULL){
		thread->next->prev = RTOS_NULL;
	}
	if(thread->prev != RTOS_NULL){
		thread->prev->next = RTOS_NULL;
	}
	thread->next = RTOS_NULL;
	thread->prev = RTOS_NULL;
}

RTOS_Thread* RTOS_CreateDynamicThread(void *entry, rtos_uint32_t stackSize, rtos_uint32_t id, rtos_uint32_t pri){
	RTOS_Thread* thread = RTOS_NULL;
	void* stackAddr = RTOS_NULL;
	thread = RTOS_RAM_BestFitMalloc(sizeof(RTOS_Thread));
	if(thread == RTOS_NULL){
		return RTOS_NULL;
	}
	stackAddr = RTOS_RAM_BestFitMalloc(stackSize);
	if(stackAddr == RTOS_NULL){
		RTOS_RAM_Free(thread);
		return RTOS_NULL;
	}
	RTOS_CreateStaticThread(thread, entry, stackAddr, stackSize, id, pri);
	return thread;
}

void RTOS_DeleteDynamicThread(RTOS_Thread* thread){
	RTOS_CloseSystemInterrupt();
	RTOS_DeleteStaticThread(thread);
	RTOS_RAM_Free(thread->stackAddr);
	RTOS_RAM_Free(thread);
	RTOS_OpenSystemInterrupt();
}

void RTOS_CurrentThreadIntoReadyList(){
	if(RTOS_CURRENT_THREAED_ADDR == RTOS_START_IDLE_THREAD_ADDR){
		return;
	}
	RTOS_pThread thread = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
	if(priReadyList[thread->pri] == RTOS_NULL){
		priReadyList[thread->pri] = thread;
		priReadyListEnd[thread->pri] = thread;
	}else{
		priReadyListEnd[thread->pri]->next = thread;
		thread->prev = priReadyListEnd[thread->pri];
		priReadyListEnd[thread->pri] = thread;
	}
}

void RTOS_DesignativeThreadIntoReadyList(RTOS_pThread thread){
	if((rtos_uint32_t)thread == RTOS_START_IDLE_THREAD_ADDR){
		return;
	}
	if(priReadyList[thread->pri] == RTOS_NULL){
		priReadyList[thread->pri] = thread;
		priReadyListEnd[thread->pri] = thread;
	}else{
		priReadyListEnd[thread->pri]->next = thread;
		thread->prev = priReadyListEnd[thread->pri];
		priReadyListEnd[thread->pri] = thread;
	}
}

void RTOS_Schedule(){
	int i = 0;
	int shouldIdle = 1;
	rtos_uint32_t readyPri = 0;
	RTOS_CloseSystemInterrupt();
	for(;i<RTOS_PRI_NUM;i++){
		if(priReadyList[i] != RTOS_NULL){
			readyPri = i;
			shouldIdle = 0;
			break;
		}
	}
	if(shouldIdle){
		if(RTOS_CURRENT_THREAED_ADDR == RTOS_START_IDLE_THREAD_ADDR){
			RTOS_OpenSystemInterrupt();
			return;
		}else{
			RTOS_NEXT_THREAD_ADDR = RTOS_START_IDLE_THREAD_ADDR;
			RTOS_NEXT_THREAD_SP_ADDR = (rtos_uint32_t)(&(((RTOS_pThread)RTOS_START_IDLE_THREAD_ADDR)->psp));
		}
	}else{
		RTOS_pThread thread = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
		RTOS_CURRENT_THREAD_SP_ADDR = (rtos_uint32_t)(&(thread->psp));
		RTOS_NEXT_THREAD_ADDR = (rtos_uint32_t)(priReadyList[readyPri]);
		RTOS_NEXT_THREAD_SP_ADDR = (rtos_uint32_t)(&(priReadyList[readyPri]->psp));
		priReadyList[readyPri] = priReadyList[readyPri]->next;
		if(priReadyList[readyPri] != RTOS_NULL){
			priReadyList[readyPri]->prev = RTOS_NULL;
		}
		((RTOS_pThread)RTOS_NEXT_THREAD_ADDR)->next = RTOS_NULL;
		((RTOS_pThread)RTOS_NEXT_THREAD_ADDR)->prev = RTOS_NULL;
	}
	RTOS_NormalSchedule();
}

void RTOS_ThreadSchedule(){
	RTOS_DesignativeThreadIntoReadyList((RTOS_pThread)RTOS_CURRENT_THREAED_ADDR);
	RTOS_Schedule();
}

void RTOS_Init(){
	int i=0;
	for(;i<RTOS_PRI_NUM;i++){
		priReadyList[i] = RTOS_NULL;
		priReadyListEnd[i] = RTOS_NULL;
	}
	RTOS_RAM_Init();
	Cortex_M3_SysTick_Init();
}

void RTOS_Start(){
	RTOS_CURRENT_THREAED_ADDR = RTOS_START_IDLE_THREAD_ADDR;
	RTOS_CURRENT_THREAD_SP_ADDR = (rtos_uint32_t)(&(((RTOS_pThread)RTOS_START_IDLE_THREAD_ADDR)->psp));
	RTOS_NEXT_THREAD_ADDR = RTOS_CURRENT_THREAED_ADDR;
	RTOS_NEXT_THREAD_SP_ADDR = RTOS_CURRENT_THREAD_SP_ADDR;
	RTOS_FirstSchedule();
}

void RTOS_SetPri(RTOS_Thread* thread, rtos_uint32_t pri){
	RTOS_CloseSystemInterrupt();
	if((rtos_uint32_t)thread == RTOS_CURRENT_THREAED_ADDR){
		thread->pri = pri;
		RTOS_OpenSystemInterrupt();
		return;
	}
	RTOS_Thread* index = priReadyList[thread->pri];
	while(index != RTOS_NULL){
		if(index == thread){
			if(index == priReadyList[thread->pri]){
				priReadyList[thread->pri] = index->next;
			}else if(index == priReadyListEnd[thread->pri] && priReadyList[thread->pri] != RTOS_NULL){
				priReadyListEnd[thread->pri] = index->prev;
			}
			if(index->prev != RTOS_NULL){
				index->prev->next = index->next;
			}
			if(index->next != RTOS_NULL){
				index->next->prev = index->prev;
			}
			index->next = RTOS_NULL;
			index->prev = RTOS_NULL;
			index->pri = pri;
			if(priReadyList[index->pri] == RTOS_NULL){
				priReadyList[index->pri] = index;
				priReadyListEnd[index->pri] = index;
			}else{
				priReadyListEnd[index->pri]->next = index;
				index->prev = priReadyListEnd[index->pri];
				priReadyListEnd[index->pri] = index;
			}
		}
		index = index->next;
	}
	thread->pri = pri;
	RTOS_OpenSystemInterrupt();
}

void RTOS_DeleyMs(rtos_uint32_t ms){
	RTOS_pThread thread = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
	thread->delayTime = ms;
	thread->next = RTOS_NULL;
	thread->prev = RTOS_NULL;
	RTOS_CloseSystemInterrupt();
	if(waitList == RTOS_NULL){
		waitList = thread;
		waitListEnd = thread;
	}else{
		waitListEnd->next = thread;
		thread->prev = waitListEnd;
		waitListEnd = thread;
	}
	RTOS_Schedule();
}

void RTOS_EnterCritical(){
	RTOS_CloseAllInterrupt();
}

void RTOS_ExitCritical(){
	RTOS_OpenAllInterrupt();
}

void RTOS_InitSemaphore(RTOS_Semaphore* s, rtos_int32_t value){
	s->value = value;
	s->waitList = RTOS_NULL;
}

void RTOS_P(RTOS_Semaphore* s){
	RTOS_CloseSystemInterrupt();
	s->value -= 1;
	if(s->value >= 0){
		RTOS_OpenSystemInterrupt();
		return;
	}
	if(s->waitList == RTOS_NULL){
		s->waitList = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
	}else{
		((RTOS_pThread)RTOS_CURRENT_THREAED_ADDR)->next = s->waitList;
		s->waitList->prev = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
		s->waitList = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
	}
	RTOS_Schedule();
}

void RTOS_V(RTOS_Semaphore* s){
	RTOS_CloseSystemInterrupt();
	s->value += 1;
	if(s->value > 0){
		RTOS_OpenSystemInterrupt();
		return;
	}
	if(s->waitList == RTOS_NULL){
		RTOS_OpenSystemInterrupt();
		return;
	}
	RTOS_pThread index = s->waitList;
	RTOS_pThread highestThread = index;
	while(index->next != RTOS_NULL){
		if(index->pri < highestThread->pri){
			highestThread = index;
		}
		index = index->next;
	}
	if(highestThread == s->waitList){
		s->waitList = s->waitList->next;
	}
	if(highestThread->prev != RTOS_NULL){
		highestThread->prev->next = highestThread->next;
	}
	if(highestThread->next != RTOS_NULL){
		highestThread->next->prev = highestThread->prev;
	}
	highestThread->next = RTOS_NULL;
	highestThread->prev = RTOS_NULL;
	RTOS_DesignativeThreadIntoReadyList(highestThread);
	RTOS_OpenSystemInterrupt();
}

void RTOS_InitMessageQueue(RTOS_messageQueue* queue, void* ram, rtos_uint32_t aSize, rtos_int32_t size){
	queue->front = 0;
	queue->rear = 0;
	queue->size = size;
	queue->aSize = aSize;
	queue->data = (rtos_uint8_t*)ram;
	RTOS_InitSemaphore(&(queue->lock), 1);
	RTOS_InitSemaphore(&(queue->resources), 0);
}

rtos_uint32_t RTOS_SendMessageTo(RTOS_messageQueue* queue, const void* const data){
	RTOS_P(&(queue->lock));
	RTOS_CloseSystemInterrupt();
	if(queue->resources.value == queue->size){
		RTOS_V(&(queue->lock));
		RTOS_OpenSystemInterrupt();
		return RTOS_FALSE;
	}
	if(queue->resources.value > 0){
		queue->rear = (queue->rear + 1) % queue->size;
	}
	int i = 0;
	for(;i<queue->aSize;i++){
		queue->data[queue->rear * queue->aSize + i] = ((rtos_uint8_t*)data)[i];
	}
	RTOS_V(&(queue->resources));
	RTOS_V(&(queue->lock));
	RTOS_OpenSystemInterrupt();
	return RTOS_TRUE;
}

rtos_uint32_t RTOS_ReceviceMessageFrom(RTOS_messageQueue* queue, void* data){
	RTOS_P(&(queue->lock));	
	RTOS_CloseSystemInterrupt();
	if(queue->resources.value <= 0){
		RTOS_V(&(queue->lock));
		return RTOS_FALSE;
	}
	int i = 0;
	for(;i<queue->aSize;i++){
		((rtos_uint8_t*)data)[i] = queue->data[queue->front * queue->aSize + i];
	}
	if(queue->front != queue->rear){
		queue->front = (queue->front + 1) % queue->size;
	}
	RTOS_P(&(queue->resources));
	RTOS_V(&(queue->lock));
	RTOS_OpenSystemInterrupt();
	return RTOS_TRUE;
}

rtos_uint32_t RTOS_WaitMessageFrom(RTOS_messageQueue* queue, void* data){
	RTOS_P(&(queue->lock));
	RTOS_CloseSystemInterrupt();
	queue->resources.value -= 1;
	if(queue->resources.value < 0){
		if(queue->resources.waitList == RTOS_NULL){
			queue->resources.waitList = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
		}else{
			((RTOS_pThread)RTOS_CURRENT_THREAED_ADDR)->next = queue->resources.waitList;
			queue->resources.waitList->prev = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
			queue->resources.waitList = (RTOS_pThread)RTOS_CURRENT_THREAED_ADDR;
		}
		RTOS_V(&(queue->lock));
		RTOS_Schedule();
	}else{
		RTOS_V(&(queue->lock));
	}
	RTOS_P(&(queue->lock));
	int i = 0;
	for(;i<queue->aSize;i++){
		((rtos_uint8_t*)data)[i] = queue->data[queue->front * queue->aSize + i];
	}
	if(queue->front != queue->rear){
		queue->front = (queue->front + 1) % queue->size;
	}
	RTOS_V(&(queue->lock));
	RTOS_OpenSystemInterrupt();
	return RTOS_TRUE;
}

rtos_uint32_t RTOS_SendMessageFromIntTo(RTOS_messageQueue* queue, const void* const data){
	if(queue->resources.value >= queue->size){
		return RTOS_FALSE;
	}
	if(queue->lock.value <= 0){
		return RTOS_FALSE;
	}
	if(queue->resources.value > 0){
		queue->rear = (queue->rear + 1) % queue->size;
	}
	int i = 0;
	for(;i<queue->aSize;i++){
		queue->data[queue->rear * queue->aSize + i] = ((rtos_uint8_t*)data)[i];
	}
	RTOS_V(&(queue->resources));
	return RTOS_TRUE;
}

void SysTick_Handler(){
	int i = 0;
	RTOS_pThread index = waitList;
	RTOS_pThread delThread = RTOS_NULL;
	while(index != RTOS_NULL && waitList != RTOS_NULL){
		if(index->delayTime == 0){
			delThread = index;
			index = index->next;
			if(delThread == waitList){
				waitList = waitList->next;
			}else if(delThread == waitListEnd && waitList != RTOS_NULL){
				waitListEnd = waitListEnd->prev;
			}
			if(delThread->next != RTOS_NULL){
				delThread->next->prev = RTOS_NULL;
			}
			if(delThread->prev != RTOS_NULL){
				delThread->prev->next = RTOS_NULL;
			}
			delThread->next = RTOS_NULL;
			delThread->prev = RTOS_NULL;
			if(priReadyList[delThread->pri] == RTOS_NULL){
				priReadyList[delThread->pri] = delThread;
				priReadyListEnd[delThread->pri] = delThread;
			}else{
				priReadyListEnd[delThread->pri]->next = delThread;
				delThread->prev = priReadyListEnd[delThread->pri];
				priReadyListEnd[delThread->pri] = delThread;
			}
		}else{
			index->delayTime -= 1;
			index = index->next;
		}
	}
	if(RTOS_CURRENT_THREAED_ADDR == RTOS_START_IDLE_THREAD_ADDR){
		RTOS_Schedule();
	}else{
		for(;i<(((RTOS_pThread)RTOS_CURRENT_THREAED_ADDR)->pri);i++){
			if(priReadyList[i] != RTOS_NULL){
				RTOS_CurrentThreadIntoReadyList();
				RTOS_Schedule();
				break;
			}
		}
	}
}

