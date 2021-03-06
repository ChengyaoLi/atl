#include "atl/gazebo/clients/df_camera_gclient.hpp"

namespace atl {
namespace gaz {

DFCameraGClient::DFCameraGClient() { this->connected = false; }

DFCameraGClient::~DFCameraGClient() {
  if (this->connected) {
    gazebo::client::shutdown();
  }
}

int DFCameraGClient::configure() {
  // pre-check
  this->connected = gazebo::client::setup(0, NULL);
  if (this->connected == false) {
    return -1;
  }

  // setup gazebo node
  GazeboNode::configure();
  this->addSubscriber(IMAGE_TOPIC, &DFCameraGClient::imageCallback, this);
  this->waitForConnection();
  return 0;
}

void DFCameraGClient::imageCallback(ConstImagePtr &msg) {
  int buf_size;
  unsigned char *img_data;

  // convert img message to cv::Mat
  buf_size = (int) msg->width() * (int) msg->height() * 3;
  img_data = new unsigned char[buf_size + 1];
  memcpy((char *) img_data, msg->data().c_str(), buf_size);

  // clang-format off
  this->image = cv::Mat(
    (int) msg->height(),
    (int) msg->width(),
    CV_8UC3,
    img_data
  ).clone();
  // clang-format on

  // // debug
  // cv::imshow("DFCameraGClient Image", this->image);
  // cv::waitKey(1);

  // clean up
  delete img_data;
}

} // namespace gaz
} // namespace atl
