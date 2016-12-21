#include "awesomo_core/quadrotor/hover_mode.hpp"


namespace awesomo {

HoverMode::HoverMode(void) {
  this->configured = false;
  this->hover_height = 0.0;
}

int HoverMode::configure(std::string config_file) {
  try {
    YAML::Node config;

    // pre-check
    if (file_exists(config_file) == false) {
      log_err("File not found: %s", config_file.c_str());
      return -1;
    }

    // parse configs
    config = YAML::LoadFile(config_file);
    this->hover_height = config["hover_height"].as<double>();

  } catch (std::exception &ex) {
    std::cout << ex.what();
    return -2;
  }

  this->configured = true;
  return 0;
}

}  // end of awesomo namespace
