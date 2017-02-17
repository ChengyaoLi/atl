#include "awesomo_core/vision/camera/pointgrey.hpp"


namespace awesomo {

PointGreyCamera::PointGreyCamera(void) {
  this->pointgrey = NULL;
}

PointGreyCamera::~PointGreyCamera(void) {
  if (initialized) {
    delete this->pointgrey;
  }
}

int PointGreyCamera::initialize() {
  int camera_index;
  FlyCapture2::Error error;
  FlyCapture2::Property property;
  FlyCapture2::Property gain_property;

  FlyCapture2::PGRGuid guid;
  FlyCapture2::BusManager bus_manager;

  // setup
  this->pointgrey = new FlyCapture2::Camera();

  // look for camera
  error = bus_manager.GetCameraFromIndex(0, &guid);
  if (error != FlyCapture2::PGRERROR_OK) {
    log_err("ERROR! Could not find a camera!");
    return -1;
  }

  error = this->pointgrey->Connect(&guid);
  if (error != FlyCapture2::PGRERROR_OK) {
    log_err("ERROR! Failed to connect to camera!");
    return -1;
  } else {
    log_info("PointGrey camera connected!");
  }


  // set video mode format and frame  USB 2.0 cameras
  // error = this->pointgrey->SetVideoModeAndFrameRate(
  //   FlyCapture2::VIDEOMODE_640x480Y8, FlyCapture2::FRAMERATE_60);
  // if (error != FlyCapture2::PGRERROR_OK) {
  //   log_err("ERROR! Failed to set camera video mode and frame rate!");
  //   return -1;
  // } else {
  //   log_err("PointGrey camera video mode and frame rate configured!");
  // }

  this->setExposure(true, this->config.exposure_value);
  this->setGain(false ,this->config.gain_value);
  this->setShutter(false, this->config.exposure_value);
  // this->setShutter(false, this->shutter_speed);
  this->setFrameRate(false, 200);
  // this->setFormat7("RAW8", 640 * 2, 480 * 2, 0); // make these not hard coded
  this->setFormat7VideoMode(0, "RAW8", 640 * 3, 480 * 3, 0); // make these not hard coded
  // All of these are hard coded
  // start camera
  error = this->pointgrey->StartCapture();
  if (error != FlyCapture2::PGRERROR_OK) {
    log_err("ERROR! Failed start camera!");
    return -1;
  } else {
    log_info("PointGrey initialized!");
  }
  this->initialized = true;

  return 0;
}

int PointGreyCamera::getFrame(cv::Mat &image) {
  double data_size;
  double data_rows;
  unsigned int row_bytes;
  cv::Size image_size;

  FlyCapture2::Image raw_img;
  FlyCapture2::Image rgb_img;
  FlyCapture2::Error error;

  // obtain raw image
  error = this->pointgrey->RetrieveBuffer(&raw_img);
  if (error != FlyCapture2::PGRERROR_OK) {
    log_err("Failed to obtain raw image from camera!");
    return -1;
  }

  // convert to rgb
  raw_img.Convert(FlyCapture2::PIXEL_FORMAT_BGR, &rgb_img);

  // convert to opencv mat
  // clang-format off
  data_size = rgb_img.GetReceivedDataSize();
  data_rows = rgb_img.GetRows();
  row_bytes = data_size / data_rows;
  cv::Mat(rgb_img.GetRows(),
          rgb_img.GetCols(),
          CV_8UC3,
          rgb_img.GetData(),
          row_bytes).copyTo(image);
  // clang-format on

  // resize the image to reflect camera mode
  // clang-format off
  image_size = cv::Size(this->config.image_width,
                        this->config.image_height);
  cv::resize(image, image, image_size);
  // clang-format on

  return 0;
}

int PointGreyCamera::setFormat7VideoMode(int format7_mode,
    std::string pixel_format,
    int width,
    int height,
    bool center_ROI)
{
  bool supported;
  bool valid;
  FlyCapture2::Format7Info fmt7_info;
  FlyCapture2::Error error;
  FlyCapture2::Format7PacketInfo fmt7PacketInfo;
  FlyCapture2::Format7ImageSettings fmt7_settings;

  this->pointgrey->GetFormat7Info(&fmt7_info, &supported);

  std::cout << "the mode is" << fmt7_info.mode << std::endl;
  // Set the pixel format
  if (pixel_format == "MONO8") {
    fmt7_settings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO8;
  } else if (pixel_format == "MONO16") {
    fmt7_settings.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO16;
  } else if (pixel_format == "RAW8") {
    fmt7_settings.pixelFormat = FlyCapture2::PIXEL_FORMAT_RAW8;
  } else if (pixel_format == "RAW16") {
    fmt7_settings.pixelFormat = FlyCapture2::PIXEL_FORMAT_RAW16;
  }

  // Set the mode, lock into 0
  fmt7_settings.mode = FlyCapture2::MODE_2;
  this->setROI(fmt7_info, fmt7_settings, width, height, center_ROI);
  error = this->pointgrey->SetFormat7Configuration(&fmt7_settings,
      fmt7PacketInfo.recommendedBytesPerPacket);

  this->pointgrey->GetFormat7Info(&fmt7_info, &supported);
  std::cout << "the mode is" << fmt7_info.mode << std::endl;
  // switch (mode) {
  //   case 0:
  //     fmt7_settings.mode = FlyCapture2::MODE_0;
  //     break;
  //   case 1:
  //     fmt7_settings.mode = FlyCapture2::MODE_1;
  //     break;
  //   case 2:
  //     fmt7_settings.mode = FlyCapture2::MODE_2;
  //     break;
  // }

  // Validate the settings
  error = this->pointgrey->ValidateFormat7Settings(&fmt7_settings,
                                                   &valid,
                                                   &fmt7PacketInfo);
  if (error != FlyCapture2::PGRERROR_OK) {
    log_err("Format7 settings are invalid!, could not configure the camera");
    return -1;
  }

  // Send the settings to the camera with recommended packet size
  error = this->pointgrey->SetFormat7Configuration(&fmt7_settings,
      fmt7PacketInfo.recommendedBytesPerPacket);
  if (error != FlyCapture2::PGRERROR_OK) {
    log_err("Could not configure the camera with Format7!");
    return -1;
  }
  log_info("Format7 Settings applied successfully!");

  return 0;
}

int PointGreyCamera::setROI(const FlyCapture2::Format7Info &format7_info,
                             FlyCapture2::Format7ImageSettings &format7_settings,
                             int width,
                             int height,
                             bool center_ROI)
{
  const auto width_setting = this->centerROI(width, format7_info.maxWidth,
      format7_info.imageHStepSize);
  const auto height_setting = this->centerROI(height, format7_info.maxHeight,
      format7_info.imageVStepSize);
  // format7_settings.width = width_setting.first;
  // format7_settings.offsetX = width_setting.second;
  // format7_settings.height = height_setting.first;
  // format7_settings.offsetY = height_setting.second;

  format7_settings.width = 1024;
  format7_settings.offsetX = 0;
  format7_settings.height =  768;
  format7_settings.offsetY = 0;

  return 0;
}

std::pair<int, int> PointGreyCamera::centerROI(int size, int max_size, int step)
{
  if (size == 0 || size > max_size) {
    size = max_size;
  }

  // Size must be a multiple of the step
  size = size / step * step;
  const int offset = (max_size - size) / 2;
  return std::make_pair(size, offset);
}

int PointGreyCamera::changeMode(std::string mode) {
  // pre-check
  if (this->configs.find(mode) == this->configs.end()) {
    return -1;
  }
  // update camera settings
  this->config = this->configs[mode];
  return 0;
}

int PointGreyCamera::setFrameRate(bool auto_frame_rate, double rate) {
  int retval;
  const auto prop_type = FlyCapture2::FRAME_RATE;
  retval = this->setProperty(prop_type, true, auto_frame_rate, rate);
  if (retval == -1) {
    log_err("ERROR! Failed to configure frame rate!");
    return -1;
  } else {
    if (auto_frame_rate) {
      log_info("PointGrey frame rate set auto");
    } else {
      log_info("PointGrey max frame rate set to %3.2f", rate);
    }
  }
  return 0;
}

int PointGreyCamera::setShutter(bool auto_shutter, double shutter_ms) {
  int retval;
  const auto prop_type = FlyCapture2::SHUTTER;
  retval = this->setProperty(prop_type, true, auto_shutter, shutter_ms);
  if (retval == -1) {
    log_err("ERROR! Failed to configure shutter speed!");
    return -1;
  } else {
    if (auto_shutter) {
      log_info("PointGrey shutter speed set auto");
    } else {
      log_info("PointGrey shutter speed set to %3.2f", shutter_ms);
    }
  }
  return 0;
}

int PointGreyCamera::setGain(bool auto_gain, double gain_db) {
  int retval;
  const auto prop_type = FlyCapture2::GAIN;
  retval = this->setProperty(prop_type, true, auto_gain, gain_db);
  if (retval == -1) {
    log_err("ERROR! Failed to configure gain!");
    return -1;
  } else {
    if (auto_gain) {
      log_info("PointGrey gain set to auto");
    } else {
      log_info("PointGrey gain set to %3.2f DB", gain_db);
    }
  }
  return 0;
}

int PointGreyCamera::setExposure(bool auto_exposure, double exposure_val) {
  int retval;
  const auto prop_type = FlyCapture2::AUTO_EXPOSURE;
  retval = this->setProperty(prop_type, true, auto_exposure, exposure_val);
  if (retval == -1) {
    log_err("ERROR! Failed to configure exposure !");
    return -1;
  } else {
    if (auto_exposure) {
      log_info("PointGrey Exposure set to auto");
    } else {
      log_info("PointGrey Exposure set to %3.2f DB", exposure_val);
    }
  }
  return 0;
}

int PointGreyCamera::printFormat7Capabilities(void) {
  bool supported;
  FlyCapture2::Format7Info fmt7_info;

  this->pointgrey->GetFormat7Info(&fmt7_info, &supported);
  log_info(
      "Max image pixels: (%u, %u)\n"
      "Image Unit size: (%u, %u)\n"
      "Offset Unit size: (%u, %u)\n"
      "Pixel format bitfield: 0x%08x\n",
      fmt7_info.maxWidth, fmt7_info.maxHeight, fmt7_info.imageHStepSize,
      fmt7_info.imageVStepSize, fmt7_info.offsetHStepSize,
      fmt7_info.offsetVStepSize, fmt7_info.pixelFormatBitField
  );

  return 0;
}

int PointGreyCamera::setProperty(const FlyCapture2::PropertyType prop_type, bool on, bool auto_on, double value) {
  FlyCapture2::Error error;
  const auto prop_info = this->getPropertyInfo(prop_type);

  if (prop_info.present) {
    FlyCapture2::Property prop;
    prop.type = prop_type;
    prop.onOff = on && prop_info.onOffSupported;
    prop.autoManualMode = auto_on && prop_info.autoSupported;
    prop.absControl = prop_info.absValSupported;
    value = std::max<double>(std::min<double>(value, prop_info.absMax), prop_info.absMin);
    prop.absValue = value;

    error = this->pointgrey->SetProperty(&prop);
    if (error != FlyCapture2::PGRERROR_OK) {
      return -1;
    }
  } else {
    return -2;
  }
  return 0;
}

FlyCapture2::Property PointGreyCamera::getProperty(
    const FlyCapture2::PropertyType &prop_type)
{
  FlyCapture2::Error error;
  FlyCapture2::Property prop;

  prop.type = prop_type;
  error = this->pointgrey->GetProperty(&prop);
  if (error != FlyCapture2::PGRERROR_OK) {
    log_err("ERROR! failed to get property type!");
  }
  return prop;
}

FlyCapture2::PropertyInfo PointGreyCamera::getPropertyInfo(
    const FlyCapture2::PropertyType &prop_type)
{
  FlyCapture2::Error error;
  FlyCapture2::PropertyInfo prop_info;

  prop_info.type = prop_type;
  error = this->pointgrey->GetPropertyInfo(&prop_info);
  if (error != FlyCapture2::PGRERROR_OK) {
    log_err("ERROR! failed to get property type!");
  }
  return prop_info;
}



}  // end of awesomo namespace
