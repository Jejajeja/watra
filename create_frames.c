#include "create_frames.h"
#include <stdint.h>
#include "main_fc.h"

/* Convert from dec to bcd */
uint8_t decimal_to_bcd(int dec)
{	return (((dec/10) << 4) + (dec%10));
}

/* Convert bcd to dec */
unsigned int bcd_to_dec(uint8_t bcd)
{
	return((bcd & 0xF) + ((bcd & 0xF0) >> 4) * 10);
}
/* Create frame with event to send to Integrator */
void createEventFrame(uint8_t *buff,uint8_t *data,argsSendEvent args)
{
    int dzien=bcd_to_dec(buff[2]);
    int miesiac=bcd_to_dec(buff[1]);
    int rok=bcd_to_dec(buff[0]);
    int s = bcd_to_dec(buff[5]);
    int min = bcd_to_dec(buff[4]);
    int hour =bcd_to_dec(buff[3]);
    int d=day_diff(dzien,miesiac,rok);
    uint16_t day=d;
    uint32_t sec=sec_diff(s,min,hour);

    int kod_zdarzenia=(buff[17]-48)*100+(buff[18]-48)*10+buff[19]-48;
    int kod_obiektu=(buff[12]-48)*1000+(buff[13]-48)*100+(buff[14]-48)*10+buff[15]-48;

     /* SEQQ */
    data[HDR_SEQQ_HI]=0xF;
    data[HDR_SEQQ_LO]=0xF;
    /* TYPE */
    data[HDR_TYPE]=0x01;
    /* SIZE */
    data[HDR_SIZE_HI]=0x00;
    data[HDR_SIZE_LO]=0x20;
    /* PATHID */
    data[HDR_PATHID]=0x01;
    /* SENDER */
    data[HDR_SENDER_HI]=args.sender[0];
	data[HDR_SENDER_LO]=args.sender[1];
	/* RECIVER */
	data[HDR_RECIVER_HI]=args.reciver[0];
	data[HDR_RECIVER_LO]=args.reciver[1];
	/* NUM */
	data[HDR_NUM_HI]=args.numberFrame[0];
	data[HDR_NUM_LO]=args.numberFrame[1];
	/* RESERVED */
	data[HDR_RES_MSB_HI]=data[HDR_RES_MSB_LO]=data[HDR_RES_LSB_HI]=data[HDR_RES_LSB_LO]=0x00;
	/* DATE */
    data[EVENT_DATE_HI]=day >>8;
    data[EVENT_DATE_LO]=day;
    /* TIME */
    data[EVENT_TIME_MSB_HI]=sec >>24;
    data[EVENT_TIME_MSB_LO]=sec >> 16;
    data[EVENT_TIME_LSB_HI]=sec >> 8;
	data[EVENT_TIME_LSB_LO]=s;
	/* EVENT CODE */
    data[EVENT_CODE_HI]=kod_zdarzenia >> 8; data[EVENT_CODE_LO]=kod_zdarzenia& 0xff;
    /* OBJECT CODE */
    data[EVENT_OBJ_HI]=kod_obiektu >> 8; data[EVENT_OBJ_LO]=kod_obiektu& 0xff;
    /* FLAG */
    data[EVENT_FLAG]=args.flag;
    /* STATUS */
    if((buff[19]=='2') && (buff[18]=='3'))
        data[EVENT_STATUS]=0x01;
    else
        data[EVENT_STATUS]=0x03;
    /* RESERVED */
		data[EVENT_RES_HI]=data[EVENT_RES_LO]=0;
    /* CRC */
	uint16_t crc_val=~crc16(data,30);
	data[EVENT_CRC_HI]=(uint8_t)(crc_val >>8);
	data[EVENT_CRC_LO]=(uint8_t)(crc_val & 0xFF);

}
/* Create Frame with readiness */
void createReady_status(uint8_t *data, argsSendEvent args,uint8_t type)
{

    /* SEQQ */
    data[HDR_SEQQ_HI]=0xF;
    data[HDR_SEQQ_LO]=0xF;
    /* TYPE */
    data[HDR_TYPE]=0x03;
    /* SIZE */
    data[HDR_SIZE_HI]=0x00;
    data[HDR_SIZE_LO]=0x16;
    /* PATHID */
    data[HDR_PATHID]=0x01;
    /* SENDER */
    data[HDR_SENDER_HI]=args.sender[0];
	data[HDR_SENDER_LO]=args.sender[1];
	/* RECIVER */
	data[HDR_RECIVER_HI]=args.reciver[0];
	data[HDR_RECIVER_LO]=args.reciver[1];
	/* NUM */
	data[HDR_NUM_HI]=args.numberFrame[0];
	data[HDR_NUM_LO]=args.numberFrame[1];
	/* RESERVED */
	data[HDR_RES_MSB_HI]=data[HDR_RES_MSB_LO]=data[HDR_RES_LSB_HI]=data[HDR_RES_LSB_LO]=0x00;
	data[READY_TYPE_NUM]=type;   //bo tylko 1 bit ==0x01      a tylko 2 bit to 0x02 :)
	data[READY_TYPE_VAL]=type;
	data[READY_RES_HI]=data[READY_RES_LO]=0;
	uint16_t crc_val=~crc16(data,20);
	data[READY_CRC_HI]=(uint8_t)(crc_val >>8);
	data[READY_CRC_LO]=(uint8_t)(crc_val & 0xFF);
}
/* Create frame to device with Time Synch */

void createSynchTime(uint8_t *buff,uint8_t *data)
{
        uint16_t day=buff[16] << 8;
                 day |=buff[17];

        uint32_t time  =buff[18] << 24;
                 time |=buff[19] << 16;
                 time |=buff[20] << 8;
                 time |=buff[21];
    int y=2000,m=1,d=1;
    struct tm t = { .tm_year=y-1900, .tm_mon=m, .tm_mday=d,  .tm_sec=0, .tm_min=0, .tm_hour=0 };
    t.tm_mday+=((int)day);

    t.tm_sec+=((int)time/1000);
    mktime(&t);
    data[SYN_SEQQ]=0xFF;
    data[SYN_DATE_Y]=decimal_to_bcd(t.tm_year-100);
    data[SYN_DATE_M]=decimal_to_bcd(t.tm_mon);
    data[SYN_DATE_D]=decimal_to_bcd(t.tm_mday);

    data[SYN_TIME_H]=decimal_to_bcd(t.tm_hour);
    data[SYN_TIME_M]=decimal_to_bcd(t.tm_min);
    data[SYN_TIME_S]=decimal_to_bcd(t.tm_sec);


    data[SYN_TIME_XOR]=CountBCC(data,7);

}
/* Create ACK frame to Integrator */
void createconf(uint8_t* data,argsSendEvent args,uint8_t ACK_type,uint16_t num)
{

    /* SEQQ */
    data[HDR_SEQQ_HI]=0x0F;
    data[HDR_SEQQ_LO]=0x0F;
    /* TYPE */
    data[HDR_TYPE]=0x02;
    /* SIZE */
    data[HDR_SIZE_HI]=0x00;
    data[HDR_SIZE_LO]=0x15;
    /* PATHID */
    data[HDR_PATHID]=0x01;
    /* SENDER */
    data[HDR_SENDER_HI]=args.sender[0];
	data[HDR_SENDER_LO]=args.sender[1];
	/* RECIVER */
	data[HDR_RECIVER_HI]=args.reciver[0];
	data[HDR_RECIVER_LO]=args.reciver[1];
	/* NUM */
	data[HDR_NUM_HI]=args.numberFrame[0];
	data[HDR_NUM_LO]=args.numberFrame[1];
	/* RESERVED */
	data[HDR_RES_MSB_HI]=data[HDR_RES_MSB_LO]=data[HDR_RES_LSB_HI]=data[HDR_RES_LSB_LO]=0x00;
	/*ACK TYPE */
	data[ACK_TYPE]=ACK_type;   //bo tylko 1 bit ==0x01      a tylko 2 bit to 0x02 :)
	data[ACK_NUM_HI]=num>> 8;
	data[ACK_NUM_LOW]=num;
	uint16_t crc_val=~crc16(data,19);
	data[ACK_CRC_HI]=(uint8_t)(crc_val >>8);
	data[ACK_CRC_LO]=(uint8_t)(crc_val & 0xFF);
}
