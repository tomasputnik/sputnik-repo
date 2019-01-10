// ExternalControl.c : Defines the class behaviors for the application.
//
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/timeb.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>



#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define BYTE unsigned char


int SendingSocket=-1;
int ReceiveSocket=-1;

struct sockaddr_in ReceiverAddr;
struct sockaddr_in SenderAddr,SenderAddr1;

char SendBuf[1024];
char ReceiveBuf[65536];

void SetExternalControlSwitches(void);
void UdpIo(void);

int  kbhit  (void)  {
	  struct termios oldt, newt;
	  int ch;
	  int oldf;

	  tcgetattr(STDIN_FILENO, &oldt);
	  newt = oldt;
	  newt.c_lflag &= ~(ICANON | ECHO);
	  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	  ch = getchar();

	  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	  fcntl(STDIN_FILENO, F_SETFL, oldf);

	  if(ch != EOF)
	  {
	    ungetc(ch, stdin);
	    return 1;
	  }

	  return 0;

}  /* kbhit */

int main()
{
	printf(" \n\n\n");
	printf("************************************\n");
	printf("Imagenex ExternalControl Program\n");
	printf("Beta Test Version -- v1,0,0,1\n\n");
	printf("November 16, 2012 \n");
	printf("************************************\n\n\n");

	//////////////create a UDP listen socket

	memset((void *)&SenderAddr1, 0, sizeof(SenderAddr));
	memset(&ReceiverAddr.sin_zero,'\0',8);

	int Port = 4040;

   if ((ReceiveSocket = socket(AF_INET, SOCK_DGRAM, 0))//
       < 0)
   {
      printf("ERROR: socket failed !\n");
      fflush(stdout);
      return 0;
   }
   printf("ReceiveSocket :%d \n",ReceiveSocket);
   ReceiverAddr.sin_family = AF_INET;
   ReceiverAddr.sin_port = htons(((unsigned short) 4040));
   ReceiverAddr.sin_addr.s_addr =0;

    if (bind(ReceiveSocket,(struct sockaddr *)&ReceiverAddr,
        sizeof(struct sockaddr)) == -1)
   {
        perror("Bind");

        fflush(stdout);
        exit(1);
    }
	printf("ReceiveSocket :%d \n",ReceiveSocket);
	while(!kbhit())
	{
		UdpIo();


	}
    getchar();
	//close socket
	if(ReceiveSocket>0)
	{
	shutdown (ReceiveSocket,2);
    close(ReceiveSocket);
	}
	if(SendingSocket>0)
	{
	shutdown (SendingSocket,2);
    close(SendingSocket);
	}

	return 0;

}
void UdpIo(void)
{

	int Ret=-1;


	////////////////////////////////////////////
	int maxfd = ReceiveSocket;
	struct timeval tvl;
	tvl.tv_sec = 0;
	tvl.tv_usec = 500000;


	int flags=0;
	flags = fcntl(maxfd, F_GETFL, 0);
	fcntl(maxfd, F_SETFL, flags | O_NONBLOCK);
	fd_set rdset;


	do{
		FD_ZERO(&rdset);
		FD_SET(maxfd, &rdset);
		;
		Ret = select(maxfd+1, &rdset,NULL, NULL, &tvl);


	}while(Ret==-1);// || Ret==0);

	unsigned int AddrLen;
	AddrLen = sizeof(SenderAddr1);
	if (FD_ISSET(maxfd, &rdset));
	{
		Ret = 0;

		if(ReceiveSocket>0)

			Ret = recvfrom(ReceiveSocket, ReceiveBuf, 65535, 0,
		   		(struct sockaddr *)&SenderAddr1, &AddrLen);


	   	printf("udp recv bytes %d !\n",Ret);
	    fflush(stdout);
	}

	//send External controls
	SetExternalControlSwitches();

	Ret = -1;

	Ret = -1;

	Ret = sendto(ReceiveSocket, SendBuf, 256, 0,
			(struct sockaddr *)&SenderAddr1, sizeof(struct sockaddr));


	printf("udp send bytes %d !\n", Ret);
	fflush(stdout);



}
void SetExternalControlSwitches(void)
{
	register int i;
	int temp = 15000;

   for(i=0;i<256;i++) SendBuf[i] = 0;

   SendBuf[0] = 'E';
   SendBuf[1] = 'C';
   SendBuf[2] =	0;		//ID

	SendBuf[3] = 1;		//Control Byte 1
                        //Bit0: 0 = LocalControl, 1 = ExternalControl
                        //Bit0 must be set to ExternalControl for Switch
                        //settings to take effect

   SendBuf[4] = 0;		//Control Byte 2
   							//Bit0: 0 = Transmit & Receive, 1 = Receive Only (disable transmitter)

   SendBuf[5] = 0;		//Control Byte 3
   SendBuf[6] = 0;		//Control Byte 4

   SendBuf[7] = 7;		//Range
   							//must be in units of Meters
   							//0 = n/a
                        //1 = n/a
                        //2 = 5M
                        //3 = 10M
                        //4 = 20M
                        //5 = 30M
                        //6 = 40M
                        //7 = 50M
                        //8 = 60M
                        //9 = 80M
   							//10 = 100M

   SendBuf[8] = 6;		//Gain, 0 to 20dB
   SendBuf[9] = 30;		//Display Gain, 1 to 100 percent
   SendBuf[10] = 0;		//Gain Equalization, 0=Off, 1=On
   SendBuf[11] = 3;		//Sector Size, 0=30, 1=60, 2=90, 3=120deg
   SendBuf[12] = 2;		//Beamwidth, 0=Wide, 1=Normal, 2=Narrow, 3=Narrow Mixed
   SendBuf[13] = 0;		//Number of Beams, 0=480, 1=240, 2=120
   SendBuf[14] = 5;		//Averaging, 0,1=Off, 2,3,4,...10 = number of shots to average

   SendBuf[15] = (0&0xFF00)>>8;			//Persistence (Hi Byte), 0 to 600sec
   SendBuf[16] = (0&0x00FF);				//Persistence (Lo Byte)

   SendBuf[17] = (temp&0xFF00)>>8;;	//Sound Velocity*10 (Hi Byte), 1400.0 to 1600.0m/s
   SendBuf[18] = (temp&0x00FF);			//Sound Velocity*10 (Lo Byte)
   												//must be in units of Meters

   SendBuf[19] = 3;		//Mode, 0=Sector, 1=Linear, 2=Perspective, 3=Profile, 4=Beamtest

   SendBuf[20] = 0;		//83P/83B Output Enable, 0=83P, 1=83B

                        //For 83P Output:
                        //Enable Profile Point Detection (set SendBuf[21]=1)

                        //For 83B Output:
                        //Sector Size must be 120 Deg (set SendBuf[11]=3)
   							//Number of Beams must be 120 (set SendBuf[13]=2)

   SendBuf[21] = 1;		//Profile Point Detection, 0=Disable, 1=Enable
   SendBuf[22] = 0;		//Profile Minimum Range, 0 to 100M
   							//must be in units of Meters
   SendBuf[23] = 25;		//Profile Minimum Level, 10 to 90 percent
   SendBuf[24] = 0;		//Transducer Up/Down, 0=Down, 1=Up
   SendBuf[25] = 0+180;	//Profile Tilt Angle + 180, -30 to +30deg
   SendBuf[26] = 0;		//Roll Correction, 0=Off, 1=On
   SendBuf[27] = 0;		//Measurement Units, 0=Meters, 1=Feet, 2=Yards
   SendBuf[28] = 1;		//Record Start/Start (.837)

   SendBuf[29] = 0;		//Record Start/Start (.83P)
   //Not implemented

   SendBuf[30] = 0;		//Record Start/Start (.83B)
   //Not implemented

   //The following External Trigger Control Bytes are valid only for DeltaT
   //Sonar Heads supplied with the External Trigger Hardware Option
   SendBuf[31] = 0x00;					//External Trigger Control
   											//Bit0, Edge: 	 0=NEG,     1=POS
                        				//Bit1, Enable: 0=Disable, 1=Enable

   SendBuf[32] = (0&0xFF00)>>8;		//External Trigger Transmit Delay (Hi Byte)
   SendBuf[33] = (0&0x00FF);			//External Trigger Transmit Delay (Lo Byte)
   											//0 to 10000 in 100 microsecond increments
}
