#ifndef _RTOS_DEFINE_H_
#define _RTOS_DEFINE_H_



//******************************自定义*******************************************

//设置RTOS的优先级数量，数字越小优先级越大（例如，设置5时有0~4的优先数）
#define RTOS_PRI_NUM 5

//设置堆空间大小，单位是Byte，如果不需要可设置为0
#define RTOS_RAM_SIZE 10240

//*******************************************************************************



typedef unsigned int rtos_uint32_t;
typedef unsigned short rtos_uint16_t;
typedef unsigned char rtos_uint8_t;
typedef int rtos_int32_t;
typedef short rtos_int16_t;
typedef char rtos_int8_t;


#define RTOS_NULL ((void*)0)
#define RTOS_TRUE 1
#define RTOS_FALSE 0

#endif//_RTOS_DEFINE_H_
