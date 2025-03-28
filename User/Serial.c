#include "stm32f10x.h"

void Serial_Init(){

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef gpioa;
	gpioa.GPIO_Mode = GPIO_Mode_AF_PP;
	gpioa.GPIO_Pin = GPIO_Pin_9;
	gpioa.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpioa);
	gpioa.GPIO_Pin = GPIO_Pin_10;
	gpioa.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &gpioa);
	
	USART_InitTypeDef usart1;
	usart1.USART_BaudRate = 115200;
	usart1.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart1.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	usart1.USART_Parity = USART_Parity_No;
	usart1.USART_StopBits = USART_StopBits_1;
	usart1.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &usart1);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = USART1_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 1;
	nvic.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&nvic);
	
	USART_Cmd(USART1, ENABLE);
	
}

void Serial_SendByte(unsigned char b){

	USART_SendData(USART1, b);
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	
}

void Serial_SendString(const char*str){

	uint32_t i=0;
	while(str[i]!='\0'){
		Serial_SendByte(str[i]);
		i++;
	}
	
}

void Serial_SendUnsigned(unsigned int num, int n){
	
	int t = 1;
	int i = 0;
	for(i = 0; i < n - 1; i++){
		t *= 10;
	}
	while(n--){
		Serial_SendByte('0' + ((num / t) % 10));
		t /= 10;
	}
	
}
