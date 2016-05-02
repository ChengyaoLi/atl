#ifndef __QUADROTOR_HPP__
#define __QUADROTOR_HPP__

#include <iostream>
#include <cmath>

#include <unistd.h>

#include <ros/ros.h>
#include <tf/transform_datatypes.h>

#include <std_msgs/Float64.h>
#include <sensor_msgs/Imu.h>
#include <geometry_msgs/PoseStamped.h>

#define MAVLINK_DIALECT common
#include <mavros/mavros.h>
#include <mavros_msgs/State.h>
#include <mavros_msgs/SetMode.h>
#include <mavros_msgs/CommandBool.h>

#include "awesomo/pid.hpp"
#include "awesomo/util.hpp"
#include "awesomo/camera.hpp"
#include "awesomo/controller.hpp"


// CONSTANTS
#define IMU_TOPIC "/mavros/imu/data"
#define ARM_TOPIC "/mavros/cmd/arming"
#define MOCAP_TOPIC "/awesomo/mocap/pose"
#define MODE_TOPIC "/mavros/set_mode"
#define POSE_TOPIC "/mavros/local_position/pose"
#define POSITION_TOPIC "/mavros/setpoint_position/local"
#define ATTITUDE_TOPIC "/mavros/setpoint_attitude/attitude"
#define THROTTLE_TOPIC "/mavros/setpoint_attitude/att_throttle"
#define POSITION_X_CONTROLLER_TOPIC "/awesomo/position_controller/x"
#define POSITION_Y_CONTROLLER_TOPIC "/awesomo/position_controller/y"
#define POSITION_Z_CONTROLLER_TOPIC "/awesomo/position_controller/z"

#define HOVER_MODE 0
#define DISCOVER_MODE 1
#define PLANNING_MODE 2
#define CARROT_MODE 3
#define MISSION_ACCOMPLISHED 4

class Pose
{
    public:
        double x;
        double y;
        double z;

        double roll;
        double pitch;
        double yaw;
};

class Quadrotor
{
    private:
        int mission_state;
        mavros_msgs::State state;
        ros::NodeHandle node;

        ros::Subscriber mocap_subscriber;
        ros::Subscriber pose_subscriber;
        ros::Subscriber imu_subscriber;

        ros::ServiceClient mode_client;
        ros::ServiceClient arming_client;

        Camera *cam;
        int tag_timeout;
        std::vector<TagPose> tag_poses;
        Eigen::Vector3d tag_position;

        void poseCallback(const geometry_msgs::PoseStamped &msg);
        void subscribeToPose(void);
        void mocapCallback(const geometry_msgs::PoseStamped &msg);
        void subscribeToMocap(void);
        void imuCallback(const sensor_msgs::Imu::ConstPtr &msg);
        void subscribeToIMU(void);
        void stateCallback(const mavros_msgs::State::ConstPtr &msg);
        void waitForConnection(void);

    public:
        Pose pose;
        Pose mocap_pose;
        CarrotController *carrot_controller;
        PositionController *position_controller;

        ros::Publisher position_publisher;
        ros::Publisher attitude_publisher;
        ros::Publisher throttle_publisher;
        ros::Publisher position_controller_x_publisher;
        ros::Publisher position_controller_y_publisher;
        ros::Publisher position_controller_z_publisher;

        Quadrotor(void);
        int arm(void);
        int disarm(void);
        int setOffboardModeOn(void);
        void positionControllerCalculate(float x, float y, float z, ros::Time last_request);
        void printPositionController(void);
        void buildPositionMessage(
            geometry_msgs::PoseStamped &msg,
            int seq,
            ros::Time time,
            float x,
            float y,
            float z
        );
        void buildAtitudeMessage(
            geometry_msgs::PoseStamped &msg,
            int seq,
            ros::Time time
        );
        void buildThrottleMessage(std_msgs::Float64 &msg);
        void buildPositionControllerMessage(
            geometry_msgs::PoseStamped &msg,
            int seq,
            ros::Time time
        );
        void publishPositionControllerStats(int seq, ros::Time time);
};

#endif