#ifndef MAIN_FC_H
#define MAIN_FC_H

#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
struct argsSendEvent_t
{

    uint8_t sender[2];
    uint8_t reciver[2];
    uint8_t flag;
    uint8_t numberFrame[2];
};
typedef struct  argsSendEvent_t argsSendEvent;

void sendEvent(uint8_t *buff,uint8_t* data, argsSendEvent args);
void createEventFrame(uint8_t *buff,uint8_t *data,argsSendEvent args);
void createGotowosc(uint8_t *data, argsSendEvent args,uint8_t type);

struct buffer_frames
{
int recent;
int start;
int end;
int active;
int size;
int *element;
};
typedef struct buffer_frames buffer_t;
int* MakeArray(int arraySizeX);
int isValidIpAdress(char *ipAddress);
int OpenPortRS(char* dev, speed_t spd, int parity, int stop);
int check_XOR(int len, uint8_t *_buf);
uint8_t CountBCC (uint8_t *_buf,int len);
uint8_t* prepare_data_to_send(unsigned char *buff,uint8_t type,uint8_t trid,char* sender, char* reciver, uint8_t flag,unsigned char data[],unsigned char numer[]);
int sec_diff(int sec,int min, int hour);
int day_diff(int day,int mon,int year);
 void decimal_hex(int n,uint8_t hex[]);
uint16_t crc16(uint8_t *ptr,int size);
void init(buffer_t* buff,int size);
void push(buffer_t* buff,int data);

int first_empty(uint8_t* bufor[]);
int first_not_empty(uint8_t* bufor[]);
int pop(buffer_t* buff);
int process_frame(unsigned char *buffor, uint8_t *frame,int frames_counter,int len,int typeOfData);
void print_usage(char * program_name);
void strrev( char str[]);
 int num_hex_digits(unsigned n);



#endif







