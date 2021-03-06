#include "ros/ros.h"
#include "std_msgs/String.h"
#include "sensor_msgs/PointCloud2.h"
#include <pcl/conversions.h>
#include <pcl_conversions/pcl_conversions.h>
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

float ang_servo;	

using namespace std;

void welcome(void);
void sonarCallback(const sensor_msgs::PointCloud2::ConstPtr& msg);
void servoCallback(const dynamixel_msgs::JointState::ConstPtr& msg);



sensor_msgs::PointCloud2 receive(){

	sensor_msgs::PointCloud2 cloud;
	
	pcl::PointCloud<pcl::PointXYZ> cloud_points;

	pcl::PointCloud<pcl::PointXYZ> rotatedCloud;
	
	pcl::fromROSMsg(cloud_sonar, cloud_points);    		//converter a cloud pcl2 lida do sonar para pcl(x,y,z)

	pcl::transformPointCloud(cloud_points,rotatedCloud,transformationMatrix);  //rodar a cloud - aplicar a transformada

	//cloud_pcl.push_back(pcl::PointXYZ (x,y,z));		

	pcl::toROSMsg(rotatedCloud, cloud);			//converter a nova cloud 3D para pcl2 (output)
	
	return cloud;
}

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

class Final_Image{
		private:
			ros::NodeHandle nh;
			ros::Publisher Final_Image_pub;
		public:
			Final_Image();
			void publishCloud3D(void);
};

	Final_Image::Final_Image(){
		Final_Image_pub = nh.advertise<sensor_msgs::PointCloud2>("Img_final", 1000);  //publica a cloud 3D
	}

	void Final_Image::publishCloud3D(void){
		sensor_msgs::PointCloud2 aux;
		aux=receive();
		aux.header.stamp=ros::Time::now();
		aux.header.frame_id="system3D";
		Final_Image_pub.publish(aux);
	} 




 
void sonarCallback(const sensor_msgs::PointCloud2::ConstPtr& msg)
{
	if (msg_servo.current_pos != 6.283185307){ 
		cloud_sonar=*msg;
		ROS_INFO("Scan is ON");
	}
	else ROS_INFO("Scan is OFF");	
}

void servoCallback(const dynamixel_msgs::JointState::ConstPtr& msg)
{
	ROS_INFO("Current position (rad): %f", msg->current_pos);
	ROS_INFO("Goal position (rad): %f", msg->goal_pos);
	//ROS_INFO("Current speed (rad/s): %f", msg->speed);

	float ang_servo = (msg->current_pos)*(180/pi);		    //graus=rads*180/pi
	ROS_INFO("Current position (graus): %f", ang_servo);
	
	msg_servo=*msg;

	//matriz de rotacao em torno do eixo Z

	transformationMatrix <<
                cos(ang_servo),  -sin(ang_servo), 0, 0,
                sin(ang_servo),   cos(ang_servo), 0, 0,
                             0,                0, 1, 0,
		             0,                0, 0, 1;
}



void welcome(void){
	
	printf(" \n\n\n");
	printf("***************** WELCOME ******************\n");
	printf(" Press 'S' key to Start 3D scanning.\n");
	printf(" Press 'C' key at any time to Close program.\n");
	printf("************* September, 2014 **************\n\n\n");

}	

int main(int argc, char **argv)
{
  
  	ros::init(argc, argv, "System3D");
  
	ros::NodeHandle nh;

	ros::Subscriber sub_sonar = nh.subscribe("cloud_test", 1000, sonarCallback);   //recebe a cloud vinda do sonar

	ros::Subscriber sub_servo = nh.subscribe("/tilt_controller/state", 1, servoCallback);   //recebe os dados do servo

  	//sensor_msgs::PointCloud2 msg;

	Dynamixel motor;

	Final_Image cloud3D;

	char c;


	while(c != 'c'){             // ou -> char c= getchar();    while(c!='c'){}
		
	    	welcome();
		c=getchar();

	    	if (c = 's'){


			//calibrar servo - virá-lo para a proa - supondo que seja 0 graus
			if (msg_servo.current_pos != 0){
				motor.moveMotor(0); // (rad)
				printf(" Turning sonar to 0 angle..\n\n");
			}

			printf(" Starting rotation of sonar..\n");   
			motor.moveMotor(6.283185307);  			//360º
			
			int num_scans= 0;
		
			while(msg_servo.current_pos != 6.283185307){  
	
		    		//msg=receive();
			    	num_scans++;
		
			    	//msg.header.stamp=ros::Time::now();
			    	//msg.header.frame_id="system3D";
			    	ros::Rate loop_rate(10);
		
				cloud3D.publishCloud3D();
			    	//3D_pub.publish(msg);  
			    	//ros::spin();
		
			 	ros::spinOnce();
    			 	loop_rate.sleep();
			}
		}
		
  	}

//////// CLOSE PROGRAM ///////

    //getchar(); 

  return 0;
}


