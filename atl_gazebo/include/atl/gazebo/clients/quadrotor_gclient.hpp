#ifndef atl_GAZEBO_QUADROTOR_CLIENT_HPP
#define atl_GAZEBO_QUADROTOR_CLIENT_HPP

#include <string>
#include <vector>

#include <Eigen/Dense>

#include <gazebo/gui/GuiEvents.hh>

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include "atl/gazebo/gazebo_node.hpp"
#include "atl/utils/utils.hpp"

namespace atl {
namespace gaz {

// MESSAGES TYPES
#define POSE_MSG gazebo::msgs::Pose
#define ATT_SETPOINT_MSG atl_msgs::msgs::AttitudeSetpoint
#define POS_SETPOINT_MSG atl_msgs::msgs::PositionSetpoint
#define VEL_SETPOINT_MSG atl_msgs::msgs::VelocitySetpoint

// PUBLISH TOPICS
#define IMAGE_GTOPIC "~/gimbal_cam/image"
#define POSE_GTOPIC "~/quadrotor/pose"
#define VELOCITY_GTOPIC "~/quadrotor/velocity"

// SUBSCRIBE TOPICS
#define ATT_SETPOINT_GTOPIC "~/quadrotor/setpoint/attitude"
#define POS_SETPOINT_GTOPIC "~/quadrotor/setpoint/position"
#define VEL_SETPOINT_GTOPIC "~/quadrotor/setpoint/velocity"

class QuadrotorGClient : public GazeboNode {
public:
  bool connected;

  Vec3 position;
  Quaternion orientation;
  Vec3 velocity;
  Vec4 attitude_setpoints;
  Vec3 position_setpoints;
  Vec3 velocity_setpoints;

  QuadrotorGClient()
      : position{Vec3::Zero()}, orientation{1.0, 0.0, 0.0, 0.0},
        attitude_setpoints{0.0, 0.0, 0.0, 0.0},
        position_setpoints{0.0, 0.0, 0.0}, velocity_setpoints{0.0, 0.0, 0.0} {}

  ~QuadrotorGClient() {
    if (this->connected) {
      gazebo::client::shutdown();
    }
  }

  int configure();
  virtual void poseCallback(ConstPosePtr &msg);
  virtual void velocityCallback(ConstVector3dPtr &msg);
  int setAttitude(double r, double p, double y, double t);
  int setPosition(double x, double y, double z);
  int setVelocity(double vx, double vy, double vz);
};

} // namespace gaz
} // namespace atl
#endif
