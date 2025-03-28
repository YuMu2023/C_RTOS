#ifndef _RTOS_CORTEX_M3_H_
#define _RTOS_CORTEX_M3_H_

#define CORTEX_M3_SYSTICK_CTRL ((unsigned int)0xE000E010)
#define CORTEX_M3_SYSTICK_RELOAD ((unsigned int)0xE000E014)
#define CORTEX_M3_SYSTICK_CURRENT ((unsigned int)0xE000E018)
#define CORTEX_M3_SYSTICK_PRI ((unsigned int)0xE000ED23)

typedef struct cortex_m3_register_group{
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r12;
	unsigned int lr;
	unsigned int pc;
	unsigned int psr;
}CORTEX_M3, *pCORTEX_M3;

void Cortex_M3_SysTick_Init();
void Cortex_M3_SysTick_Enable();
void Cortex_M3_SysTick_Disable();

#endif//_RTOS_CORTEX_M3_H_
