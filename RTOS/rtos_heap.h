#ifndef _RTOS_HEAP_H
#define _RTOS_HEAP_H

#include "RTOS_Define.h"

//内存块链表节点结构体
typedef struct RTOS_RAM_ListNode {
	rtos_uint32_t size;				//内存块能存放的大小
	rtos_uint32_t used;				//是否被使用
	struct RTOS_RAM_ListNode* pre;		//上一个内存块节点的指针
	struct RTOS_RAM_ListNode* next;		//下一个内存块节点的指针
}RTOS_RAM_ListNode, *RTOS_RAM_pListNode;


//内存信息结构体
typedef struct RTOS_RAM_Info{
	rtos_uint32_t blocks;			//内存块的块数
	rtos_uint32_t useds;			//有多少内存块被使用
	rtos_uint32_t usedSize;		//已被使用的内存大小（Byte）
	rtos_uint32_t spaceSize;		//未被使用的内存大小（Byte）
}RTOS_RAM_Info, *RTOS_RAM_pInfo;


void RTOS_RAM_ListNodeInit(RTOS_RAM_ListNode* node);
void RTOS_RAM_Init();
void RTOS_RAM_GetInfo(RTOS_RAM_Info* info);
void* RTOS_RAM_FirstFitMalloc(rtos_uint32_t size);
void* RTOS_RAM_BestFitMalloc(rtos_uint32_t size);
void RTOS_RAM_Free(void* p);


#endif//_RTOS_HEAP_H
