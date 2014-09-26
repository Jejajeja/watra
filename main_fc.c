#include "main_fc.h"
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <arpa/inet.h>

typedef struct buffer_frames buffer_t;




/* Make array with defined size */
int* MakeArray(int arraySizeX) {
int* theArray;
theArray = (int *) malloc(arraySizeX*sizeof(int));
   return theArray;
}

/* Check if IP adress is valid */
int isValidIpAdress(char *ipAddress)
{
    struct sockaddr_in sa;
    int  result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result;
}

/* Open serial port */
int OpenPortRS(char* dev, speed_t spd, int parity, int stop)
{

    struct termios options;
    int fd=open(dev,O_RDWR|O_NOCTTY|O_NDELAY);
    if(fd<0)
    {
        return -1;
    }
    else
    {
        tcgetattr(fd, &options);
	cfsetispeed(&options, spd);
	cfsetospeed(&options, spd);
	options.c_cflag&= ~CSIZE;
	options.c_cflag |= CS8;
	if(parity==1)
		options.c_cflag |= PARENB;
	else
		options.c_cflag&= ~PARENB;

		options.c_cflag&= ~CSTOPB;
	options.c_lflag&= ~(ICANON |ECHO | ECHOE | ISIG);
	options.c_iflag&= ~(ICRNL | INLCR | IGNCR | IUCLC);
	options.c_iflag&= ~(IXON | IXOFF| IXANY);
	options.c_oflag&= ~OPOST;
	tcsetattr(fd, TCSANOW, &options);

   }
return fd;
}

/*Check XOR sum if valid ret 0 else 1 */
int check_XOR(int len, uint8_t *_buf)
{
    int sum;
    sum=CountBCC(_buf,len-1);
    if(_buf[len-1]==sum)
        return 0;
    else
        return 1;
}

/*Count XOR sum*/
uint8_t CountBCC (uint8_t *_buf,int len)
{
    uint8_t bcc=0x00;
    int x;
    for(x=0; x<len; x++)
        bcc ^=_buf[x];
    return bcc;
}


/*Count how many sec elapsed since 0:00:00 */
int sec_diff(int sec,int min, int hour)
{
   struct tm date;
    struct tm beg;
	bzero(&date,sizeof(struct tm));
	bzero(&beg,sizeof(struct tm));
	time_t t1,t2;
    date.tm_hour=hour;date.tm_min=min;date.tm_sec=sec;
date.tm_year=114;
	beg.tm_year=114;
	t2=mktime(&date);
	t1=mktime(&beg);
    int c; c=(t2-t1)*1000;
    return c;


}

/* Count how many days elapsed since 1:1:2000 */
int day_diff(int day,int mon,int year)
{
    int days;
    struct tm beg;

    struct tm date;
	bzero(&date,sizeof(struct tm));
	bzero(&beg,sizeof(struct tm));
    beg.tm_hour=0;beg.tm_min=0;beg.tm_sec=0;beg.tm_mon=0;beg.tm_mday=0;beg.tm_year=100;
    beg.tm_isdst=1;
    date.tm_isdst=1;
    date.tm_hour=0;date.tm_min=0;date.tm_sec=0;date.tm_mday=day-1;date.tm_mon=mon-1;date.tm_year=100+year;
	days=(int)difftime(mktime(&date),mktime(&beg))/86400;

    return days;
}

 /*Convert from dec to hex */
  void decimal_hex(int n,uint8_t hex[])
{
	int i=0,rem;
	while(n!=0)
	{
		rem=n%16;
		switch(rem)
		{
			case 10:
				hex[i]=0x0A;
				break;
			case 11:
				hex[i]=0x0B;
				break;
			case 12:
				hex[i]=0x0C;
				break;
			case 13:
				hex[i]=0x0D;
				break;
			case 14:
				hex[i]=0x0E;
				break;
			case 15:
				hex[i]=0x0F;
				break;
			default:
				hex[i]=rem;
				break;
		}
		++i;
		n/=16;

	}
	hex[i]='\0';
	strrev(hex);
}

/*Count crc16 sum */
uint16_t crc16(uint8_t *ptr,int size)
{
	uint16_t crc=0xFFFF;
	uint16_t i;

	while(size--)
	{
		crc=crc ^ (uint16_t)*ptr++;
		for (i=0;i<8;i++)
		{
			if(crc & 0x0001)
				crc=(crc >>1) ^ 0x8005;
			else
				crc >>=1;
		}
	}
	return (crc);
}

/* Print usage */
void print_usage(char * program_name)
{
    printf("Usage: %s options \n",program_name);
    printf(" -h --help                    Wyswietla ta wiadomosc\n"
           " -p --parzystosc              Wlacza kontrole parzystoci\n"
           " -s --stop                    Wlacz obsluge bitu stopu\n"
           " -b --baudrate liczba         Okresla baudrate                *\n"
           " -i --ip      [Adres IP]      Okresla  adres ip integratora   *\n"
           " -q --port    [numer portu]   Okresla numer portu integratora *\n"
           " -w --sender  [kod nadawcy]   Okresla kod nadawcy komunikatu  *\n"
           " -o --reciver [kod odbiorcy]  Okresla kod odbiorcy komunikatu *\n"
	   " -d --dev     [adresportu]    Okresla adres portu             *\n"
	   "  * - opcje wymagane\n"
);

}

/* Reverse string */
void strrev(char str[])
{
	int i,j;
	char temp[33];
	for(i=strlen(str)-1,j=0; i+1!=0;--i,++j)
		temp[j]=str[i];
	temp[j]='\0';
	strcpy(str,temp);
}

/*How many dig will have hex string after converting from dec */
 int num_hex_digits(unsigned n)
{
	int ret =0;
	if(!n)
		return 1;
	for(;n;n>>=4)
		++ret;
	return ret;
}
