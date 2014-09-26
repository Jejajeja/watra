#include "main_fc.h"
#include "create_frames.h"
#include "cyclic_buffer.h"
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <getopt.h>
#include <termios.h>

#include <sys/ioctl.h>

/*Louis De Funes */
char *IP="192.168.1.2";  //IP
uint16_t reciver=6969;   //Reciver code
uint16_t sender=1212;    //Sender code
int portno=26000;        //Port nb. (Integrator) on TCP/IP
int baudrate=9600;       //Baudrate
char *dev="/dev/ttyAMA0";//device adress
int parzystosc,bit_stopu=0;


int a,f,n=0;		//varibles for loops
int frames_send_counter;//counter for sended frames to Integrator
int retval;	        //variable for select()
int sockfd=0;           //File desciptor for socket
int play,playrecv;	//variables for mutex
int bufor_count[1000];  //Bufor how many times each frame was sended
uint8_t dataIntegrator[1000]; //Array for recived frame
uint8_t* bufor[1000];   //Frames sended bufor
time_t framesDelayTimeArray[1000]; //How many sec elapsed since send each frame
char* program_name;
argsSendEvent args;
buffer_t buff;         //cyclic buffer
speed_t speed;	       //Baudrate
extern int h_errno;    //Socket connection error
time_t begin,end;     //
time_t LastAckFrameFromDev=0;
time_t LastAckFrameFromInt=0;
struct sockaddr_in serv_addr;
struct hostent *server;

fd_set fdSet;


        struct argsReadingData {
char* dev;
speed_t spd;
int parity;
int stop;
int sockfd;
};



void connectSocket(int start)
{
if(start!=1) //jezeli to nie jest pierwsze polacznie zamknij stare
close(sockfd);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
/*Obsluga roznych bledow */
    if (sockfd <= 0)
       {

           write(2,"ERROR opening socket",20);
           sleep(1);
           connectSocket(0);

       }
    server = gethostbyname(IP);
    if ( server == NULL)
    {
        write(2,"ERROR, no such host",19);
        sleep(1);
        connectSocket(0);

    }
    else if (( h_errno ==HOST_NOT_FOUND) || ( h_errno==NO_RECOVERY))
    {
      write(2,"HOST NOT FOUND",14);
      sleep(1);
      connectSocket(0);

    }
    else if(( h_errno==NO_ADDRESS) || ( h_errno==NO_DATA))
    {
         write(2,"WRONG IP ADRESS",15);
         sleep(1);
         connectSocket(0);

    }
    else if( h_errno==TRY_AGAIN)
    {
         write(2,"ERROR opening socket",20);
         sleep(1);
         connectSocket(0);


    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
       {
          sleep(1);
          syslog(LOG_ERR,"Error while connection to socket. Reconnecting");
          connectSocket(0);
      }

}
}
void increaseConterSendedFrames()
{
    if(frames_send_counter<65536)
    frames_send_counter++;
    else
    frames_send_counter=0;
}





void resend (int number)
{
        if(bufor_count[number]<3)
        {
                write(sockfd,bufor[number],32);
                if(bufor_count[number]==3)
                {
                bufor[number]=0;
                framesDelayTimeArray[number]=0;
                bufor_count[number]=0;
                }
                else
                bufor_count[number]++;
        }
 }
pthread_mutex_t lockrecv = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condrecv = PTHREAD_COND_INITIALIZER;

void lock_recive()
{
    pthread_mutex_lock(&lockrecv);      //Lock pop thread
    playrecv= 0;
    pthread_mutex_unlock(&lockrecv);
}

void unlock_recive()
{
    pthread_mutex_lock(&lockrecv);      //unlock pop thread
    playrecv = 1;
    pthread_cond_signal(&condrecv);
    pthread_mutex_unlock(&lockrecv);
}

//FUNKCJA ODBIERANIA I TLUMACZENIA DANYCH
void *reciveFrame(void *args)
{
    struct argsReadingData *readParams = args;

    int sockfd=readParams.sockfd;
      //definicje zmiennych
struct timeval tv;
bzero(&tv,sizeof(struct timeval));
uint16_t crc;
uint8_t crc_lsb,crc_msb,num_frame[2];
int nfds,retval,fd,n,i,licznik;
uint8_t data[2];
int fdRS=OpenPortRS(readParams.dev,readParams.spd,readParams.parity,readParams.stop);
uint8_t filteredDataUrzadzenie[1000];

uint8_t frame[13];
uint8_t *frameFromIntegrator;
uint8_t  dataRecivedRS[1024];


for(;;)
{

    pthread_mutex_lock(&lockrecv);
    while(!playrecv)
    { /* We're paused */
        pthread_cond_wait(&condrecv, &lockrecv); /* Wait for play signal */
   }
    pthread_mutex_unlock(&lockrecv);

    FD_ZERO(&fdSet);					  //zerowanie zbioru
    FD_SET(fdRS, &fdSet);				  //dodanie socketu do zbioru deskryptorów
    FD_SET(sockfd,&fdSet);

if(sockfd>fdRS)
	nfds=sockfd+1;
else
    nfds=fdRS+1;

    tv.tv_sec = 10;
    tv.tv_usec = 0;

    retval=select(nfds,&fdSet,NULL,NULL,&tv );      //ustaw TIMEOUT NA 11 sekund jezeli nie otrzymano wpierdol Błąd
    if(retval==-1)
    {
		  close(fdRS);
                        syslog(LOG_ERR,"ERROR WHILE READING FROM PORT");

                        OpenPortRS(dev,speed,parity,stop);
                        sleep(2);
	}
else if(retval)
    {

        for (fd = 0; fd <= nfds-1; fd++)
        {
            if (FD_ISSET(fd, &fdSet))
            {
                if (fd == fdRS)                                 //Jezeli dane sa na RS'ie
                {
                    usleep(15000);                              //Poczekaj żeby odebrać wszystkie dane
                 	n=read(fdRS, &dataRecivedRS, 1024);         //Odczytaj max 1024 znaki z RS
                    if(n<1)                                     //jeżeli błąd odczytu
                    {
                        close(fdRS);                            //zamknij połączenie
                    	syslog(LOG_ERR,"ERROR WHILE READING FROM PORT");
                        OpenPortRS(dev,speed,parity,stop);      // Otwórz połączenie jeszcze raz
                        sleep(2);                               //Poczekaj 2s
                    }
                    else                                        //Jeżeli odczytano jakieś dane
                    {
                        for (i=0;i<n-1;i++)
                        {

                            if((dataRecivedRS[i]==0x32) && (dataRecivedRS[i+1]==0x3f))  //Jeżeli odebrano KeepAlive
                            {
                                if(LastAckFrameFromInt!=0)                              //Jeżeli już odebrano jakąś ramkę Keep Alive od Integratora
                                {
                                    time_t end;
                                    time(&end);
                                    if(end-LastAckFrameFromInt<20)                      //Jeżeli czas od ostatniego Keep Alive od Integratora jest mniejszy od 20s
                                    {
                                        data[0]=0x4F;
                                        data[1]=0x4B;
                                        write(fdRS,&data,2);                            //Odpowiedz OK
                                    }
                                }
                                else                                                    //Jeżeli jeszcze nie odbrano pierwszej ramki KA od Integratora odeślij OK
                                {
                                    data[0]=0x4F;
                                	data[1]=0x4B;
                                    write(fdRS,&data,2);
                                }
                                i++;                                                    //Przejdź do następnych odebranych bajtów informacji
                                time(&LastAckFrameFromDev);                             //Zapisz czas ostatniego KA od urządzenia
                            }
                            else if ((dataRecivedRS[i]==0xFF) && (dataRecivedRS[i+1]==0x01)) //Jeśli odebrano ACK od urządzenia przedź do następnego bajtu (po ACK)
                            {
                                i++;
                            }
                            else if((dataRecivedRS[i]==0xFF) && (dataRecivedRS[i+1]==0x00)) //Jeśli odebrano NACK od urządzenia przedź do następnego bajtu (po NACK)
                            {
                                i++;
                            }
                            else if((dataRecivedRS[i]==0x4F) && (dataRecivedRS[i+1]==0x4B)) //Jeśli odebrano OK od urządzenia przedź do następnego bajtu (po OK)
                            {
                                i++;
                            }
                            else                                                            //Jest to ramka ze zdarzeniem, załaduj bajt do pamięci
                            {
                                filteredDataUrzadzenie[a]=dataRecivedRS[i];
                                if(i<n)
                                    a++;
                            }

                        }

                        if(a>=13)         //Jeśli w pamięci jest minimum 13 bajtów
                        {
                            a--;          //zmniejszam a o 1 aby uniknąć wyjścia poza tablicę w pętli poniżej
                            for(n=0; n<a-11;n++)
                            {
                                memcpy(frame, filteredDataUrzadzenie+n, 13*sizeof(uint8_t)); //skopiuj 13 bitów do pamięci
                                if(!check_XOR(13,frame))                                     //Sprawdź XOR jeśli się zgadza
                                {
                                    uint8_t* frameToIntegrator=malloc(32*sizeof(uint8_t));   //Zajęcie zmiejsca na ramkę do Integratora
                                    decimal_hex(frames_send_counter,num_frame);              //Oblicz numer ramki

                                    args.numberFrame[0]=frames_send_counter <<8;             /*Tworzenie nagłówka */
                                    args.numberFrame[1]=frames_send_counter;
                                    args.flag=0x01;
                                    args.sender[0]=sender >> 8;
                                    args.sender[1]=sender;
                                    args.reciver[0]=reciver >> 8;
                                    args.reciver[1]=reciver;
                                    createEventFrame(frame,frameToIntegrator, args);         //stworzenie całej ramki do Integratora
                                    licznik=first_empty(bufor);                              //Pierwsze wolne pole w tablicy z ramkami
                                    time(&framesDelayTimeArray[licznik]);                    //Wpisz czas wysłania ramki do tablicy
                                    bufor[licznik]=frameToIntegrator;                        //wpisz do bufora ramkę która zostawie wysłana do Integartora
                                    bufor_count[licznik]=1;                                  //Wpisz, że jest to pierwszy wysłanie ramki
                                    push(&buff,n);                                           //Wrzuć ID ramki w buforze (numer pola w którym znajduje się ramka ) do bufora cyklicznego
                                    write(sockfd,frameToIntegrator,32);                      //Wyślij do Integratora ramkę
                                    free(frameToIntegrator);                                 //Zwolnij miejsce zaakolowane na ramkę
                                    increaseConterSendedFrames();                            //zwiększ licznik ramek wysłanych
				            	}
                                else                                                         //Jeśli XOR ramki się nie zgadza
                                {
                                   syslog(LOG_ERR,"Warning wrong XOR of frame recived from device!");
                                }
                                a=0;
                            }


                        }
                    }

                }
                else if(fd==sockfd)                                                      //Jeśli otrzymano dane od Integratora
                {
                    bzero(dataIntegrator,1000);                                          //Wyzeruj bufor do którego zostanie załadowana ramka
                    n = read(sockfd,dataIntegrator,1000);                                //Odczytaj ramkę

                    if(n>0)                                                              //Jeśli odczytano jakieś dane
                    {
                        for (i=0;i<n-1;i++)                                              //Odczytuj po kolei bajty
                        {
                            if((dataIntegrator[i]==0x0F) && (dataIntegrator[i+1]==0x0F)) //Jeśli znaleziono początek ramki
                            {
                                int size=(int)dataIntegrator[i+4];                       //Odczytanie rozmiaru tej ramki
                                frameFromIntegrator=malloc(size*sizeof(uint8_t));        //Akolacja miejsca na ramkę
                                memcpy(frameFromIntegrator,dataIntegrator+i,size);       //Skopiowanie ramki

                                switch(dataIntegrator[2])                                //Rozpoznawanie ramki po jej typie
                                {
                                    case(0x03):	//Gotowość
                                    crc=~crc16(frameFromIntegrator,20);
                                    crc_lsb=(uint8_t)(crc & 0xFF);
                                    crc_msb =(uint8_t) (crc >> 8);
                                    if((frameFromIntegrator[20]==crc_msb) && (frameFromIntegrator[21]==crc_lsb)) //Jeśli zgadza się suma kontrolna
                                    {
                                        if(frameFromIntegrator[16]==0x02)               //Jeśli jest błąd w połączeniu zresetuj połączenie
                                            {
                                                close(sockfd);
                                                sleep(1);
                                                connectSocket(0);
                                            }
                                        time(&LastAckFrameFromInt);
                                        decimal_hex(frames_send_counter,num_frame);
                                        args.numberFrame[0]=num_frame[0];
                                        args.numberFrame[1]=num_frame[1];

                                        uint8_t* frameToIntegrator=malloc(22*sizeof(uint8_t));
                                        time(&begin);
                                        if(begin-LastAckFrameFromDev<=10)    //jeżeli ostatnia ramka z gotowością od urządzneia nie była dawniej niż 10s temu
                                             createReady_status(frameToIntegrator,args,0x01);             //Pełna gotowość
                                        else
                                             createReady_status(frameToIntegrator,args,0x02);             //W przeciwnym wypadku błąd

                                        write(sockfd,frameToIntegrator,22);                           //wysłanie ramki do Integratora z gotowością
                                        increaseConterSendedFrames();                                 //zwiększenie licznika ramek wysłanych
                                        free(frameToIntegrator);
                                    }
                                    break;

                                case(0x02):       //ACK/NACK

                                    crc=~crc16(frameFromIntegrator,19);
                                    crc_lsb=(uint8_t)(crc & 0xFF);
                                    crc_msb =(uint8_t) (crc >> 8);
                                    if((frameFromIntegrator[19]==crc_msb) && (frameFromIntegrator[20]==crc_lsb)) //Jeśli suma kotrolna sie zgadza
                                    {
                                        uint8_t frameFromBufor[32];
                                        uint8_t ACKtype=frameFromIntegrator[16];
                                        uint8_t num[2];
                                        int not_found=0;
                                        num[0]=frameFromIntegrator[17];                                          //Pobranie numeru ramki
                                        num[1]=frameFromIntegrator[18];
                                        for(n=0;n<1000;n++)                                                      //Przeszukanie
                                        {
                                            if((bufor[n]!=0) && (bufor[n]!=NULL))
                                            {
                                                memcpy(frameFromBufor,bufor[n],13);
                                                if((frameFromBufor[10]==num[0]) && (frameFromBufor[11]==num[1]))
                                                    break;
                                            }
                                            if(n==1000)                                                          //Jeśli nie znaleziono ramki w buforze
                                                not_found=1
                                       }
                                       if(!not_found)                                                          //Jeżeli numer ramki znajdował sie w buforze
                                        {
                                            if((ACKtype==0x01) || (ACKtype==0x03))                                   //Jeżeli otrzymałem potwierdzenie
                                            {
                                                bufor[n]=NULL;                                                      //usuń ramkę
                                                framesDelayTimeArray[n]=0;
                                                bufor_count[n]=0;
                                            }
                                            else
                                                resend(n);                                                          //W przeciwnym wypadku wyślij ponownie
                                        }
                                    }
                                    break;


                            case(24):      //synchronizacja czasu

                                crc=~crc16(dataIntegrator,22);
                                crc_lsb=(uint8_t)(crc & 0xFF);
                                crc_msb =(uint8_t) (crc >> 8);
                                uint16_t numFrame=dataIntegrator[10] << 8;
                                numFrame|=dataIntegrator[11];

                                if((dataIntegrator[22]==crc_msb) && (dataIntegrator[23]==crc_lsb))              //Jeśli suma kotrnolna się zgadza
                                    {
                                    uint8_t polecenieSyn[8];
                                    createSynchTime(dataIntegrator,polecenieSyn);                               //Stwórz ramkę z synchronizacją czasu
                                    write(fdRS,polecenieSyn,8);                                                 //Wyślij ją do urządzenia
                                    uint8_t frameToIntegrator[21];
                                    createconf(frameToIntegrator,args,0x01,numFrame);                           //Stwóz ramkę z potwierdzeniem do Integratora
                                    write(sockfd,frameToIntegrator,21);                                         //Wyślij ramkę z potwiedzeniem do Integratora
                                    increaseConterSendedFrames();                                               //Zwiększ licnzik wysłanych ramek


                                    }
                                else                                                                            //Jeśli błędna suma kontrolna, wyślij NACK
                                {
                                    uint8_t frameToIntegrator[21];
                                    createconf(frameToIntegrator,args,0x02,numFrame);
                                    write(sockfd,frameToIntegrator,21);
                                    increaseConterSendedFrames();
                               	}
                                break;


                            }
                            free(frameFromIntegrator);

                        }

                    }

                }
                else                                                                                        //Błąd odczytu z Socketu, restart połączenia
                {
                    syslog(LOG_ERR,"ERROR WHILE READING FROM SOCKET");
                    connectSocket(0);
                }

            }
            bzero(dataIntegrator,1000);

        }
    }

    }
    else                                                    //timeout on select function
	syslog(LOG_ERR,"Nothing recived in 11 sec");
}

}







void *check_frames_send_ACK()
{
    time_t tm;
    int i;
    int timeElapsed;
    for(;;)
    {

           time(&tm);
        lock_recive();
        for (i=0;i<1000;i++)
            {
                if(framesDelayTimeArray[i]!=0)
                {
                    timeElapsed=tm-framesDelayTimeArray[i];
                    if(timeElapsed>=5)
                        {if(bufor_count[i]!=3)
                            {
                                resend(i);
                                bufor_count[i]++;
                            }
                        else
                            {
                            bufor[i]=NULL;
                            framesDelayTimeArray[i]=0;
                            bufor_count[i]=0;

                           } }
                }

            }
	unlock_recive();
        usleep(500000);
    }

}





int main(int argc, char *argv[])
{
    char *port;

    int next_option=0;
    program_name=argv[0];
    int options_selected=0;

    const char* const short_options="hpb:i:w:o:q:sd:";
/*Obsługa parametrów uruchomieniowych */
    const struct option long_options[] = {
    {"help",       0, NULL, 'h'},
    {"parzystosc", 0, NULL, 'p'},
    {"baudrate",   1, NULL, 'b'},
    {"IP",         1, NULL, 'i'},
    {"sender",     1, NULL, 'w'},
    {"reciver",    1, NULL, 'o'},
    {"port",       1, NULL, 'q'},
    {"stop",       0, NULL, 's'},
    {"dev",        1, NULL, 'd'},
    {NULL,         0, NULL,  0 }
    };

int index=0;
if(argc<2)
{
syslog(LOG_ERR,"No option defined using default!");


}
     while(next_option!=-1)
  	 {

   next_option=getopt_long(argc,argv,short_options,long_options, &index);
       switch(next_option)
       {

	 case 'h':
        print_usage(program_name);
        return 0;
       case 'p':
        parzystosc=1;
	options_selected++;
        break;
       case 's':
        bit_stopu=1;
        break;
       case 'b':
        baudrate=atoi(optarg);
	options_selected++;
        break;
       case 'd':
        dev=strdup(optarg);
	options_selected++;
        break;
       case 'i':
	IP=strdup(optarg);
	options_selected++;
        break;
       case 'q':
	port=strdup(optarg);
	options_selected++;
	break;
        case 'w':
        options_selected++;
        sender = (int)strtol(optarg,NULL,16);
        break;
        case 'o':
        options_selected++;
        reciver = (int)strtol(optarg,NULL,16);
        break;
       case '?':
       print_usage(program_name);
        syslog(LOG_ERR, "Wrong option specified! ");
        break;
       case -1:
        break;
       default:
       print_usage(program_name);
           syslog(LOG_ERR,"Unexpected error in options!");

       }

   }
if(options_selected!=6)
	{syslog(LOG_ERR,"NIE ZOSTLY OKRESLONE WSZYSTKIE WYMAGANE OPCJE!\n");
}

	switch(baudrate)
	{
	case 1 ... 75:
	speed=B75;
	break;
	case 76 ... 110:
	speed=B110;
	break;
	case 111 ... 134:
	speed=B134;
	break;
	case 135 ... 150:
	speed=B150;
	break;
	case 151 ... 200:
	speed=B200;
	break;
	case 201 ... 300:
	speed=B300;
	break;
	case 301 ... 600:
	speed=B600;
	break;
	case 601 ... 1200:
	speed=B1200;
	break;
	case 1201 ... 1800:
	speed=B1800;
	break;
	case 1801 ... 2400:
	speed=B2400;
	break;
	case 2401 ... 4800:
	speed=B4800;
	break;
	case 4801 ... 10000:
	speed=B9600;
	break;
	case 10001 ... 19200:
	speed=B19200;
	break;
	case 19201 ... 38400:
	speed=B38400;
	break;
	case 38401 ... 100600:
	speed=B57600;
	break;

	default:
	syslog(LOG_ERR,"Wrong baudrate defined using 9600");
	speed=B9600;
	return 0;
	}



	pthread_t thReciveFrame; //Definiowanie wątku odbierania przetwarzanie i wysyłanie

	pthread_t thThread_check_frames_send_ACK; //Definiowanie wątku odpowiedzialnego za sprawdzenie czy ramka została potwierdzona
	bzero(bufor,1000);                        //Zerowanie bufora cyklicznego (używany w razie przepełnienia bufora ramek
    portno=atoi(port);
	connectSocket(1);                         //Połączenie socket

	init(&buff,1000);                         //Inicjowanie bufora cykliczngo
    bzero(framesDelayTimeArray,1000);         //Zerowanie tablicy z czsami wysłania ramek

	struct argsReadingData args={dev,speed,parzystosc,bit_stopu,sockfd};


	pthread_create(&thReciveFrame,NULL,reciveFrame,&args);   //Tworzenie wątku odbierania i przetwarzania i wysyłania ramek
    pthread_create(&thThread_check_frames_send_ACK,NULL,check_frames_send_ACK,NULL); //uruchomienie wątku spradzania czy otrzymałem potwierdzenie na ramkę



for(;;)
	sleep(2000);

    pthread_cancel(thThread_check_frames_send_ACK);
	pthread_cancel(thReciveFrame);
	pthread_exit(NULL);


        free(buff.element);
return 0;
}
