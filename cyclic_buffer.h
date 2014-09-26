#ifndef __CYCLIC_BUFFER_H
#define __CYCLIC_BUFFER_H

#include <stdint.h>
#include "main_fc.h"

void init(buffer_t* buff,int size);
void push(buffer_t* buff,int data);
int first_empty(uint8_t* bufor[]);
int first_not_empty(uint8_t* bufor[]);
int pop(buffer_t* buff);




#endif

