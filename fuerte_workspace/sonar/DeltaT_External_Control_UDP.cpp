// Module Name: DeltaT_External_Control_UDP.cpp
//
// Date: 12JAN10
//
// Description:
//
//    This sample program illustrates how to develop a simple UDP application
//		that reads 83P, 83B or 83Z data messages from the Imagenex DeltaT.exe
//		Multibeam beamforming sonar program. An external control scheme allows
//		the user to control the DeltaT.exe program through the same UDP socket.
//		This sample is implemented as a console-style application and simply
//		prints status messages when data is received.
//
// Command Line:
//
//		DeltaT_External_Control_UDP.exe
//
//		12JAN10 - add Profile Point Filter (Byte 34)

#include <winsock2.h>
#include <stdio.h>
#include <conio.h>

void SendExternalControlSwitches(void);
void SetExternalControlSwitches(void);

WSADATA wsaData;
SOCKET ReceivingSocket;
SOCKADDR_IN ReceiverAddr;
int Port = 4040;
unsigned char ReceiveBuf[65536];
int BufLength = 65536;
SOCKADDR_IN SenderAddr;
int SenderAddrSize = sizeof(SenderAddr);
unsigned char SendBuf[256];

void main(void)
{
   register int i;
   int Ret;
   int ctr;

   //Initialize Winsock version 2.2
   if ((Ret = WSAStartup(MAKEWORD(2,2), &wsaData)) != 0)
   {
      //NOTE: Since Winsock failed to load we cannot use WSAGetLastError
      //to determine the error code as is normally done when a Winsock
      //API fails. We have to report the return status of the function
      printf("ERROR: WSAStartup failed with error %d\n", Ret);
      getch();
      return;
   }

   //Create a new socket to receive datagrams on
   if ((ReceivingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
       == INVALID_SOCKET)
   {
      printf("ERROR: socket failed with error %d\n", WSAGetLastError());
      WSACleanup();
      getch();
      return;
   }

   //Setup a SOCKADDR_IN structure that will tell bind that we want to
   //receive datagrams from all interfaces using port 4040
   ReceiverAddr.sin_family = AF_INET;
   ReceiverAddr.sin_port = htons(Port);
   ReceiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

   //Associate the address information with the socket using bind
   if (bind(ReceivingSocket, (SOCKADDR *)&ReceiverAddr, sizeof(ReceiverAddr))
       == SOCKET_ERROR)
   {
      printf("ERROR: bind failed with error %d\n", WSAGetLastError());
      closesocket(ReceivingSocket);
      WSACleanup();
      getch();
      return;
   }

   printf("We are ready to receive datagrams from any address on port %d...\n",Port);

   //At this point you can receive datagrams on your bound socket
	while(!kbhit()) {

   	for(i=0;i<256;i++) ReceiveBuf[i] = 0;

      //receive 83P, 83B or 83Z data messages from DeltaT.exe
	   if ((Ret = recvfrom(ReceivingSocket, ReceiveBuf, BufLength, 0,
   	    (SOCKADDR *)&SenderAddr, &SenderAddrSize)) == SOCKET_ERROR)
	   {
   	   printf("ERROR: recvfrom failed with error %d\n", WSAGetLastError());
      	closesocket(ReceivingSocket);
	      WSACleanup();
   	   getch();
      	return;
	   }
      ctr = ((ReceiveBuf[93]<<24) | (ReceiveBuf[94]<<16) |
      		 (ReceiveBuf[95]<<8) | ReceiveBuf[96] );
      printf("%c%c%c - %d",ReceiveBuf[0],ReceiveBuf[1],ReceiveBuf[2],ctr);
		printf("\n");

		//send external control switches to DeltaT.exe
      SendExternalControlSwitches();
	}

   // When your application is finished receiving datagrams close the socket
   closesocket(ReceivingSocket);

   // When your application is finished call WSACleanup
   WSACleanup();

   getch();
}

void SendExternalControlSwitches(void)
{
   int Ret;

	SetExternalControlSwitches();

   if ((Ret = sendto(ReceivingSocket, &SendBuf[0], 256, 0,
   	 (SOCKADDR *)&SenderAddr, sizeof(SenderAddr))) == SOCKET_ERROR)
   {
   	printf("ERROR: sendto failed with error %d\n", WSAGetLastError());
      closesocket(ReceivingSocket);
      WSACleanup();
      getch();
      return;
   }
}

void SetExternalControlSwitches(void)
{
	register int i;

   for(i=0;i<256;i++) SendBuf[i] = 0;

   SendBuf[0] = 'E';
   SendBuf[1] = 'C';
	SendBuf[2] = 0;		//ID

	SendBuf[3] = 1;		//Control Byte 1
                        //Bit0: 0 = LocalControl, 1 = ExternalControl
                        //Bit0 must be set to ExternalControl for Switch
                        //settings to take effect

   SendBuf[4] = 0;		//Control Byte 2
   							//Bit0: 0 = Transmit & Receive, 1 = Receive Only (disable transmitter)

   SendBuf[5] = 0;		//Control Byte 3
   SendBuf[6] = 0;		//Control Byte 4

   SendBuf[7] = 4;		//Range
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
   SendBuf[9] = 50;		//Display Gain, 1 to 100 percent
   SendBuf[10] = 1;		//Gain Equalization, 0=Off, 1=On
   SendBuf[11] = 3;		//Sector Size, 0=30, 1=60, 2=90, 3=120deg
   SendBuf[12] = 3;		//Beamwidth, 0=Wide, 1=Normal, 2=Narrow, 3=Narrow Mixed
   SendBuf[13] = 0;		//Number of Beams, 0=480, 1=240, 2=120
   SendBuf[14] = 5;		//Averaging, 0,1=Off, 2,3,4,...10 = number of shots to average

   SendBuf[15] = (0&0xFF00)>>8;			//Persistence (Hi Byte), 0 to 600sec
   SendBuf[16] = (0&0x00FF);				//Persistence (Lo Byte)

   SendBuf[17] = (15000&0xFF00)>>8;;	//Sound Velocity*10 (Hi Byte), 1400.0 to 1600.0m/s
   SendBuf[18] = (15000&0x00FF);			//Sound Velocity*10 (Lo Byte)
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
   SendBuf[26] = 1;		//Roll Correction, 0=Off, 1=On
   SendBuf[27] = 0;		//Measurement Units, 0=Meters, 1=Feet, 2=Yards
   SendBuf[28] = 0;		//Record Start/Start (.837)

   SendBuf[29] = 0;		//Record Start/Start (.83P)
   //Not implemented

   SendBuf[30] = 0;		//Record Start/Start (.83B)
   //Not implemented

   //The following External Trigger Control Bytes (31-33) are valid only for
   //DeltaT Sonar Heads supplied with the External Trigger Hardware Option
   SendBuf[31] = 0x00;					//External Trigger Control
   											//Bit0, Edge: 	 0=NEG,     1=POS
                        				//Bit1, Enable: 0=Disable, 1=Enable

   SendBuf[32] = (0&0xFF00)>>8;		//External Trigger Transmit Delay (Hi Byte)
   SendBuf[33] = (0&0x00FF);			//External Trigger Transmit Delay (Lo Byte)
   											//0 to 10000 in 100 microsecond increments

   SendBuf[34] = 2;		//Profile Point Filter
   							//0=First Return, 1=Maximum Return, 2=Bottom Following                                    
}
