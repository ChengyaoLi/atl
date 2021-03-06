#include "atl/gazebo/bridge/px4_quadrotor_node.hpp"

namespace atl {
namespace gazebo_bridge {

int PX4QuadrotorNode::configure(const int hz) {
  this->quad_frame = "NWU";

  // setup ros node
  // clang-format off
  ROSNode::configure(hz);
  ROSNode::addPublisher<geometry_msgs::PoseStamped>(PX4_POSE_RTOPIC);
  ROSNode::addPublisher<geometry_msgs::TwistStamped>(PX4_VELOCITY_RTOPIC);
  ROSNode::addSubscriber(PX4_ATTITUDE_SETPOINT_RTOPIC, &PX4QuadrotorNode::attitudeSetpointCallback, this, 1);
  ROSNode::addSubscriber(PX4_THROTTLE_SETPOINT_RTOPIC, &PX4QuadrotorNode::throttleSetpointCallback, this, 1);
  ROSNode::addSubscriber(PX4_POSITION_SETPOINT_RTOPIC, &PX4QuadrotorNode::positionSetpointCallback, this, 1);
  ROSNode::addSubscriber(PX4_VELOCITY_SETPOINT_RTOPIC, &PX4QuadrotorNode::velocitySetpointCallback, this, 1);
  // clang-format on

  // setup gazebo client
  if (QuadrotorGClient::configure() != 0) {
    ROS_ERROR("Failed to configure QuadrotorGClient!");
    return -1;
  }

  return 0;
}

void PX4QuadrotorNode::poseCallback(const RPYPosePtr &msg) {
  geometry_msgs::PoseStamped pose_msg;
  Vec3 euler;
  Vec3 gaz_pos, ros_pos, pos;
  Quaternion gaz_quat;

  // gazebo pose callback
  QuadrotorGClient::poseCallback(msg);

  // obtain position
  gaz_pos(0) = this->pose(0);
  gaz_pos(1) = this->pose(1);
  gaz_pos(2) = this->pose(2);

  if (this->quad_frame == "NWU") {
    // do not transform
    pos(0) = gaz_pos(0);
    pos(1) = gaz_pos(1);
    pos(2) = gaz_pos(2);

    // convert euler angles to quaternion
    euler << msg->roll(), msg->pitch(), msg->yaw();
    euler2quat(euler, 321, gaz_quat);

  } else if (this->quad_frame == "NED") {
    // transform from NWU to NED
    pos(0) = gaz_pos(0);
    pos(1) = -gaz_pos(1);
    pos(2) = -gaz_pos(2);

    // convert euler angles to quaternion
    euler << msg->roll(), -msg->pitch(), -msg->yaw();
    euler2quat(euler, 321, gaz_quat);
  }

  // build pose msg
  pose_msg.header.seq = this->ros_seq;
  pose_msg.header.stamp = ::ros::Time::now();

  pose_msg.pose.position.x = pos(0);
  pose_msg.pose.position.y = pos(1);
  pose_msg.pose.position.z = pos(2);

  pose_msg.pose.orientation.w = gaz_quat.w();
  pose_msg.pose.orientation.x = gaz_quat.x();
  pose_msg.pose.orientation.y = gaz_quat.y();
  pose_msg.pose.orientation.z = gaz_quat.z();

  this->ros_pubs[PX4_POSE_RTOPIC].publish(pose_msg);
}

void PX4QuadrotorNode::velocityCallback(const ConstVector3dPtr &msg) {
  // gazebo velocity callback
  QuadrotorGClient::velocityCallback(msg);

  // build ros msg
  geometry_msgs::TwistStamped ros_msg;

  if (this->quad_frame == "NWU") {
    ros_msg.twist.linear.x = this->velocity(0);
    ros_msg.twist.linear.y = this->velocity(1);
    ros_msg.twist.linear.z = this->velocity(2);
  } else if (this->quad_frame == "NED") {
    ros_msg.twist.linear.x = this->velocity(0);
    ros_msg.twist.linear.y = -this->velocity(1);
    ros_msg.twist.linear.z = -this->velocity(2);
  }

  this->ros_pubs[PX4_VELOCITY_RTOPIC].publish(ros_msg);
}

void PX4QuadrotorNode::attitudeSetpointCallback(
    const geometry_msgs::PoseStamped &msg) {
  Vec3 euler;
  Quaternion q;
  double throttle;

  // convert quaternions to euler
  q.w() = msg.pose.orientation.w;
  q.x() = msg.pose.orientation.x;
  q.y() = msg.pose.orientation.y;
  q.z() = msg.pose.orientation.z;
  quatToEuler(q, 321, euler);

  // obtain throttle from message
  throttle = this->attitude_setpoints(3);

  // set attitude
  this->setAttitude(euler(0), euler(1), euler(2), throttle);
}

void PX4QuadrotorNode::throttleSetpointCallback(const std_msgs::Float64 &msg) {
  double roll, pitch, yaw, throttle;

  roll = this->attitude_setpoints(0);
  pitch = this->attitude_setpoints(1);
  yaw = this->attitude_setpoints(2);
  throttle = msg.data;

  this->setAttitude(roll, pitch, yaw, throttle);
}

void PX4QuadrotorNode::positionSetpointCallback(
    const geometry_msgs::PoseStamped &msg) {
  Vec3 ros_pos, gaz_pos;

  // transform ros position to gazebo position
  ros_pos(0) = msg.pose.position.x;
  ros_pos(1) = msg.pose.position.y;
  ros_pos(2) = msg.pose.position.z;
  ros2gaz(ros_pos, gaz_pos);

  this->setPosition(gaz_pos(0), gaz_pos(1), gaz_pos(2));
}

void PX4QuadrotorNode::velocitySetpointCallback(
    const geometry_msgs::TwistStamped &msg) {
  Vec3 ros_vel, gaz_vel;

  // transform ros position to gazebo velocity
  ros_vel(0) = msg.twist.linear.x;
  ros_vel(1) = msg.twist.linear.y;
  ros_vel(2) = msg.twist.linear.z;
  ros2gaz(ros_vel, gaz_vel);

  this->setVelocity(gaz_vel(0), gaz_vel(1), gaz_vel(2));
}

} // namespace gazebo_bridge
} // namespace atl

RUN_ROS_NODE(atl::gazebo_bridge::PX4QuadrotorNode, NODE_RATE);
