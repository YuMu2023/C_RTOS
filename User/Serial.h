#ifndef _SERIAL_H
#define _SERIAL_H

void Serial_Init(void);
void Serial_SendByte(unsigned char b);
void Serial_SendString(const char*str);
void Serial_SendUnsigned(unsigned int num, int n);

#endif//_SERIAL_H
