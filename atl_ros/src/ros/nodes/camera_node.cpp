#include "atl/ros/nodes/camera_node.hpp"

namespace atl {

int CameraNode::configure(const int hz) {
  std::string config_path;

  // ros node
  if (ROSNode::configure(hz) != 0) {
    return -1;
  }

  // camera
  ROS_GET_PARAM(this->node_name + "/config_dir", config_path);
  if (this->camera.configure(config_path) != 0) {
    ROS_ERROR("Failed to configure Camera!");
    return -2;
  };
  this->camera.initialize();

  // register publisher and subscribers
  // clang-format off
  this->addImagePublisher(CAMERA_IMAGE_TOPIC);
  // this->addSubscriber(GIMBAL_POSITION_TOPIC, &CameraNode::gimbalPositionCallback, this);
  // this->addSubscriber(GIMBAL_FRAME_ORIENTATION_TOPIC, &CameraNode::gimbalFrameCallback, this);
  // this->addSubscriber(GIMBAL_JOINT_ORIENTATION_TOPIC, &CameraNode::gimbalJointCallback, this);
  // this->addSubscriber(APRILTAG_TOPIC, &CameraNode::aprilTagCallback, this);
  // clang-format on
  this->addShutdownListener(SHUTDOWN_TOPIC);

  // register loop callback
  this->addLoopCallback(std::bind(&CameraNode::loopCallback, this));

  this->configured = true;
  return 0;
}

int CameraNode::publishImage() {
  sensor_msgs::ImageConstPtr img_msg;

  // encode position and orientation into image (first 11 pixels in first row)
  // if (this->gimbal_mode) {
  this->image.at<double>(0, 0) = this->gimbal_position(0);
  this->image.at<double>(0, 1) = this->gimbal_position(1);
  this->image.at<double>(0, 2) = this->gimbal_position(2);

  this->image.at<double>(0, 3) = this->gimbal_frame_orientation.w();
  this->image.at<double>(0, 4) = this->gimbal_frame_orientation.x();
  this->image.at<double>(0, 5) = this->gimbal_frame_orientation.y();
  this->image.at<double>(0, 6) = this->gimbal_frame_orientation.z();

  this->image.at<double>(0, 7) = this->gimbal_joint_orientation.w();
  this->image.at<double>(0, 8) = this->gimbal_joint_orientation.x();
  this->image.at<double>(0, 9) = this->gimbal_joint_orientation.y();
  this->image.at<double>(0, 10) = this->gimbal_joint_orientation.z();
  // }

  // clang-format off
  cv::Mat gray_image;
  cv::cvtColor(this->image, gray_image, CV_BGR2GRAY);

  std_msgs::Header header;
  header.seq = this->ros_seq;
  header.stamp = ros::Time::now();
  header.frame_id = this->node_name;

  img_msg = cv_bridge::CvImage(
    header,
    // "bgr8",
    // this->image
    "mono8",
    gray_image
  ).toImageMsg();
  this->img_pubs[CAMERA_IMAGE_TOPIC].publish(img_msg);
  // clang-format on

  return 0;
}

void CameraNode::gimbalPositionCallback(const geometry_msgs::Vector3 &msg) {
  convertMsg(msg, this->gimbal_position);
}

void CameraNode::gimbalFrameCallback(const geometry_msgs::Quaternion &msg) {
  convertMsg(msg, this->gimbal_frame_orientation);
}

void CameraNode::gimbalJointCallback(const geometry_msgs::Quaternion &msg) {
  convertMsg(msg, this->gimbal_joint_orientation);
}

void CameraNode::aprilTagCallback(const atl_msgs::AprilTagPose &msg) {
  convertMsg(msg, this->tag);
}

int CameraNode::loopCallback() {
  double dist;

  // change mode depending on apriltag distance
  if (this->tag.detected == false) {
    this->camera.changeMode("640x640");

  } else {
    dist = this->tag.position(2);
    if (dist > 8.0) {
      this->camera.changeMode("640x640");
    } else if (dist > 4.0) {
      this->camera.changeMode("320x320");
    } else {
      this->camera.changeMode("160x160");
    }
  }

  this->camera.showImage(this->image);
  this->camera.getFrame(this->image);
  this->publishImage();

  return 0;
}

} // namespace atl

RUN_ROS_NODE(atl::CameraNode, NODE_RATE);
