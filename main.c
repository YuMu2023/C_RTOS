#include "stm32f10x.h"
#include "RTOS_API.h"
#include "Serial.h"

RTOS_pThread thread_1;
RTOS_pThread thread_2;
RTOS_pThread thread_3;

RTOS_messageQueue usart_message;//串口消息队列
RTOS_Semaphore serial_device_lock;//串口锁
RTOS_Semaphore serial_resource;//串口资源

void delay();//阻塞延时
void start_thread_entry();//空闲/启动线程入口
void thread_1_entry();//线程1入口
void thread_2_entry();//线程2入口
void thread_3_entry();//线程3入口

int main(){
	
	Serial_Init();
	Serial_SendString("Start\r\n");
	
	RTOS_Init();
	//创建启动/空闲线程
	RTOS_CreateStaticIdleThread(RTOS_RAM_FirstFitMalloc(sizeof(RTOS_Thread)),\
								start_thread_entry,\
								RTOS_RAM_FirstFitMalloc(512),\
								512);
	RTOS_Start();
	
	while(1){
		Serial_SendString("ERROR\r\n");
		delay();
	}
	
}

void start_thread_entry(){
	
	RTOS_EnterCritical();
	
	//创建3个线程
	thread_1 = RTOS_CreateDynamicThread(thread_1_entry, 512, 1, 1);
	thread_2 = RTOS_CreateDynamicThread(thread_2_entry, 512, 2, 2);
	thread_3 = RTOS_CreateDynamicThread(thread_3_entry, 512, 3, 2);
	
	//初始化串口消息队列，共128字节的串口缓冲区
	RTOS_InitMessageQueue(&usart_message,\
						  RTOS_RAM_BestFitMalloc(sizeof(char)*128),\
						  sizeof(char),\
						  128);
	//初始化串口锁
	RTOS_InitSemaphore(&serial_device_lock, 1);
	//初始化串口资源，没有资源即0
	RTOS_InitSemaphore(&serial_resource, 0);
	
	RTOS_ExitCritical();
	RTOS_Schedule();
	
	//系统空闲时执行内容
	for(;;){
		RTOS_P(&serial_device_lock);
		Serial_SendString("idle\r\n");
		RTOS_V(&serial_device_lock);
		for(int i=0;i<3000;i++)
			for(int j=0;j<1000;j++);
	}
}


//线程1接收串口数据并回送显示
void thread_1_entry(){
	uint8_t data = 0;
	for(;;){
		RTOS_P(&serial_resource);//等待串口资源
		RTOS_P(&serial_device_lock);//锁上串口设备
		Serial_SendString("I am thread 1, I am reprintting.\r\n");
		while(RTOS_ReceviceMessageFrom(&usart_message, &data)){
			Serial_SendByte(data);//将收到的数据发送回去
		}
		Serial_SendString("\r\n");
		RTOS_V(&serial_device_lock);//释放锁
	}
}


//线程2每隔1s向串口发送一次堆信息
void thread_2_entry(){
	RTOS_RAM_Info ram_info;
	for(;;){
		RTOS_DeleyMs(1000);
		RTOS_EnterCritical();
		RTOS_RAM_GetInfo(&ram_info);//获取当前堆信息
		RTOS_ExitCritical();
		RTOS_P(&serial_device_lock);
		Serial_SendString("I am thread 2. \r\n");
		Serial_SendString("RTOS RAM used size: ");
		Serial_SendUnsigned(ram_info.usedSize, 8);
		Serial_SendString(" byte\r\n");
		RTOS_V(&serial_device_lock);
	}
}


//线程3每隔500ms向串口发送一次数据
void thread_3_entry(){
	for(;;){
		RTOS_DeleyMs(500);
		RTOS_P(&serial_device_lock);
		Serial_SendString("I am thread 3. \r\n");
		RTOS_V(&serial_device_lock);
	}
}


//串口中断，将收到的数据通过消息队列发送给thread_1
void USART1_IRQHandler(){
	static uint8_t shouldEnableIDLE = RTOS_TRUE;
	uint8_t serial_data = 0;
	if(USART_GetITStatus(USART1, USART_IT_RXNE)){
		serial_data = USART_ReceiveData(USART1);
		RTOS_SendMessageFromIntTo(&usart_message, &serial_data);//数据放入消息队列
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		if(shouldEnableIDLE){
			USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
			shouldEnableIDLE = RTOS_FALSE;
		}
	}
	if(USART_GetITStatus(USART1, USART_IT_IDLE)){
		RTOS_V(&serial_resource);//idle中断（即这次串口数据接收完毕），放出一个串口数据资源
		USART_ITConfig(USART1, USART_IT_IDLE, DISABLE);
		shouldEnableIDLE = RTOS_TRUE;
	}
}


//阻塞延时?ms
void delay(){
	for(int i=0;i<1000;i++){
		for(int j=0;j<10000;j++);
	}
}


