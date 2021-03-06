#ifndef atl_GAZEBO_GIMBAL_CLIENT_HPP
#define atl_GAZEBO_GIMBAL_CLIENT_HPP

#include <string>
#include <vector>

#include <Eigen/Dense>

#include "atl/gazebo/gazebo_node.hpp"
#include "atl/utils/math.hpp"

namespace atl {
namespace gaz {

// MESSAGE TYPES
#define VEC3_MSG gazebo::msgs::Vector3d

// PUBLISH TOPICS
#define SETPOINT_GTOPIC "~/gimbal/joint/setpoint"
#define TRACK_GTOPIC "~/gimbal/target/track"

// SUBSCRIBE TOPICS
#define FRAME_ORIENTATION_GTOPIC "~/gimbal/frame/orientation/inertial"
#define JOINT_ORIENTATION_GTOPIC "~/gimbal/joint/orientation/inertial"

class GimbalGClient : public GazeboNode {
public:
  bool connected;
  Quaternion frame_orientation;
  Quaternion joint_orientation;

  GimbalGClient();
  ~GimbalGClient();
  int configure();
  void setAttitude(Vec3 euler_W);
  void trackTarget(Vec3 target_C);
  virtual void frameOrientationCallback(ConstQuaternionPtr &msg);
  virtual void jointOrientationCallback(ConstQuaternionPtr &msg);
};

} // namespace gaz
} // namespace atl
#endif
