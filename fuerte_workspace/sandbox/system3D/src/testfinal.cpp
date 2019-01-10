#include "ros/ros.h"
#include "std_msgs/String.h"
#include "sensor_msgs/PointCloud2.h"
#include <pcl/ros/conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <dynamixel_msgs/JointState.h>
#include <pcl/common/transforms.h>

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
pcl::PointCloud<pcl::PointXYZ> cloud_points;

float ang_servo;	

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


		/*	ROS_INFO("Current position (rad): %f", msg->current_pos);
			ROS_INFO("Goal position (rad): %f", msg->goal_pos);
			ROS_INFO("Current speed (rad/s): %f", msg->velocity);
			ROS_INFO("Current position (graus): %f", ang_servo);*/
    			
    		
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
	printf("\n S - Start Scan");
	printf("\n C - Check Servo");
	printf("\n R - Rotate Servo");
	printf("\n X - Exit");
	printf("\n Enter selection: ");
}

int main(int argc, char **argv)
{
  
  	ros::init(argc, argv, "testfinal");
    	ros::NodeHandle nh;
 	//ros::Subscriber sub_sonar = nh.subscribe("cloud_test", 100, sonarCallback);   //recebe a cloud vinda do sonar
	ros::Subscriber sub_servo = nh.subscribe("/tilt_controller/state", 1, servoCallback);   //recebe os dados do servo
	ros::Rate loop_rate(10);

  	sensor_msgs::PointCloud2 msg;

	Dynamixel motor;

	//Final_Image cloud3D;

	char c;

		
	do{
		while (ros::ok()){

			menu();
			c=getchar();

			switch(c){			

			case 'S' :
			case 's' :{
		
				//calibrar servo - virá-lo para a proa - supondo que seja 0 graus
				if (msg_servo.current_pos != 0){
					motor.moveMotor(0); // (rad)
					printf("Sonar not calibrated! Turning sonar to 0 angle..\n\n");
				}
			
				//while(msg_servo.current_pos != msg_servo.goal_pos){printf("\n Current position (graus): %f", ang_servo);}
				printf(" Servo well positioned, press any key to start the 3D scan..\n");

				getchar();

				printf(" Starting rotation of sonar..\n");   
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
				printf("\n Closing program");

			}break;

			default :
				printf("\n Invalid selection");
			}// end of switch

			ros::spinOnce();
			loop_rate.sleep();
			getchar();

		}// end of ros::ok

	}while (c!='x');// end of do

	return 0;
}
