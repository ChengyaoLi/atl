#include "atl/gazebo/bridge/world_node.hpp"

namespace atl {
namespace gazebo_bridge {

int WorldNode::configure(const int hz) {
  // setup ros node
  // clang-format off
  ROSNode::configure(hz);
  ROSNode::addPublisher<rosgraph_msgs::Clock>(CLOCK_RTOPIC);
  ROSNode::addSubscriber(SHUTDOWN_RTOPIC, &WorldNode::shutdownCallback, this);
  ROSNode::addSubscriber(PAUSE_RTOPIC, &WorldNode::pauseCallback, this);
  ROSNode::addSubscriber(UNPAUSE_RTOPIC, &WorldNode::unPauseCallback, this);
  ROSNode::addSubscriber(RESET_RTOPIC, &WorldNode::resetCallback, this);
  ROSNode::addSubscriber(MODEL_POSE_RTOPIC, &WorldNode::modelPoseCallback, this);
  ROSNode::addSubscriber(LOAD_WORLD_RTOPIC, &WorldNode::loadWorldCallback, this);
  ROSNode::addSubscriber(CLEAR_WORLD_RTOPIC, &WorldNode::clearWorldCallback, this);
  // clang-format on

  // setup gazebo client
  if (WorldGClient::configure() != 0) {
    ROS_ERROR("Failed to configure WorldGClient!");
    return -1;
  }

  return 0;
}

void WorldNode::clockCallback(ConstTimePtr &gaz_msg) {
  gazebo::msgs::Time sim_time;
  rosgraph_msgs::Clock ros_msg;

  // parse gazebo time message
  WorldGClient::clockCallback(gaz_msg);

  // publish simulation time
  sim_time = this->time.sim_time;
  ros_msg.clock =
      ::ros::Time((uint32_t) sim_time.sec(), (uint32_t) sim_time.nsec());
  this->ros_pubs[CLOCK_RTOPIC].publish(ros_msg);
}

void WorldNode::shutdownCallback(const std_msgs::Bool &msg) {
  if (msg.data) {
    ROS_INFO("Shutting Down Gazebo World!");
    WorldGClient::shutdownServer();
    system("rosnode kill -a");
  }
}

void WorldNode::pauseCallback(const std_msgs::Bool &msg) {
  if (msg.data) {
    ROS_INFO("Pause Gazebo World!");
    WorldGClient::pauseWorld();
  }
}

void WorldNode::unPauseCallback(const std_msgs::Bool &msg) {
  if (msg.data) {
    ROS_INFO("Un-Pause Gazebo World!");
    WorldGClient::unPauseWorld();
  }
}

void WorldNode::resetCallback(const std_msgs::Bool &msg) {
  if (msg.data) {
    ROS_INFO("Reset Gazebo World!");
    WorldGClient::resetWorld();
  }
}

void WorldNode::modelPoseCallback(const atl_msgs::ModelPose &msg) {
  std::string model_name = msg.model_name.data;
  Vec3 pos{msg.position.x, msg.position.y, msg.position.z};
  Vec3 rpy{msg.orientation.x, msg.orientation.y, msg.orientation.z};
  WorldGClient::setModelPose(model_name, pos, rpy);
}

void WorldNode::loadWorldCallback(const std_msgs::String &msg) {
  ROS_INFO("Load Gazebo World!");
  if (WorldGClient::loadWorld(msg.data) != 0) {
    ROS_ERROR("Failed to load [%s]!", msg.data.c_str());
  }
}

void WorldNode::clearWorldCallback(const std_msgs::Bool &msg) {
  if (msg.data) {
    ROS_INFO("Clear Gazebo World!");
    WorldGClient::clearWorld();
  }
}

} // namespace gazebo_bridge
} // namespace atl

RUN_ROS_NODE(atl::gazebo_bridge::WorldNode, NODE_RATE);
