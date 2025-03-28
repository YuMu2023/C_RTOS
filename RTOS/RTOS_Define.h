#ifndef _RTOS_DEFINE_H_
#define _RTOS_DEFINE_H_






#define RTOS_PRI_NUM 5
#define RTOS_RAM_SIZE 10240
#define RTOS_A_TIMER_STACK 256


typedef unsigned int rtos_uint32_t;
typedef unsigned short rtos_uint16_t;
typedef unsigned char rtos_uint8_t;
typedef int rtos_int32_t;
typedef short rtoss_int16_t;
typedef char rtos_int8_t;


#define RTOS_NULL ((void*)0)
#define RTOS_TRUE 1
#define RTOS_FALSE 0
#define RTOS_TIMER_TYPE_ONCE 0
#define RTOS_TIMER_TYPE_REPEAT 1

#endif//_RTOS_DEFINE_H_
