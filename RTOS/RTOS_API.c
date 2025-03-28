#include "RTOS_API.h"
#include "RTOS_CortexM3.h"

rtos_uint32_t RTOS_IS_FIRST_SCHEDULE_ADDR = 1;//是否为第一次调度
rtos_uint32_t RTOS_CURRENT_THREAD_SP_ADDR = 0;//当前线程栈指针的地址
rtos_uint32_t RTOS_NEXT_THREAD_SP_ADDR = 0;//下一个线程栈指针的地址（仅在切换上下文时有效）
rtos_uint32_t RTOS_CURRENT_THREAED_ADDR = 0;//当前线程地址
rtos_uint32_t RTOS_NEXT_THREAD_ADDR = 0;//下一个线程地址（仅在切换上下文时有效）
rtos_uint32_t RTOS_START_IDLE_THREAD_ADDR = 0;//空闲/启动 线程地址
rtos_uint32_t RTOS_IDLE_THREAD_ENTRY = 0;//空闲/启动 线程入口

//等待链表
RTOS_pThread waitList = RTOS_NULL;
RTOS_pThread waitListEnd = RTOS_NULL;

//就绪链表
RTOS_pThread priReadyList[RTOS_PRI_NUM];
RTOS_pThread priReadyListEnd[RTOS_PRI_NUM];

 
 
/**
*	@brief 初始化线程栈，内存8字节对齐后，根据cpu中断规则将寄存器按对应的方式入栈
*	@param entry	 (void *) 			线程入口地址
*		   stackAddr (void *) 			线程栈起始地址
*		   stackSize (rtos_uint32_t) 	线程栈大小，Byte
*	@return (void *) 返回psp，即数据入栈后的栈指针位置
**/
void* RTOS_ThreadStackInit(void* entry, void* stackAddr, rtos_uint32_t stackSize){
	pCORTEX_M3 regGroup;
	//8字节内存对齐，即栈底地址低3位为0
	regGroup = (pCORTEX_M3)(((rtos_uint32_t)stackAddr & (0xFFFFFFF8)) + stackSize - sizeof(CORTEX_M3));
	int i = 0;
	for(;i<16;i++){
		((rtos_uint32_t*)regGroup)[i] = 0;
	}
	regGroup->pc = (rtos_uint32_t)entry;
	regGroup->psr = 0x01000000ul;
	return (void*)regGroup;
}


/**
*	@brief 静态创建线程，创建后根据优先级插入就绪链表
*	@param thread 		(RTOS_Thread *) 	线程结构体指针
*		   entry 		(void *) 			线程入口地址
*		   stackAddr 	(void *) 			线程栈起始地址
*		   stackSize 	(rtos_uint32_t) 	线程栈大小，Byte
*		   id 			(rtos_uint32_t) 	线程id
*		   pri 			(rtos_uint32_t) 	线程优先级
*	@return void
**/
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


/**
*	@brief RTOS启动后最先执行的函数，即IDLE线程执行该函数后跳转到设定的IDLE函数
*	@param  void
*	@return void
**/
void RTOS_Start_Thread(){
	RTOS_IS_FIRST_SCHEDULE_ADDR = 0;
	void(*thread_entry)() = (void*)RTOS_IDLE_THREAD_ENTRY;
	thread_entry();
}


/**
*	@brief 静态创建空闲/启动线程，优先级为RTOS_PRI_NUM+1（确保最小），该线程会先执行RTOS_Start_Thread在到entry
*	@param thread 		(RTOS_Thread *) 	线程结构体指针
*		   entry 		(void *) 			线程入口地址
*		   stackAddr 	(void *) 			线程栈起始地址
*		   stackSize 	(rtos_uint32_t) 	线程栈大小，Byte
*	@return void
**/
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


/**
*	@brief 删除通过静态方式创建的线程，即把线程从所有地方（链表）拿出
*		   不允许删除空闲/启动线程，不允许线程自己删除自己
*	@param thread 		(RTOS_Thread *) 	线程结构体指针
*	@return void
**/
void RTOS_DeleteStaticThread(RTOS_Thread* thread){
	if(thread == ((RTOS_pThread)RTOS_START_IDLE_THREAD_ADDR)){
		return;
	}
	if(thread == ((RTOS_pThread)RTOS_CURRENT_THREAED_ADDR)){
		return;
	}
	//从等待链表删除
	if(thread == waitList){
		waitList = waitList->next;
	}else if(thread == waitListEnd && waitList != RTOS_NULL){
		waitListEnd = waitListEnd->prev;
	}
	//从就绪链表删除
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


/**
*	@brief 动态创建线程，创建后根据优先级插入就绪链表
*	@param thread 		(RTOS_Thread *) 	线程结构体指针
*		   entry 		(void *) 			线程入口地址
*		   stackSize 	(rtos_uint32_t) 	线程栈大小，Byte
*		   id 			(rtos_uint32_t) 	线程id
*		   pri 			(rtos_uint32_t) 	线程优先级
*	@return (RTOS_Thread *) 返回创建好的线程结构体指针
**/
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


/**
*	@brief 删除通过动态方式创建的线程，即把线程从所有地方（链表）拿出，同时释放空间，传入的指针不再有效
*		   不允许删除空闲/启动线程，不允许线程自己删除自己，否则会造成OS错误
*		   不需要进入临界区
*	@param thread 		(RTOS_Thread *) 	线程结构体指针
*	@return void
**/
void RTOS_DeleteDynamicThread(RTOS_Thread* thread){
	RTOS_CloseSystemInterrupt();
	RTOS_DeleteStaticThread(thread);
	RTOS_RAM_Free(thread->stackAddr);
	RTOS_RAM_Free(thread);
	RTOS_OpenSystemInterrupt();
}


/**
*	@brief 将当前线程放入就绪链表（线程执行时其实属于游离的状态，只能通过结构体指针和RTOS_CURRENT_THREAED_ADDR找到）
*	@param void
*	@return void
**/
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


/**
*	@brief 将指定线程放入就绪链表，不考虑线程状态，需要保证线程非运行的游离态
*	@param thread 		(RTOS_Thread *) 	线程结构体指针
*	@return void
**/
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


/**
*	@brief 手动进行一次系统调度，不需要进入临界区
*	@param void
*	@return void
**/
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


/**
*	@brief 将当前线程放入就绪链表后手动系统调度一次，多用于线程执行一次循环后调度一次
*	@param void
*	@return void
**/
void RTOS_ThreadSchedule(){
	RTOS_DesignativeThreadIntoReadyList((RTOS_pThread)RTOS_CURRENT_THREAED_ADDR);
	RTOS_Schedule();
}


/**
*	@brief 初始化RTOS
*	@param void
*	@return void
**/
void RTOS_Init(){
	int i=0;
	for(;i<RTOS_PRI_NUM;i++){
		priReadyList[i] = RTOS_NULL;
		priReadyListEnd[i] = RTOS_NULL;
	}
	RTOS_RAM_Init();//初始化堆
	Cortex_M3_SysTick_Init();//初始化系统时钟
}


/**
*	@brief 启动RTOS
*	@param void
*	@return void
**/
void RTOS_Start(){
	RTOS_CURRENT_THREAED_ADDR = RTOS_START_IDLE_THREAD_ADDR;
	RTOS_CURRENT_THREAD_SP_ADDR = (rtos_uint32_t)(&(((RTOS_pThread)RTOS_START_IDLE_THREAD_ADDR)->psp));
	RTOS_NEXT_THREAD_ADDR = RTOS_CURRENT_THREAED_ADDR;
	RTOS_NEXT_THREAD_SP_ADDR = RTOS_CURRENT_THREAD_SP_ADDR;
	RTOS_FirstSchedule();
}


/**
*	@brief 设置线程优先级，不需要进入临界区
*	@param thread 		(RTOS_Thread *) 	线程结构体指针
*		   pri 			(rtos_uint32_t) 	线程优先级
*	@return void
**/
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
			break;
		}
		index = index->next;
	}
	thread->pri = pri;
	RTOS_OpenSystemInterrupt();
}


/**
*	@brief 线程非阻塞延时，不需要进入临界区
*	@param ms (rtos_uint32_t) 延时时长(ms)
*	@return void
**/
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


/**
*	@brief 进入临界区，屏蔽所有cpu中断
*	@param void
*	@return void
**/
void RTOS_EnterCritical(){
	RTOS_CloseAllInterrupt();
}


/**
*	@brief 退出临界区，打开所有cpu中断
*	@param void
*	@return void
**/
void RTOS_ExitCritical(){
	RTOS_OpenAllInterrupt();
}


/**
*	@brief 初始化信号量
*	@param s 		(RTOS_Semaphore *)	信号量结构体指针
*		   value 	(rtos_int32_t)		初始资源量
*	@return void
**/
void RTOS_InitSemaphore(RTOS_Semaphore* s, rtos_int32_t value){
	s->value = value;
	s->waitList = RTOS_NULL;
}


/**
*	@brief 获取信号量资源，如果没有则将线程挂载到信号量等待链表上
*		   不需要进入临界区
*	@param s 		(RTOS_Semaphore *)	信号量结构体指针
*	@return void
**/
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


/**
*	@brief 释放信号量资源，将等待链表中优先级最高的线程设为就绪态，中断程序可调用（因为所有中断优先级大于系统优先级）
*		   不需要进入临界区
*	@param s 		(RTOS_Semaphore *)	信号量结构体指针
*	@return void
**/
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


/**
*	@brief 初始化消息队列
*	@param queue 	(RTOS_messageQueue *) 	消息队列结构体指针
*		   ram 		(void *) 				消息队列使用空间地址
*		   aSize 	(rtos_uint32_t) 		消息队列单个元素大小，Byte
*		   size 	(rtos_uint32_t) 		消息队列使用空间大小，Byte
*	@return void
**/
void RTOS_InitMessageQueue(RTOS_messageQueue* queue, void* ram, rtos_uint32_t aSize, rtos_int32_t size){
	queue->front = 0;
	queue->rear = 0;
	queue->size = size;
	queue->aSize = aSize;
	queue->data = (rtos_uint8_t*)ram;
	RTOS_InitSemaphore(&(queue->lock), 1);
	RTOS_InitSemaphore(&(queue->resources), 0);
}


/**
*	@brief 线程发送消息到消息队列，不需要进入临界区
*	@param queue 		(RTOS_messageQueue *) 	消息队列结构体指针
*		   data 		(const void* const) 	消息地址
*	@return rtos_uint32_t 发送成功返回RTOS_TRUE
**/
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


/**
*	@brief 线程从消息队列接收消息，不需要进入临界区
*	@param queue 		(RTOS_messageQueue *) 	消息队列结构体指针
*		   data 		(void *)			 	消息存放地址
*	@return rtos_uint32_t 接收成功返回RTOS_TRUE
**/
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


/**
*	@brief 线程等待消息队列来消息（一直等待），不需要进入临界区
*	@param queue 		(RTOS_messageQueue *) 	消息队列结构体指针
*		   data 		(void *)			 	消息存放地址
*	@return rtos_uint32_t 永远返回RTOS_TRUE
**/
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


/**
*	@brief 从中断程序中发送消息到消息队列
*	@param queue 		(RTOS_messageQueue *) 	消息队列结构体指针
*		   data 		(const void* const) 	消息地址
*	@return rtos_uint32_t 发送成功返回RTOS_TRUE
**/
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


/**
*	@brief 系统定时中断，在不屏蔽该中断下每1ms触发一次
*		   每次都会检查等待链表，将等待链表上的线程delayTime - 1，减为0后将该线程放于就绪链表中
*	@param void
*	@return void
**/
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

