#ifndef __CREATE_FRAMES_H
#define __CREATE_FRAMES_H
#include <stdint.h>
#include "main_fc.h"


void createEventFrame(uint8_t *buff,uint8_t *data,argsSendEvent args);
void createGotowosc(uint8_t *data, argsSendEvent args,uint8_t type);
void createSynchTime(uint8_t *buff,uint8_t *data);
void createconf(uint8_t* data,argsSendEvent args,uint8_t ACK_type,uint16_t num);

enum header{
        HDR_SEQQ_HI=0,
        HDR_SEQQ_LO,
        HDR_TYPE,
        HDR_SIZE_HI,
        HDR_SIZE_LO,
        HDR_PATHID,
        HDR_SENDER_HI,
        HDR_SENDER_LO,
        HDR_RECIVER_HI,
        HDR_RECIVER_LO,
        HDR_NUM_HI,
        HDR_NUM_LO,
        HDR_RES_MSB_HI,
        HDR_RES_MSB_LO,
        HDR_RES_LSB_HI,
        HDR_RES_LSB_LO,
};
enum event{
        EVENT_DATE_HI=16,
        EVENT_DATE_LO,
        EVENT_TIME_MSB_HI,
        EVENT_TIME_MSB_LO,
        EVENT_TIME_LSB_HI,
        EVENT_TIME_LSB_LO,
        EVENT_CODE_HI,
        EVENT_CODE_LO,
        EVENT_OBJ_HI,
        EVENT_OBJ_LO,
        EVENT_FLAG,
        EVENT_STATUS,
        EVENT_RES_HI,
        EVENT_RES_LO,
        EVENT_CRC_HI,
        EVENT_CRC_LO,
};

enum ready_status{

        READY_TYPE_NUM=16,
        READY_TYPE_VAL,
        READY_RES_HI,
        READY_RES_LO,
        READY_CRC_HI,
        READY_CRC_LO,
};

enum synch_frame{
        SYN_SEQQ=0,
        SYN_DATE_Y,
        SYN_DATE_M,
        SYN_DATE_D,
        SYN_TIME_H,
        SYN_TIME_M,
        SYN_TIME_S,
        SYN_TIME_XOR,
};

enum ack{
        ACK_TYPE=16,
        ACK_NUM_HI,
        ACK_NUM_LOW,
        ACK_CRC_HI,
        ACK_CRC_LO,
};
#endif
