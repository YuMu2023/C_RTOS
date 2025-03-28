#include "RTOS_CortexM3.h"

void Cortex_M3_SysTick_Init(){
	*((unsigned int*)CORTEX_M3_SYSTICK_RELOAD) = (unsigned int)9000;
	*((unsigned int*)CORTEX_M3_SYSTICK_CURRENT) = (unsigned int)0;
	*((unsigned int*)CORTEX_M3_SYSTICK_CTRL) &= (unsigned int)0xFFFFFFFA;
	*((unsigned int*)CORTEX_M3_SYSTICK_CTRL) |= (unsigned int)0x00000002;
	*((unsigned char*)CORTEX_M3_SYSTICK_PRI) = (unsigned char)0xFF;
}

void Cortex_M3_SysTick_Enable(){
	*((unsigned int*)CORTEX_M3_SYSTICK_CTRL) |= (unsigned int)0x00000001;
}

void Cortex_M3_SysTick_Disable(){
	*((unsigned int*)CORTEX_M3_SYSTICK_CTRL) &= (unsigned int)0xFFFFFFFE;
}

