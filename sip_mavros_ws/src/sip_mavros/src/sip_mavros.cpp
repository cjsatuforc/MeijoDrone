/*Low-pass filter is initialized "0", therefore it took a little time to be good condition.
 */
/**
 *  * @file offb_node.cpp
 *   * @brief offboard example node, written with mavros version 0.14.2, px4 flight
 *    * stack and tested in Gazebo SITL
 *     */
#include <math.h> // atan2()
#include <limits.h> // isnan()
#include <stdlib.h>
#include <string>
#include <vector>
#include <time.h> // clock_t
#include <tinyxml.h>

#include <ros/ros.h>
#include <std_msgs/Bool.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Int8.h>
#include <std_msgs/Int16.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Point.h>
#include <geometry_msgs/Vector3.h>
#include <sensor_msgs/Imu.h>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/RCIn.h>
#include <mavros_msgs/AttitudeTarget.h>
#include <tf2/LinearMath/Transform.h>
#include <tf/transform_listener.h>
#include <geometry_msgs/WrenchStamped.h>

#include "sip_mavros/PID.h"
//#include "sip_mavros/kalman_filter.h" conflict to estimate_position
#include "sip_mavros/estimate_position.hpp"

#include <sensor_msgs/BatteryState.h>

/*
double logxp=0;
double logxv=0;
double logxa=0;
double logyp=0;
double logyv=0;
double logya=0;
double logroll=0;
double logpitch=0;
double inner_des_alt=0;
double desalt=0;
double crralt=0;
double logzv=0;
double logza=0;
double desx=0;
double desy=0;
*/
char temp_char;

class SipMavrosMem{
public:
    ros::NodeHandle nh;
    ros::Subscriber state_sub;
    ros::Subscriber target_point_sub;
    ros::Subscriber target_yaw_sub;
    ros::Subscriber quit_sub;
    ros::Subscriber rc_sub;
    ros::Subscriber battery_sub;
    ros::Subscriber slam_sub;
    ros::Subscriber ground_sub;
    ros::Subscriber imu_sub;
    //for tf
    ros::Publisher reset_init_pub;
    std_msgs::Bool reset_init;
    
    //order to Pixhawk
    ros::Publisher attitude_pub;
	ros::Publisher throttle_pub;
    ros::Publisher target_pub;
    ros::ServiceClient arming_client;
    ros::ServiceClient set_mode_client;
    ros::Rate rate;
    mavros_msgs::State current_state;

    mavros_msgs::RCIn rc_raw;
    bool rc_accept;
    bool rc_direct_roll;
    bool rc_direct_pitch;
    double rc_roll;
    double rc_pitch;
    double rc_yaw;
    double rc_throttle_rate;
    bool rc_land;
    bool rc_exit;
    bool rc_interrupt;
    bool rc_override;
    bool rc_arm;
    
    double roll_ctrl;
    double pitch_ctrl;
    double yaw_ctrl;
    double fixed_alt;
    double throttle_ave;
    double throttle_pid;
    double landing_alt;
    
    Ground_class ground;
    
    /*slam related*/
    TimeSet slam_tm;
    geometry_msgs::Point slam_point_current;
    geometry_msgs::Point slam_point_past;
    geometry_msgs::Point slam_point_velocity;
    geometry_msgs::Vector3 slam_attitude_current;
    geometry_msgs::Vector3 slam_attitude_past;
    geometry_msgs::Vector3 slam_attitude_velocity;
    TimeSet slam_add_tm;
    geometry_msgs::Vector3 slam_position;
    int slam_flag;
    /*end slam related*/
    
    geometry_msgs::Point initial_point;
    double initial_compass_yaw;
    geometry_msgs::Point target_point;
    std_msgs::Float64 target_yaw;

    geometry_msgs::PoseStamped target_attitude;
    std_msgs::Float64 target_throttle;
    mavros_msgs::AttitudeTarget mavros_target;
    
    double compass_roll;
    double compass_pitch;
    double compass_yaw;
    double current_roll;
    double current_pitch;
    double current_yaw;
    
    float battery_percent;
    
    TimeSet imu_tm;
    geometry_msgs::Vector3 body_attitude;
    sensor_msgs::Imu body_imu;
    geometry_msgs::Vector3 accel;
    LPF imuLPFX;
    LPF imuLPFY;
    LPF imuLPFZ;
    
    PID xp_pid;
    PID xv_pid;
    PID xa_pid;
    PID yp_pid;
    PID yv_pid;
    PID ya_pid;
    PID yaw_pid;
    PID alt_pid;
    PID altv_pid;
    PID alta_pid;

    bool quit_flg;
    bool fcu_is_connected;

    tf::TransformListener listener;
    tf::StampedTransform current_transform;
    tf::StampedTransform compass_transform;
    
    geometry_msgs::Vector3 attitude_rate;
    
    SipMavrosMem(const std::string &target_point_topic
                 , const std::string &target_yaw_topic
                 , const PID &xp_pid_
                 , const PID &xv_pid_
                 , const PID &xa_pid_
                 , const PID &yp_pid_
                 , const PID &yv_pid_
                 , const PID &ya_pid_
                 , const PID &alt_pid_
                 , const PID &altv_pid_
                 , const PID &alta_pid_
                 , const PID &yaw_pid_
                 , double battery_coefficient_
                 , double ground_effect_coefficient_
                 , bool rc_accept_
                 , bool rc_direct_roll_
                 , bool rc_direct_pitch_
                 , double control_freq_
                 );
    void starting();
    void shutdown();
    
    void updateErr();
    void publishCtrlInput();

    bool ok(){
        return (! quit_flg) && current_state.armed;
    }

    void state_cb(const mavros_msgs::StateConstPtr &msg){
        current_state = *msg;
        fcu_is_connected = current_state.connected;
    }
    
    bool updateTransform(){
        try{
            listener.lookupTransform("map", "base_frame", ros::Time(0), current_transform);
				listener.lookupTransform("map", "fc_imu", ros::Time(0), compass_transform);
			}
			catch(tf::TransformException ex){
				ROS_ERROR("YAMAKAZE> %s.", ex.what());
				return false;
			}
        return true;
    }
    
    void target_point_cb(const geometry_msgs::PointConstPtr &msg){
        target_point.x = msg->x;
        target_point.y = msg->y;
        target_point.z = msg->z;
    }

    void target_yaw_cb(const std_msgs::Float64ConstPtr &msg){
        target_yaw.data = msg->data;
        
    }

    void quit_cb(const std_msgs::EmptyConstPtr &msg){
        quit_flg = true;
    }

    void rc_cb(const mavros_msgs::RCInConstPtr &msg){
        /*     pitch            throttle        */
        /*          1094              1934      */
        /*          ^                 ^         */
        /*          | yaw             | roll     */
        /*   1094 <-o-> 1924   1094 <-o-> 1934  */
        /*          |                 |         */
        /*          v                 v         */
        /*          1934              1094      */
        /*                                      */
        /* RC Joystic Layout and boundary value */

        if(rc_accept){
            rc_raw = *msg;
            
            if(rc_raw.channels[13] > 1450){
                rc_land = true;
            }else{
                rc_land = false;
            }
            
            /*
            if(rc_raw.channels[10] > 1450){
                rc_exit = true;
            }
            */
             
            if((not rc_override) and rc_raw.channels[8] > 1450){
                rc_override = true;
            }
            if(rc_override and rc_raw.channels[8] < 1450){
                rc_override = false;
            }

            if((not rc_override)
               and (
                    ((not rc_direct_roll)
                     and fabs(rc_raw.channels[0] - 1514) > 50)
                    or
                    ((not rc_direct_pitch)
                     and fabs(rc_raw.channels[1] - 1514) > 50))){
                        rc_interrupt = true;
                    }
            rc_roll = remap((double)(rc_raw.channels[0])
                            , 1934, 1094
                            , 7.0 * M_PI / 180.0
                            , -7.0 * M_PI / 180.0);
            
            rc_pitch = -1.0 * remap((double)(rc_raw.channels[1])
                                    , 1934, 1094
                                    , 7.0 * M_PI / 180.0
                                    , -7.0 * M_PI / 180.0);
            
            rc_yaw = -1.0 * remap((double)(rc_raw.channels[3])
                                  , 1934, 1094
                                  , 7.0 * M_PI / 180.0
                                  , -7.0 * M_PI / 180.0);
            
            if(rc_yaw >= M_PI / 2){
                rc_yaw = M_PI / 2;
            }
            if(rc_yaw <= -M_PI / 2){
                rc_yaw = -M_PI / 2;
            }
                
            rc_throttle_rate = remap((double)(rc_raw.channels[5])
                                     , 1745, 1283
                                     , 1.0 , 0.0);
            
            if(rc_raw.channels[6] > 1450){
                rc_arm = true;
                rc_exit = false;
            }else{
                rc_arm = false;
                rc_exit = true;
            }
        }
    }

    void battery_cb(const sensor_msgs::BatteryStateConstPtr &msg){
        battery_percent = msg->percentage;
    }
    
    void slam_cb(const geometry_msgs::PoseStamped &msg){
        slam_flag = 1;
        
        slam_tm.update();
        
        slam_point_past = slam_point_current;
        slam_attitude_past = slam_attitude_current;
        
        slam_point_current = msg.pose.position;
        Quat2RollPitchYaw(msg.pose.orientation, slam_attitude_current.x, slam_attitude_current.y, slam_attitude_current.z);
        
        slam_point_velocity.x = (slam_point_current.x - slam_point_past.x)/slam_tm.diff;
        slam_point_velocity.y = (slam_point_current.y - slam_point_past.y)/slam_tm.diff;
        slam_point_velocity.z = (slam_point_current.z - slam_point_past.z)/slam_tm.diff;
        slam_attitude_velocity.x = (slam_attitude_current.x - slam_attitude_past.x)/slam_tm.diff;
        slam_attitude_velocity.y = (slam_attitude_current.y - slam_attitude_past.y)/slam_tm.diff;
        slam_attitude_velocity.z = (slam_attitude_current.z - slam_attitude_past.z)/slam_tm.diff;
        
        slam_position.x = slam_point_current.x;
        slam_position.y = slam_point_current.y;
        slam_position.z = slam_point_current.z;
    }


    void ground_cb(const std_msgs::Float64 &msg){
        double temp = (double)msg.data;
        ground.update(temp, body_attitude.y, body_attitude.x);//(alt, roll, pitch)
    }
    

    void imu_cb(const sensor_msgs::Imu& msg){
        imu_tm.update();
        body_imu = msg;
        Quat2RollPitchYaw(msg.orientation, body_attitude.x, body_attitude.y, body_attitude.z);
        attitude_rate.x = 0.8*attitude_rate.x + 0.2*msg.angular_velocity.x;
        attitude_rate.y = 0.8*attitude_rate.y + 0.2*msg.angular_velocity.y;
        attitude_rate.y = 0.8*attitude_rate.y + 0.2*msg.angular_velocity.y;
        
        double temp_roll = body_attitude.x;
        double temp_pitch = body_attitude.y;
        double temp_yaw = body_attitude.z;
        
        accel.x = imuLPFX.filter( (double)(msg.linear_acceleration.x) + gravity*sin(temp_pitch) );
        accel.y = imuLPFY.filter( (double)(msg.linear_acceleration.y) + gravity*sin(-temp_roll) );
        accel.z = imuLPFZ.filter( (double)(msg.linear_acceleration.z) - gravity *cos(temp_roll)*cos(temp_pitch) );

        slam_point_velocity.x += (accel.x * cos(slam_attitude_current.z) - accel.y * sin(slam_attitude_current.z)) * imu_tm.diff;
        slam_point_velocity.y += (accel.x * sin(slam_attitude_current.z) + accel.y * cos(slam_attitude_current.z)) * imu_tm.diff;
        
    }
    
		static geometry_msgs::Point averagePoint(
				const std::vector<tf::Vector3> &points);
		static double averageYaw(const std::vector<double> &yaws);
};

SipMavrosMem::SipMavrosMem(
        const std::string &target_point_topic
		, const std::string &target_yaw_topic
		, const PID &xp_pid_
        , const PID &xv_pid_
        , const PID &xa_pid_
		, const PID &yp_pid_
        , const PID &yv_pid_
        , const PID &ya_pid_
		, const PID &alt_pid_
        , const PID &altv_pid_
                           , const PID &alta_pid_
		, const PID &yaw_pid_
		, double battery_coefficient_
		, double ground_effect_coefficient_
		, bool rc_accept_
		, bool rc_direct_roll_
		, bool rc_direct_pitch_
		, double control_freq_
		)
	: nh()
        , state_sub()
        , target_point_sub()
        , target_yaw_sub()
	    , quit_sub()
	    , rc_sub()
	    , battery_sub()
        , slam_sub()
        , ground_sub()
        , imu_sub()

        , reset_init_pub()
        , reset_init()

	    , attitude_pub()
	    , throttle_pub()
, target_pub()
	    , arming_client()
	    , set_mode_client()
	    , rate(control_freq_) // must faster than 2 Hz
	    , current_state()
        , rc_raw()
	    , rc_accept(rc_accept_)
	    , rc_direct_roll(rc_direct_roll_)
	    , rc_direct_pitch(rc_direct_pitch_)
	    , rc_throttle_rate(0.0)
	    , rc_roll(0.0)
	    , rc_pitch(0.0)
        , rc_yaw(0.0)
	    , rc_land(false)
	    , rc_exit(false)
	    , rc_interrupt(false)
	    , rc_override(false)
        , rc_arm(false)

, roll_ctrl(0.0)
, pitch_ctrl(0.0)
, yaw_ctrl(0.0)
, fixed_alt(0.0)

        , throttle_ave(0.4)
        , throttle_pid(0.0)
        , landing_alt(0)
        , ground(0.02)

        , slam_tm(0.1)
        , slam_point_current()
        , slam_point_past()
        , slam_point_velocity()
        , slam_attitude_current()
        , slam_attitude_past()
        , slam_attitude_velocity()
        , slam_add_tm(0.01)
        , slam_position()
        , slam_flag(0)

        , initial_point()
	    , initial_compass_yaw(0.0)
	    , target_point()
	    , target_yaw()
	    , target_attitude()
	    , target_throttle()
, mavros_target()
	    , compass_roll(0.0)
	    , compass_pitch(0.0)
	    , compass_yaw(0.0)
	    , current_roll(0.0)
	    , current_pitch(0.0)
	    , current_yaw(0.0)

        , battery_percent(0.0)

        , imu_tm(0.01)
        , body_attitude()
        , body_imu()
        , accel()
        , imuLPFX(3)
        , imuLPFY(3)
        , imuLPFZ(3)

	    , xp_pid(xp_pid_)
        , xv_pid(xv_pid_)
        , xa_pid(xa_pid_)
	    , yp_pid(yp_pid_)
        , yv_pid(yv_pid_)
        , ya_pid(ya_pid_)
	    , yaw_pid(yaw_pid_)
	    , alt_pid(alt_pid_)
        , altv_pid(altv_pid_)
        , alta_pid(alta_pid_)

	    , quit_flg(false)
	    , fcu_is_connected(false)
	    , listener()
	    , current_transform()
	    , compass_transform()
        , attitude_rate()
{
	state_sub
		= nh.subscribe<mavros_msgs::State>(
				"mavros/state", 10, &SipMavrosMem::state_cb, this);
	target_point_sub
		= nh.subscribe<geometry_msgs::Point>(
				target_point_topic.c_str(), 10
				, &SipMavrosMem::target_point_cb, this);
	target_yaw_sub
		= nh.subscribe<std_msgs::Float64>(
				target_yaw_topic.c_str(), 10
				, &SipMavrosMem::target_yaw_cb, this);
	quit_sub
		= nh.subscribe<std_msgs::Empty>(
				"/sip/quit", 10
				, &SipMavrosMem::quit_cb, this);
	rc_sub
		= nh.subscribe<mavros_msgs::RCIn>(
				"/mavros/rc/in", 10
				, &SipMavrosMem::rc_cb, this);

	battery_sub
		= nh.subscribe<sensor_msgs::BatteryState>(
				"/mavros/battery", 10
				, &SipMavrosMem::battery_cb, this);
    
    slam_sub = nh.subscribe("/slam_out_pose", 1, &SipMavrosMem::slam_cb, this);

    ground_sub = nh.subscribe("/altitude", 10, &SipMavrosMem::ground_cb, this);
    
    imu_sub = nh.subscribe("/mavros/imu/data", 10, &SipMavrosMem::imu_cb, this);
    
    reset_init_pub = nh.advertise<std_msgs::Bool>("/sip/reset_initial", 1);
    
	attitude_pub
		= nh.advertise<geometry_msgs::PoseStamped>(
				"mavros/setpoint_attitude/attitude", 100);
	throttle_pub
		= nh.advertise<std_msgs::Float64>(
				"mavros/setpoint_attitude/att_throttle", 100);
    
    target_pub = nh.advertise<mavros_msgs::AttitudeTarget>("mavros/setpoint_raw/attitude",100);
    
	arming_client
		= nh.serviceClient<mavros_msgs::CommandBool>(
				"mavros/cmd/arming");
	set_mode_client
		= nh.serviceClient<mavros_msgs::SetMode>(
				"mavros/set_mode");
    
	ROS_INFO("YAMAKAZE> Set PID Parameters.");
	ROS_DEBUG("SYSTEM> position Pgain Igain Dgain PreDgain HighLimit LowLimit\n");
	ROS_DEBUG("X %f %f %f %f %f %f\n", xp_pid.p_gain, xp_pid.i_gain, xp_pid.d_gain, xp_pid.pre_d_gain, xp_pid.ctrl_upper_limit, xp_pid.ctrl_lower_limit);
	ROS_DEBUG("Y %f %f %f %f %f %f\n", yp_pid.p_gain, yp_pid.i_gain, yp_pid.d_gain, yp_pid.pre_d_gain, yp_pid.ctrl_upper_limit, yp_pid.ctrl_lower_limit);
	ROS_DEBUG("Z %f %f %f %f %f %f\n", alt_pid.p_gain, alt_pid.i_gain, alt_pid.d_gain, alt_pid.pre_d_gain, alt_pid.ctrl_upper_limit, alt_pid.ctrl_lower_limit);
	ROS_DEBUG("YAW %f %f %f %f %f %f\n", yaw_pid.p_gain, yaw_pid.i_gain, yaw_pid.d_gain, yaw_pid.pre_d_gain, yaw_pid.ctrl_upper_limit, yaw_pid.ctrl_lower_limit);

	ROS_INFO("YAMAKAZE> Set Machine Parameter.");

}


void SipMavrosMem::starting(){
	ROS_INFO("YAMAKAZE> Start Initilize Phase.");
	ROS_INFO("YAMAKAZE> Waiting for FCU connection.");
	while(ros::ok() && not fcu_is_connected){
		rate.sleep();
	}

	tf::StampedTransform initial_transform;
	ROS_INFO("YAMAKAZE> Waiting for Complation tf tree.");
	while(ros::ok()){
		rate.sleep();
		if(updateTransform()) { break; }
	}


	ROS_INFO("YAMAKAZE> Sending a few setpoints before starting.");
	target_attitude.header.stamp = ros::Time::now();
	tf::Quaternion q = compass_transform.getRotation();
	target_attitude.pose.orientation.x = q.getX(); 
	target_attitude.pose.orientation.y = q.getY(); 
	target_attitude.pose.orientation.z = q.getZ(); 
	target_attitude.pose.orientation.w = q.getW(); 
	std::vector<double> initial_compass_yaw_buff;
	std::vector<tf::Vector3> initial_point_buff;

	target_throttle.data = 0.3;
	for(int i = 150; ros::ok() && i > 0; --i){
		while(ros::ok()){
			rate.sleep();
			if(updateTransform()){break;}
		}
		double roll, pitch, yaw;
		tf::Matrix3x3(
				compass_transform.getRotation()).getRPY(
				roll, pitch, yaw);
		initial_compass_yaw_buff.push_back(yaw);

        target_attitude.header.stamp = ros::Time::now();//ikeda
        roll=0;
        pitch=0;
       	RollPitchYaw2Quat(target_attitude.pose.orientation, roll, pitch, yaw);

		initial_point_buff.push_back(current_transform.getOrigin());
//		attitude_pub.publish(target_attitude);
//		throttle_pub.publish(target_throttle);
        
        mavros_target.header.stamp = ros::Time::now();
        mavros_target.orientation = target_attitude.pose.orientation;
        mavros_target.thrust = target_throttle.data;
        target_pub.publish(mavros_target);
        
		rate.sleep();
	}
    
	ROS_INFO("YAMAKAZE> Set Offboard mode.");
	ros::Time last_request = ros::Time::now();
	while(ros::ok() && current_state.mode != "OFFBOARD"){
        
        printf("waiting Offboard mode\n");
        printf("Current is %s\n\n", current_state.mode.c_str());
		while(ros::ok()){
            target_attitude.header.stamp = ros::Time::now();//ikeda
//            attitude_pub.publish(target_attitude);
//            throttle_pub.publish(target_throttle);
            
            mavros_target.header.stamp = ros::Time::now();
            mavros_target.orientation = target_attitude.pose.orientation;
            mavros_target.thrust = target_throttle.data;
            target_pub.publish(mavros_target);

			rate.sleep();
			if(updateTransform()){break;}
		}
        double roll, pitch, yaw;
		tf::Matrix3x3(
				compass_transform.getRotation()).getRPY(
				roll, pitch, yaw);
        target_attitude.header.stamp = ros::Time::now();//ikeda
        roll=0;
        pitch=0;
       	RollPitchYaw2Quat(target_attitude.pose.orientation, roll, pitch, yaw);
		initial_compass_yaw_buff.push_back(yaw);
		initial_point_buff.push_back(current_transform.getOrigin());
//		attitude_pub.publish(target_attitude);
//		throttle_pub.publish(target_throttle);
        
        mavros_target.header.stamp = ros::Time::now();
        mavros_target.orientation = target_attitude.pose.orientation;
        mavros_target.thrust = target_throttle.data;
        target_pub.publish(mavros_target);
        
		rate.sleep();
	}
    
    while(ros::ok() && rc_arm){//ikeda
        ROS_INFO("Reset RC Arm Stick");
        target_attitude.header.stamp = ros::Time::now();//ikeda
//        attitude_pub.publish(target_attitude);
//        throttle_pub.publish(target_throttle);
        
        mavros_target.header.stamp = ros::Time::now();
        mavros_target.orientation = target_attitude.pose.orientation;
        mavros_target.thrust = target_throttle.data;
        target_pub.publish(mavros_target);
        
        rate.sleep();
    }
    while(ros::ok() && not rc_arm){//ikeda
        ROS_INFO("Waiting to Arm");
        target_attitude.header.stamp = ros::Time::now();//ikeda
//        attitude_pub.publish(target_attitude);
//        throttle_pub.publish(target_throttle);
        
        mavros_target.header.stamp = ros::Time::now();
        mavros_target.orientation = target_attitude.pose.orientation;
        mavros_target.thrust = target_throttle.data;
        target_pub.publish(mavros_target);

        rate.sleep();
    }
    
	ROS_INFO("YAMAKAZE> Arm Vehicle.");
	mavros_msgs::CommandBool arm_cmd;
	arm_cmd.request.value = true;
	while(ros::ok() && !current_state.armed){

        if(not rc_arm){//ikeda
            ROS_INFO("Waiting to Arm");
            target_attitude.header.stamp = ros::Time::now();//ikeda
//            attitude_pub.publish(target_attitude);
//            throttle_pub.publish(target_throttle);
            
            mavros_target.header.stamp = ros::Time::now();
            mavros_target.orientation = target_attitude.pose.orientation;
            mavros_target.thrust = target_throttle.data;
            target_pub.publish(mavros_target);

            rate.sleep();
            continue;
        }
        
        
		if(ros::Time::now() - last_request > ros::Duration(5.0)){
			last_request = ros::Time::now();
			int call_success = arming_client.call(arm_cmd);
			if(call_success){
				ROS_INFO("SYSTEM> Vehicle armed");
			}
		}
		while(ros::ok()){
			rate.sleep();
			if(updateTransform()){break;}
		}
		double roll, pitch, yaw;
		tf::Matrix3x3(
				compass_transform.getRotation()).getRPY(
				roll, pitch, yaw);
        target_attitude.header.stamp = ros::Time::now();//ikeda
		initial_compass_yaw_buff.push_back(yaw);
		initial_point_buff.push_back(current_transform.getOrigin());
//		attitude_pub.publish(target_attitude);
//		throttle_pub.publish(target_throttle);
        
        mavros_target.header.stamp = ros::Time::now();
        mavros_target.orientation = target_attitude.pose.orientation;
        mavros_target.thrust = target_throttle.data;
        target_pub.publish(mavros_target);

		rate.sleep();
	}
    
    reset_init.data = true;
    reset_init_pub.publish(reset_init);

	initial_compass_yaw = averageYaw(initial_compass_yaw_buff);
	initial_point = averagePoint(initial_point_buff);
	target_yaw.data = 0.0;
	target_point.x = 0.0;
	target_point.y = 0.0;
	target_point.z = 0.0;
	
	ROS_DEBUG("SYSTEM> initial_compass_yaw:%f\n", initial_compass_yaw);
	ROS_DEBUG("SYSTEM> initial_point: x:%f y:%f z:%f\n"
			, initial_point.x
			, initial_point.y
			, initial_point.z
			);
			

	xp_pid.reset_clock();
    xv_pid.reset_clock();
    xa_pid.reset_clock();
	yp_pid.reset_clock();
    yv_pid.reset_clock();
    ya_pid.reset_clock();
	yaw_pid.reset_clock();
	alt_pid.reset_clock();
    altv_pid.reset_clock();
    alta_pid.reset_clock();
	ROS_INFO("YAMAKAZE> Complete Initilize.");
	ROS_INFO("YAMAKAZE> Good luck.");
}

void SipMavrosMem::shutdown(){
	ROS_INFO("YAMAKAZE> Shutdown Phase.");

    target_attitude.header.stamp = ros::Time::now();//ikeda
	double roll = 0.0;
	double pitch = 0.0;
	double yaw = compass_yaw;
	RollPitchYaw2Quat(target_attitude.pose.orientation, roll, pitch, yaw);
	ROS_DEBUG("SYSTEM> shutdown: roll:%f pitch:%f yaw:%f\n"
			, roll
			, pitch
			, yaw);
	target_throttle.data = 0.0;
//	attitude_pub.publish(target_attitude);
//	throttle_pub.publish(target_throttle);
    
    mavros_target.header.stamp = ros::Time::now();
    mavros_target.orientation = target_attitude.pose.orientation;
    mavros_target.thrust = target_throttle.data;
    target_pub.publish(mavros_target);

	rate.sleep();

	ROS_INFO("YAMAKAZE> Disarm Vehicle.");
	mavros_msgs::CommandBool disarm_cmd;
	disarm_cmd.request.value = false;
	ros::Time last_request = ros::Time::now();
	while(ros::ok()){
		if(ros::Time::now() - last_request > ros::Duration(5.0)){
			int call_success = arming_client.call(disarm_cmd);
			if(call_success){
				ROS_INFO("SYSTEM> Vehicle disarmed");
				break;
			}
		}
        target_attitude.header.stamp = ros::Time::now();//ikeda
//		attitude_pub.publish(target_attitude);
//		throttle_pub.publish(target_throttle);
        
        mavros_target.header.stamp = ros::Time::now();
        mavros_target.orientation = target_attitude.pose.orientation;
        mavros_target.thrust = target_throttle.data;
        target_pub.publish(mavros_target);
        
		rate.sleep();
	}
	ROS_INFO("YAMAKAZE> Complete Shutdown.");
	ROS_INFO("YAMAKAZE> Thank you for your hard work.");
}

void SipMavrosMem::updateErr(){
    slam_add_tm.update();

    ros::Time temp = ros::Time::now();

    slam_position.x += slam_point_velocity.x * slam_add_tm.diff;
    slam_position.y += slam_point_velocity.y * slam_add_tm.diff;
    slam_position.z += slam_point_velocity.z * slam_add_tm.diff;
    
	tf::Matrix3x3(
			compass_transform.getRotation()).getRPY(
			compass_roll, compass_pitch, compass_yaw);
	tf::Matrix3x3(
			current_transform.getRotation()).getRPY(
			current_roll, current_pitch, current_yaw);
	tf::Vector3 t_vec = current_transform.getOrigin();
	// !! Caution !!
	// The coordinate system of PixHawk IMU uses North-East-Down
	// The coordinate system of sip_mavros uses North-West-Up

    char mode = 0;
    
    /*Moved to below
    if(rc_accept and rc_land){target_point.z = -1.0;}*/

	double alt = t_vec.getZ() - initial_point.z;//いらない

    clock_t cl = clock();

    /* position and velocity directions are inertial coordinate and accele direction is body coordinate */
    xp_pid.push_err(target_point.x, slam_position.x, cl, mode);
    xv_pid.push_err(0, slam_point_velocity.x, cl);
    xa_pid.push_err(0, accel.x, cl);

    
	//  yp_pid.push_err(target_point.y + initial_point.y , y_lpf.filter(t_vec.getY()), cl);
    yp_pid.push_err(target_point.y, slam_position.y, cl);
    yv_pid.push_err(0, slam_point_velocity.y, cl);
    ya_pid.push_err(0, accel.y, cl);
    
    yaw_pid.push_err(0, slam_attitude_current.z, cl);
    
	fixed_alt = target_point.z;
	if(rc_accept){
        if(rc_land){
            landing_alt -= 0.002;
            if(landing_alt>0.0){
                landing_alt = 0.0;
            }else if(landing_alt<-1.0){
                landing_alt = -1.0;
            }
            fixed_alt = landing_alt;
        }else{
            landing_alt=0.0;
            fixed_alt *= rc_throttle_rate;
        }
	}

    alt_pid.push_err(fixed_alt, ground.current,cl);
    altv_pid.push_err(0.0, ground.velocity, cl);//ikeda
    alta_pid.push_err(0.0, accel.z ,cl);
    
	
	ROS_DEBUG("SYSTEM> initial_point: %f %f %f\n"
			, initial_point.x, initial_point.y, initial_point.z);
	ROS_DEBUG("SYSTEM> target_point: %f %f %f\n"
			, target_point.x, target_point.y, target_point.z);
	ROS_DEBUG("SYSTEM> z position err: P:%f I:%f D:%f PreD:%f\n"
			, alt_pid.current_err, alt_pid.integrated_err
			, alt_pid.differentiated_err
			, alt_pid.pre_differentiated_err);
}

void SipMavrosMem::publishCtrlInput(){
	target_attitude.header.stamp = ros::Time::now();
    
    target_attitude.pose.position.x = slam_position.x;
	target_attitude.pose.position.y = slam_position.y;
	target_attitude.pose.position.z = ground.current;

	// !! Caution !!
	// The coordinate system of PixHawk IMU uses North-East-Down
	// The coordinate system of sip_mavros uses North-West-Up
    
    /* position and velocity directions are inertial coordinate and accele direction is body coordinate */
    /* considering yaw rotation */
    double x_fb_tmp = xp_pid.pop_ctrl() + xv_pid.pop_ctrl();
    double y_fb_tmp = yp_pid.pop_ctrl() + yv_pid.pop_ctrl();
    double yaw_tmp = slam_attitude_current.z/*-current_yaw*//*compass_yaw -initial_compass_yaw*/;
    if(rc_direct_roll){
        y_fb_tmp = 0.0;
    }
    
	//	= (rc_direct_roll ? rc_roll : -1.0 * yp_pid.pop_ctrl() - yv_pid.pop_ctrl() - ya_pid.pop_ctrl()  );
    /*  considering yaw rotation */
	//	= (rc_direct_pitch ? rc_pitch :  xp_pid.pop_ctrl() + xv_pid.pop_ctrl() + xa_pid.pop_ctrl() );
    /*  considering yaw rotation */

    pitch_ctrl = (rc_direct_pitch ? rc_pitch :        ( x_fb_tmp * cos(yaw_tmp) - y_fb_tmp * sin(yaw_tmp) ) + xa_pid.pop_ctrl()  );
    roll_ctrl  = (rc_direct_roll  ? rc_roll  : -1.0 * ( y_fb_tmp * cos(yaw_tmp) + x_fb_tmp * sin(yaw_tmp) ) - ya_pid.pop_ctrl()  );

	yaw_ctrl = yaw_pid.pop_ctrl();

    ground.input         = alt_pid.pop_ctrl() + altv_pid.pop_ctrl() + alta_pid.pop_ctrl();
    target_throttle.data = ground.input + throttle_ave;

    
    //throttle_ave is average throttle during hover in stable altitude
    if(current_state.mode == "OFFBOARD"){
        printf("a\n");
        throttle_ave = 0.999 * throttle_ave + 0.001 * ( target_throttle.data - alt_pid.offset );//previous 0.998 vs 0.002
        if(throttle_ave > 0.85){
            throttle_ave = 0.85;
        }else if(throttle_ave < 0.3){
            throttle_ave = 0.3;
        }
    }
        
	if(rc_accept and (rc_override or rc_interrupt)){
		if(rc_override){ROS_WARN("YAMAKAZE> You have control.");}
		if(rc_interrupt){ROS_FATAL("YAMAKAZE> YOU HAVE CONTROL!!");}
		roll_ctrl = rc_roll;
		pitch_ctrl = rc_pitch;
        yaw_ctrl = rc_yaw;
	}

    
	RollPitchYaw2Quat(target_attitude.pose.orientation
			, roll_ctrl 
			, pitch_ctrl
			, /*compass_yaw*/ body_attitude.z + yaw_ctrl);

	if(target_throttle.data > 1.0){target_throttle.data = 1.0;}
	if(target_throttle.data < 0.0){target_throttle.data = 0.0;}
	
	ROS_DEBUG("SYSTEM> target throttle:%f\n"
			, target_throttle.data);
	ROS_DEBUG("SYSTEM> target att roll:%f, pitch:%f, yaw:%f\n"
			, roll_ctrl
			, pitch_ctrl
			, initial_compass_yaw + yaw_ctrl);

//	attitude_pub.publish(target_attitude);
//	throttle_pub.publish(target_throttle);
    
    mavros_target.header.stamp = ros::Time::now();
    mavros_target.orientation = target_attitude.pose.orientation;
    mavros_target.thrust = target_throttle.data;
    target_pub.publish(mavros_target);

}

double SipMavrosMem::averageYaw(const std::vector<double> &yaws){
	double ave = 0.0;
	size_t n_yaw = 0;
	for(auto yaw= yaws.begin(); yaw != yaws.end(); ++yaw){
		if(std::isnan(*yaw)){continue;}
		ave += *yaw;
		++n_yaw;
	}
	ave /= (double)(n_yaw);

	return ave;
}

geometry_msgs::Point SipMavrosMem::averagePoint( const std::vector<tf::Vector3> &points ){
	double x = 0.0;
	double y = 0.0;
	double z = 0.0;
	size_t n_data = 0;
	for(auto p = points.begin(); p != points.end(); ++p){
		if(std::isnan(p->getX()) or std::isnan(p->getY()) or std::isnan(p->getZ())){continue;}
		x += p->getX();
		y += p->getY();
		z += p->getZ();
		++n_data;
	}
	x /= (double)(n_data);
	y /= (double)(n_data);
	z /= (double)(n_data);
	geometry_msgs::Point ave;
	ave.x = x;
	ave.y = y;
	ave.z = z;
	return ave;
}

void readme(char *name){
	ROS_INFO("SYSTEM>\n\
			Usage: %s \n\
			-t <target_point_topic [geometry_msgs::Point in]>\n\
			-y <target_yaw_topic [std_msgs::Float64 in]> \n\
			-f <parameter xml file> \n\
			\n\
			Ex: %s \n\
			-t /sip/target_point\n\
			-y /sip/target_yaw\n\
			-f param_RW_quad.xml\n\
			\n", name, name);
}

void cmd_arguments(
		int argc
		, char **argv
		, std::string &target_point_topic
		, std::string &target_yaw_topic
		, std::string &param_xml
		){
    int result;
	while((result = getopt(argc, argv, "ht:y:f:")) != -1){
		switch(result){
			case 'h':
				readme(argv[0]);
				exit(0);
				break;
			case 't':
				target_point_topic = std::string(optarg);
				break;
			case 'y':
				target_yaw_topic = std::string(optarg);
				break;
			case 'f':
				param_xml = std::string(optarg);
				break;
		}
	}
}

bool loadParamXML(const std::string &xml_path
		, std::vector<PID> &params
		, double &battery_coefficient
		, double &ground_effect_coefficient
		, bool &rc_accept
		, bool &rc_direct_roll
		, bool &rc_direct_pitch
		, double sample_freq
		){
	params.clear();
	params.resize(10);
	TiXmlDocument xml(xml_path.c_str());
	bool success = xml.LoadFile();
	if(not success){return 1;}
	TiXmlElement *data = xml.FirstChildElement("param");

	TiXmlElement *machine_param = data->FirstChildElement("machine");
	battery_coefficient
		= atof(machine_param->FirstChildElement(
					"battery_coefficient")->GetText());
	ground_effect_coefficient
		= atof(machine_param->FirstChildElement(
					"ground_effect_coefficient")->GetText());
	rc_accept
		= (strncmp(machine_param->FirstChildElement(
					"rc_accept")->GetText(), "true", 4) == 0);
	rc_direct_roll
		= (strncmp(machine_param->FirstChildElement(
					"rc_direct_roll")->GetText(), "true", 4) == 0);
	rc_direct_pitch
		= (strncmp(machine_param->FirstChildElement(
					"rc_direct_pitch")->GetText(), "true", 4) == 0);

	TiXmlElement *pid_param = data->FirstChildElement("pid");
	TiXmlElement *elm[3], *yaw_elm;
	elm[0] = pid_param->FirstChildElement("x");
	elm[1] = pid_param->FirstChildElement("y");
	elm[2] = pid_param->FirstChildElement("z");
	yaw_elm = pid_param->FirstChildElement("yaw");

	for(size_t i = 0; i < 3; ++i){
		TiXmlElement *position, *velocity, *accel;
		position = elm[i]->FirstChildElement("position");
		velocity = elm[i]->FirstChildElement("velocity");
		accel = elm[i]->FirstChildElement("accel");
        
		params.at(i * 3)
			= PID(
					atof(position->FirstChildElement("p_gain")->GetText())
					, atof(position->FirstChildElement("i_gain")->GetText())
					, atof(position->FirstChildElement("d_gain")->GetText())
					, atof(position->FirstChildElement("pre_d_gain")->GetText())
					, atof(position->FirstChildElement("upper_limit")->GetText())
					, atof(position->FirstChildElement("lower_limit")->GetText())
					, atof(position->FirstChildElement("offset")->GetText())
					, atof(position->FirstChildElement("cutoff_freq")->GetText())
					, sample_freq
					, atof(position->FirstChildElement("q_param")->GetText())
				 );

		params.at(i * 3 + 1)
			= PID(
					atof(velocity->FirstChildElement("p_gain")->GetText())
					, atof(velocity->FirstChildElement("i_gain")->GetText())
					, atof(velocity->FirstChildElement("d_gain")->GetText())
					, atof(velocity->FirstChildElement("pre_d_gain")->GetText())
					, atof(velocity->FirstChildElement("upper_limit")->GetText())
					, atof(velocity->FirstChildElement("lower_limit")->GetText())
					, atof(velocity->FirstChildElement("offset")->GetText())
					, atof(velocity->FirstChildElement("cutoff_freq")->GetText())
					, sample_freq
					, atof(velocity->FirstChildElement("q_param")->GetText())
				 );

		params.at(i * 3 + 2)
			= PID(
					atof(accel->FirstChildElement("p_gain")->GetText())
					, atof(accel->FirstChildElement("i_gain")->GetText())
					, atof(accel->FirstChildElement("d_gain")->GetText())
					, atof(accel->FirstChildElement("pre_d_gain")->GetText())
					, atof(accel->FirstChildElement("upper_limit")->GetText())
					, atof(accel->FirstChildElement("lower_limit")->GetText())
					, atof(accel->FirstChildElement("offset")->GetText())
					, atof(accel->FirstChildElement("cutoff_freq")->GetText())
					, sample_freq
					, atof(accel->FirstChildElement("q_param")->GetText())
				 );
	}

	params.at(9)
		= PID(
				atof(yaw_elm->FirstChildElement("p_gain")->GetText())
				, atof(yaw_elm->FirstChildElement("i_gain")->GetText())
				, atof(yaw_elm->FirstChildElement("d_gain")->GetText())
				, atof(yaw_elm->FirstChildElement("pre_d_gain")->GetText())
				, atof(yaw_elm->FirstChildElement("upper_limit")->GetText())
				, atof(yaw_elm->FirstChildElement("lower_limit")->GetText())
				, atof(yaw_elm->FirstChildElement("offset")->GetText())
				, atof(yaw_elm->FirstChildElement("cutoff_freq")->GetText())
				, sample_freq
				, atof(yaw_elm->FirstChildElement("q_param")->GetText())
			 );
	return 0;
}

int main(int argc, char **argv){
	std::string target_point_topic("/sip/target_point");
	std::string target_yaw_topic("/sip/target_yaw");
	std::string param_xml("param_hexa.xml");
	cmd_arguments(argc, argv
			, target_point_topic
			, target_yaw_topic
			, param_xml
			);

	ros::init(argc, argv, "sip_mavros_node");
    /*multi thread spinner*/
    ros::AsyncSpinner spinner(12);
    spinner.start();
    
    FILE *fp;
    if((fp=fopen("output.txt","w"))==NULL){
        perror("openfile");
        exit(1);
    }
    
	double control_freq = 200.0;//frequency
	std::vector<PID> pid_params;
	double battery_coefficient, ground_effect_coefficient;
	bool rc_accept, rc_direct_roll, rc_direct_pitch;
	bool error = loadParamXML(param_xml, pid_params
			, battery_coefficient, ground_effect_coefficient
			, rc_accept, rc_direct_roll, rc_direct_pitch
			, control_freq);
	if(error){
		ROS_FATAL("YAMAKAZE> ERROR!! Cannot open Parameter File: %s."
				, param_xml.c_str());
		ROS_FATAL("YAMAKAZE> This system is finished automatically.");
		exit(1);
	}

	SipMavrosMem smm(
			target_point_topic
			, target_yaw_topic
			, pid_params.at(0) // x
            , pid_params.at(1)
            , pid_params.at(2)
			, pid_params.at(3) // y
            , pid_params.at(4)
            , pid_params.at(5)
			, pid_params.at(6) // alt
            , pid_params.at(7)
            , pid_params.at(8)
			, pid_params.at(9) // yaw
			, battery_coefficient
			, ground_effect_coefficient
			, rc_accept
			, rc_direct_roll
			, rc_direct_pitch
			, control_freq
			);

    ros::Time rostime_getter;
    double tm_init = 0.0;
    double tm_crr = 0.0;
    
	smm.starting();

	ROS_INFO("YAMAKAZE> Start Main Phase.");
    rostime_getter = ros::Time::now();
    tm_init = (double)(rostime_getter.sec)+(double)(rostime_getter.nsec)/1000000000.0;
    
    fprintf(fp,"xpGp=%lf~xpGi=%lf~xpGd=%lf~",smm.xp_pid.p_gain,smm.xp_pid.i_gain,smm.xp_pid.d_gain);
    fprintf(fp,"xvGp=%lf~xvGi=%lf~xvGd=%lf~",smm.xv_pid.p_gain,smm.xv_pid.i_gain,smm.xv_pid.d_gain);
    fprintf(fp,"xaGp=%lf~xaGi=%lf~xaGd=%lf~",smm.xa_pid.p_gain,smm.xa_pid.i_gain,smm.xa_pid.d_gain);
    fprintf(fp,"\t");
    //
    fprintf(fp,"ypGp=%lf~ypGi=%lf~ypGd=%lf~",smm.yp_pid.p_gain,smm.yp_pid.i_gain,smm.yp_pid.d_gain);
    fprintf(fp,"yvGp=%lf~yvGi=%lf~yvGd=%lf~",smm.yv_pid.p_gain,smm.yv_pid.i_gain,smm.yv_pid.d_gain);
    fprintf(fp,"yaGp=%lf~yaGi=%lf~yaGd=%lf~",smm.ya_pid.p_gain,smm.ya_pid.i_gain,smm.ya_pid.d_gain);
    fprintf(fp,"\t");
    //
    fprintf(fp,"zpGp=%lf~zpGi=%lf~zpGd=%lf~",smm.alt_pid.p_gain,smm.alt_pid.i_gain,smm.alt_pid.d_gain);
    fprintf(fp,"zvGp=%lf~zvGi=%lf~zvGd=%lf~",smm.altv_pid.p_gain,smm.altv_pid.i_gain,smm.altv_pid.d_gain);
    fprintf(fp,"zaGp=%lf~zaGi=%lf~zaGd=%lf~",smm.alta_pid.p_gain,smm.alta_pid.i_gain,smm.alta_pid.d_gain);
    fprintf(fp,"\n");
    
    fprintf(fp,"crrTime\t");
    fprintf(fp,"desX\tdesY\tdesZ\tdesNose\t");
    fprintf(fp,"slamX\tslamY\tlaserZ\tslamNose\t");
    fprintf(fp,"desRoll\tdesPitch\tdesYaw\tdesThrottle\t");
    fprintf(fp,"imuRoll\timuPitch\timuYaw\taveThrottle\t");
    fprintf(fp,"tfYaw\t");
    fprintf(fp,"battery\t");
    
    fprintf(fp,"\n");
	while(ros::ok() && smm.ok() && not smm.rc_exit){
        rostime_getter = ros::Time::now();
        tm_crr = ((double)(rostime_getter.sec)+(double)(rostime_getter.nsec)/1000000000.0) - tm_init;

		smm.updateTransform();
		smm.updateErr();
		smm.publishCtrlInput();
        /*time             1*/
        fprintf(fp,"%lf\t",
                tm_crr);
        fprintf(fp,"%lf\t%lf\t%lf\t%lf\t",//desired position and direction
                smm.target_point.x,//2
                smm.target_point.y,//3
                smm.target_point.z,//4
                smm.target_yaw.data);//5
        fprintf(fp,"%lf\t%lf\t%lf\t%lf\t",//current position and direction
                smm.slam_position.x,//6
                smm.slam_position.y,//7
                smm.ground.current,//8
                smm.slam_attitude_current.z);//9
        fprintf(fp,"%lf\t%lf\t%lf\t%lf\t",//desired attitude and throttle
                smm.roll_ctrl,//10
                smm.pitch_ctrl,//11
                smm.yaw_ctrl,//12
                (double)smm.target_throttle.data);//13
        fprintf(fp,"%lf\t%lf\t%lf\t%lf\t",
                smm.body_attitude.x,//14
                smm.body_attitude.y,//15
                smm.body_attitude.z,//16
                smm.throttle_ave);//17
        fprintf(fp,"%lf\t",
                smm.compass_yaw);//18
        fprintf(fp,"%f\t",
                smm.battery_percent);//19
        
        

        printf("\n");
        fprintf(fp,"\n");
		smm.rate.sleep();
	}
    fclose(fp);
	smm.shutdown();

	return 0;
}
