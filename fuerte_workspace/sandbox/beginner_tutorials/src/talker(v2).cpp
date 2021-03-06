#include "ros/ros.h"
#include "std_msgs/String.h"
#include "sensor_msgs/PointCloud2.h"
#include <pcl/ros/conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <dynamixel.h>

#include <sstream>
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
int Ret, num_scans;
int GoalPos[2] = {0,1023};
int CommStatus;

float pontosXX [1000][65536];
float pontosYY [1000][65536];
float pontosZZ [1000][65536];

struct sockaddr_in ReceiverAddr;
struct sockaddr_in SenderAddr,SenderAddr1;

char SendBuf[1024];
unsigned char ReceiveBuf[65536];

void SetExternalControlSwitches(void);
void UdpOut(void);
void PrintErrorCode();
void PrintCommStatus(int CommStatus);


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

}  

int get_value(unsigned char *c){  //shift para a esquerda
	return (c[0]<<8 | c[1]);
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

   SendBuf[8] = 0;		//Gain, 0 to 20dB
   SendBuf[9] = 30;		//Display Gain, 1 to 100 percent
   SendBuf[10] = 0;		//Gain Equalization, 0=Off, 1=On
   SendBuf[11] = 3;		//Sector Size, 0=30, 1=60, 2=90, 3=120deg
   SendBuf[12] = 2;		//Beamwidth, 0=Wide, 1=Normal, 2=Narrow, 3=Narrow Mixed
   SendBuf[13] = 2;		//Number of Beams, 0=480, 1=240, 2=120
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



sensor_msgs::PointCloud2 receive(){

	sensor_msgs::PointCloud2 cloud;
	pcl::PointCloud<pcl::PointXYZ> cloud_pcl;
	Ret=-1;
	int i,j,range_resolution,n_beams;
	unsigned char w;
	float x,y,z,Sound_Velocity,angle_increment,range,starting_angle,corrected_range,pi=3.14159265;	


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
		
		w=ReceiveBuf[83];
		Sound_Velocity=1500.0;

		if (w>127){
			Sound_Velocity = (((ReceiveBuf[83] & 0x7F)<<8) | (ReceiveBuf[84]))/10.0;
		};
		//printf("Sound Velocity :%f \n",Sound_Velocity);  // m/s

		n_beams=get_value(&ReceiveBuf[70]);	//numero de beams
		//printf("beams :%d \n",n_beams);

		angle_increment=((ReceiveBuf[78])/100.0); //em graus
		//printf("angle increment :%f \n",angle_increment);

		starting_angle=get_value(&ReceiveBuf[76])/100-180; //angulo do beam 0
		//printf("starting angle :%f \n",starting_angle); //suposto dar -60graus

		range_resolution=get_value(&ReceiveBuf[85]);
		//printf("range resolution :%d \n",range_resolution); //em mm's
		
		//printf("ReceiveBuf :%c \n",ReceiveBuf);
	    fflush(stdout);
	}

	for(i= 256, j=0; i < Ret; i=i+2, j++){ 
		
		range = get_value(&ReceiveBuf[i]) * range_resolution / 1000.0; // metros
		corrected_range = range * Sound_Velocity / 1500.0;
		//printf("RANGE CORRECTED :%f \n",corrected_range); //em mm's

		if (0.5<corrected_range && corrected_range<50){

		x=corrected_range*cos((pi/180.0)*(starting_angle + j*angle_increment));
		y=corrected_range*sin((pi/180.0)*(starting_angle + j*angle_increment));
		//z=corrected_range*angulo_servo;

		cloud_pcl.push_back(pcl::PointXYZ (x,y,z));
		//printf("angulo: %f \n",((pi/180.0)*(starting_angle + j*angle_increment)));
		}
		
		//msg.fields.data[i-256]=ReceiveBuf[i];
		
		//guarda todos os pontos para depois fazer a imagem 3D
		pontosXX[num_scans][j]=x; pontosYY[num_scans][j]=y; pontosZZ[num_scans][j]=z;  
		

	}

	//Input of the above cloud_pcl and the corresponding output of cloud(pcl2)
	pcl::toROSMsg(cloud_pcl, cloud);

	
	return cloud;
}



void UdpOut(void)
{

	//send External controls
	SetExternalControlSwitches();

	Ret = -1;

	Ret = -1;

	Ret = sendto(ReceiveSocket, SendBuf, 256, 0,
			(struct sockaddr *)&SenderAddr1, sizeof(struct sockaddr));


	printf("udp send bytes %d !\n", Ret);
	fflush(stdout);



}

void welcome(void){
	
	// mostrar dados do servo - posicao actual

	

	printf(" \n\n\n");
	printf("************* WELCOME **************\n");
	printf(" Press 'S' key to Start 3D scanning...\n");
	printf(" Press 'M' key to set sonar moving speed.\n");
	printf(" Press 'C' key at any time to Close program.\n\n");
	printf("September, 2014 \n");
	printf("************************************\n\n\n");

}

bool scan(){

	//enquanto nao concluir os 360º -> False
	if 
	while

}	

int main(int argc, char **argv)
{
  
  	ros::init(argc, argv, "talker");
  
	ros::NodeHandle nh;

  	ros::Publisher sonar_pub = nh.advertise<sensor_msgs::PointCloud2>("Sonar_points", 1000);

  	sensor_msgs::PointCloud2 msg;

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

	if (bind(ReceiveSocket,(struct sockaddr *)&ReceiverAddr,sizeof(struct sockaddr)) == -1){
		perror("Bind");
		fflush(stdout);
		exit(1);
	}
	
	printf("ReceiveSocket :%d \n",ReceiveSocket);

	//UdpOut();   //sempre que qiser dar uma instrucao nova ao sonar

/////////// socket OK -> proceed ///////////


	while(userInput != 'c'){             // ou -> char c= getchar();    while(c!='c'){}

	    	welcome();

	    	if (userInput='s'){

			printf(" Starting rotation of sonar\n");   // mais á frente pedir velocidade de rotacao ao user
	
			//reset array points
			memset(point, 0, sizeof(pontosXX));
			memset(point, 0, sizeof(pontosYY));
			memset(point, 0, sizeof(pontosZZ));


			//calibrar servo - virá-lo para a proa - supondo que seja 0 graus
			if (PresentPos != 0 && PresentPos >){

			
			//iniciar contador de scans completos do sonar
			num_scans= 0;
		
			while(scan){  
	
		    		msg=receive();
			    	num_scans++;
		
			    	msg.header.stamp=ros::Time::now();
			    	msg.header.frame_id="sonar";
			    	ros::Rate loop_rate(10);
		
			    	sonar_pub.publish(msg);  
			    	//ros::spin();
		
			 	ros::spinOnce();
    			 	loop_rate.sleep();

			 	//UdpOut();
			}
		}
		
  	}

//////// CLOSE PROGRAM ///////

    getchar(); //?

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

	dxl_terminate(); // close servo

  return 0;
}


