#include "atl/ros/nodes/example_node.hpp"

namespace atl {

int ExampleNode::configure(const int hz) {
  ROSNode::configure(hz);

  // clang-format off
  this->addPublisher<std_msgs::String>(SAY_TOPIC);
  this->addSubscriber(SAY_TOPIC, &ExampleNode::sayCallback, this);
  this->addLoopCallback(std::bind(&ExampleNode::loopCallback, this));
  // clang-format on

  return 0;
}

int ExampleNode::loopCallback() {
  std_msgs::String msg;
  msg.data = "Hello";
  this->ros_pubs[SAY_TOPIC].publish(msg);
  return 0;
}

void ExampleNode::sayCallback(const std_msgs::String &msg) {
  std::cout << msg.data << std::endl;
}

} // namespace atl

RUN_ROS_NODE(atl::ExampleNode, NODE_RATE);
