#include "rtos_heap.h"

rtos_uint8_t RTOS_RAM[RTOS_RAM_SIZE];
RTOS_RAM_ListNode RTOS_RAM_LIST;

void RTOS_RAM_ListNodeInit(RTOS_RAM_ListNode* node){
	node->next = RTOS_NULL;
	node->pre = RTOS_NULL;
	node->size = 0;
	node->used = 0;
}

//内存初始化。
//会对内存起始地址进行4字节对齐。
//生成一个内存块节点，size为除去节点所需剩余的内存大小（Byte），并连接到头节点。
void RTOS_RAM_Init(){
	rtos_uint32_t start = (rtos_uint32_t)RTOS_RAM;
	rtos_uint32_t a = start;
	start = start & 0xFFFFFFF8ull;
	RTOS_RAM_ListNode* node = (RTOS_RAM_ListNode*)start;
	node->size = (a + RTOS_RAM_SIZE - start) - sizeof(RTOS_RAM_ListNode);
	node->used = 0;
	node->next = RTOS_NULL;
	node->pre = RTOS_NULL;
	RTOS_RAM_LIST.size = 1;
	RTOS_RAM_LIST.used = 0;
	RTOS_RAM_LIST.pre = RTOS_NULL;
	RTOS_RAM_LIST.next = node;
}

void RTOS_RAM_GetInfo(RTOS_RAM_Info* info){
	info->blocks = RTOS_RAM_LIST.size;
	info->useds = RTOS_RAM_LIST.used;
	info->spaceSize = 0;
	info->usedSize = 0;
	RTOS_RAM_ListNode* index = RTOS_RAM_LIST.next;
	while(index != RTOS_NULL){
		if(index->used == 1){
			info->usedSize += index->size;
		}else{
			info->spaceSize += index->size;
		}
		index = index->next;
	}
}



//使用传入内存块的内存。
//index：要使用的内存块指针。
//size ：需要使用的内存大小（Byte）。
//return：内存起始地址。
//如果index->size恰好为size，则将该内存块设置为已使用，返回内存起始地址。
//如果index->size大于size，则判断前后是否为未使用的内存块，会将多余的内存并入前后内存块，返回内存起始地址。
void* RTOS_RAM_USE(RTOS_RAM_ListNode* index, rtos_uint32_t size) {
	void* ret = RTOS_NULL;
	RTOS_RAM_ListNode* newNode = RTOS_NULL;
	RTOS_RAM_ListNode oldNode;
	if (index->size > size) {
		if ((index->next != RTOS_NULL) && (index->next->used == 0)) {
			index->used = 1;
			ret = (void*)((rtos_uint32_t)index + sizeof(RTOS_RAM_ListNode));
			oldNode = *(index->next);
			newNode = (RTOS_RAM_ListNode*)((rtos_uint32_t)ret + size);
			newNode->used = 0;
			newNode->size = index->size - size + oldNode.size;
			newNode->next = oldNode.next;
			newNode->pre = index;
			if (oldNode.next != RTOS_NULL) {
				oldNode.pre->next = newNode;
			}
			index->next = newNode;
			index->size = size;
		}
		else {
			index->used = 1;
			ret = (void*)((rtos_uint32_t)index + sizeof(RTOS_RAM_ListNode));
			if ((index->size - size) > sizeof(RTOS_RAM_ListNode)) {
				newNode = (RTOS_RAM_ListNode*)((rtos_uint32_t)ret + size);
				newNode->used = 0;
				newNode->size = (index->size - size - sizeof(RTOS_RAM_ListNode));
				newNode->pre = index;
				newNode->next = index->next;
				if (index->next != RTOS_NULL) {
					index->next->pre = newNode;
				}
				index->next = newNode;
				index->size = size;
				RTOS_RAM_LIST.size += 1;
			}
		}
	}
	else if (index->size == size) {
		index->used = 1;
		ret = (void*)((rtos_uint32_t)index + sizeof(RTOS_RAM_ListNode));
	}
	return ret;
}


//使用首次匹配算法分配内存。
//size：需要分配的内存大小（Byte）。
//return：内存首地址。
//首次匹配：根据内存块链表获取到第一块可用内存，且该内存块大小大于size，并使用该内存块。
void* RTOS_RAM_FirstFitMalloc(rtos_uint32_t size){
	void* ret = RTOS_NULL;
	if (size == 0) {
		return ret;
	}
	if (RTOS_RAM_LIST.size == RTOS_RAM_LIST.used) {
		return ret;
	}
	RTOS_RAM_ListNode* index = RTOS_RAM_LIST.next;
	while (index != RTOS_NULL) {
		if (index->used == 1) {
			index = index->next;
			continue;
		}
		if (index->size > size || index->size == size) {
			ret = RTOS_RAM_USE(index, size);
			break;
		}
		else {
			index = index->next;
		}
	}
	if (ret != RTOS_NULL) {
		RTOS_RAM_LIST.used += 1;
	}
	return ret;
}



//使用最佳匹配算法分配内存。
//size：需要分配的内存大小（Byte）。
//return：内存首地址。
//最佳匹配：根据内存块链表获取到一块大小和size差值最小的内存块，并使用该内存块。
void* RTOS_RAM_BestFitMalloc(rtos_uint32_t size){
	void* ret = RTOS_NULL;
	if (size == 0) {
		return ret;
	}
	if (RTOS_RAM_LIST.size == RTOS_RAM_LIST.used) {
		return ret;
	}
	RTOS_RAM_ListNode* index = RTOS_RAM_LIST.next;
	RTOS_RAM_ListNode* xp = RTOS_NULL;
	rtos_uint32_t x = 0xFFFFFFFFul;
	while (index != RTOS_NULL) {
		if (index->used == 1) {
			index = index->next;
			continue;
		}
		if (index->size > size || index->size == size) {
			if ((index->size - size) < x) {
				x = index->size - size;
				xp = index;
			}
		}
		index = index->next;
	}
	if (xp != RTOS_NULL) {
		ret = RTOS_RAM_USE(xp, size);
	}
	if (ret != RTOS_NULL) {
		RTOS_RAM_LIST.used += 1;
	}
	return ret;
}



//释放通过分配得到的内存。
//p：将要释放内存的首地址。
//释放后的内存块会和前后空内存块合并。
void RTOS_RAM_Free(void* p){
	RTOS_RAM_ListNode* index = (RTOS_RAM_ListNode*)((rtos_uint32_t)p - sizeof(RTOS_RAM_ListNode));
	index->used = 0;
	RTOS_RAM_LIST.used -= 1;
	if ((index->next != RTOS_NULL) && (index->next->used == 0) && (index->pre != RTOS_NULL) && (index->pre->used == 0)) {
		index->pre->size += sizeof(RTOS_RAM_ListNode) * 2 + index->size + index->next->size;
		index->pre->next = index->next->next;
		RTOS_RAM_LIST.size -= 2;
	}
	else if ((index->next != RTOS_NULL) && (index->next->used == 0)) {
		if (index->next->next != RTOS_NULL) {
			index->next->next->pre = index;
		}
		index->size += sizeof(RTOS_RAM_ListNode) + index->next->size;
		index->next = index->next->next;
		RTOS_RAM_LIST.size -= 1;
	}
	else if ((index->pre != RTOS_NULL) && (index->pre->used == 0)) {
		if (index->next != RTOS_NULL) {
			index->next->pre = index->pre;
		}
		index->pre->next = index->next;
		index->pre->size += sizeof(RTOS_RAM_ListNode) + index->size;
		RTOS_RAM_LIST.size -= 1;
	}
	else {
		
	}
}
