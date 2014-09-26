#include "cyclic_buffer.h"
#include <stdint.h>



void init(buffer_t* buff,int size)
{
buff->recent=0;
buff->start=0;
buff->end=0;
buff->active=0;
buff->size=size;
buff->element=MakeArray(1000);
}


void push(buffer_t* buff,int data)
{

buff->element[buff->end]=data;
buff->recent=buff->end;
buff->end=(buff->end+1)%buff->size;

if(buff->active<buff->size)
buff->active++;
else
buff->start=(buff->start+1)%buff->size;
}

/* Function returns >0 (index) of first empty field in array else index of first recived frame to overwrite*/

int first_empty(uint8_t* bufor[])
{
	int i;
	for (i=0;i<1000;i++)
	if(bufor[i]==NULL)
		return i;
	return -1;
}

/* Function returns >0 (index in array) if array is not empty, else <0*/

int first_not_empty(uint8_t* bufor[])
{
	int i=0;
	for (i=0;i<1000;i++)
		if (bufor[i]!=0)
			return i;
		return -1;
}


int pop(buffer_t* buff)
{
int p;
if(!buff->active) return -1;
p=buff->element[buff->start];
buff->start=(buff->start+1)%buff->size;
buff->active--;
return p;

}
