#include "ros/ros.h"
#include "std_msgs/String.h"
#include "sensor_msgs/PointCloud2.h"
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <dynamixel_msgs/JointState.h>
#include <pcl/common/transforms.h>
#include <pcl/io/pcd_io.h>

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
#include <std_msgs/Float64.h>

#define	pi 3.14159265

dynamixel_msgs::JointState msg_servo;
sensor_msgs::PointCloud2 cloud_sonar;
Eigen::Matrix4f transformationMatrix;	
pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_points (new pcl::PointCloud<pcl::PointXYZ>);
pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
float ang_servo;
int scan;

using namespace std;

void welcome(void);
void sonarCallback(const sensor_msgs::PointCloud2::ConstPtr& msg);
void servoCallback(const dynamixel_msgs::JointState::ConstPtr& msg);


class Dynamixel{
	private:
		ros::NodeHandle nh;
		ros::Publisher Servo_pub;
	public:
		Dynamixel();
		void moveMotor(double position);
};

	Dynamixel::Dynamixel(){
		Servo_pub = nh.advertise<std_msgs::Float64>("/tilt_controller/command", 1);  // para publicar dados no servo
	}

	void Dynamixel::moveMotor(double position){
		std_msgs::Float64 aux;
		aux.data = position;
		Servo_pub.publish(aux);
	}



void servoCallback(const dynamixel_msgs::JointState::ConstPtr& msg)
{

	msg_servo.current_pos= msg->current_pos;
	msg_servo.goal_pos= msg->goal_pos;
	msg_servo.velocity= msg->velocity;
	msg_servo.is_moving= msg->is_moving;
	ang_servo = (msg_servo.current_pos)*(180/pi);		    //graus=rads*180/pi


	if (ang_servo<359.0 && ang_servo>1.0){
		//******* cria logo a cloud 3D e guarda-a num ficheiro .pcd ***********
			
		//concatenate clouds
		*cloud = (*cloud) + (*cloud_points);
		printf(".");
	}

					
			
}

void sonarCallback(const sensor_msgs::PointCloud2::ConstPtr& msg){
	
	
	pcl::fromROSMsg(*msg, *cloud_points);    		//converter a cloud pcl2 lida do sonar para pcl(x,y,z)
	
}


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

void menu(void){
	printf("\n Menu");
	printf("\n========");
	printf("\n G - Go Scan");
	printf("\n C - Check Servo");
	printf("\n R - Rotate Servo");
	printf("\n S - Save Cloud (.pcd)");
	printf("\n X - Exit");
	printf("\n Enter selection: ");
}

int main(int argc, char **argv)
{
  
  	ros::init(argc, argv, "testfinal");
    	ros::NodeHandle nh;
 	ros::Subscriber sub_img_transformada = nh.subscribe("Img_transformada", 1000, sonarCallback);   //recebe a cloud vinda do sonar
	ros::Subscriber sub_servo = nh.subscribe("/tilt_controller/state", 1, servoCallback);   //recebe os dados do servo
	ros::Rate loop_rate(10);

  	sensor_msgs::PointCloud2 msg;

	Dynamixel motor;

	//Final_Image cloud3D;

	char c;
	scan= 0;
		
	do{
		while (ros::ok()){

			if (scan!=1){
		
			menu();
			c=getchar();

			switch(c){			

			case 'G' :
			case 'g' :{
				cloud->points.clear(); 	   //reset á cloud;
				//calibrar servo - virá-lo para a proa - supondo que seja 0 graus
				if (msg_servo.current_pos != 0){
					motor.moveMotor(0); // (rad)
					printf("Sonar not calibrated! Turning sonar to 0 angle..\n\n");
					sleep(6);
				}
			

				printf(" Starting sonar scan..\n");   
				scan=1;    //scan is ON
				motor.moveMotor(2*pi);  			//360º

				//sleep(5);
					
				
			}break;

			case 'C' :
			case 'c' :{
				printf("\n\n Current position (rad): %f", msg_servo.current_pos);
				printf("\n Goal position (rad): %f", msg_servo.goal_pos);
				printf("\n Current position (graus): %f", ang_servo);
				printf("\n Speed: %f \n", msg_servo.velocity);
				printf("\n Moving: %d \n", msg_servo.is_moving);
				scan=0;

			}break;

			case 'S' :
			case 's' :{
				
				scan=0;
				
				// SAVE CLOUD
				// Another possibility would be "savePCDFileBinary()".
				pcl::io::savePCDFileBinary("output.pcd", *cloud);
				printf("\nCloud saved in catkin workspace\n");
				cloud->points.clear(); 	   //reset á cloud;
			}break;

			case 'R' :
			case 'r' :{
				float angulo;
				printf("\n Rotate to desired angle(º): ");
				scanf("%f", &angulo);
				motor.moveMotor(angulo*(pi/180));

			}break;

			case 'X' :
			case 'x' :{
				printf("\n Closing program\n");

			}break;

			default :
				printf("\n Invalid selection");
			}// end of switch

			ros::spinOnce();
			loop_rate.sleep();
			getchar();

			}else{

			if (ang_servo>358){scan=0;}
			ros::spinOnce();
			//loop_rate.sleep();
			}// end of if scan

			if (c='x'){break;}

		}// end of ros::ok

	}while (c!='x');// end of do

	return 0;
}
