#include "atl/estimation/kf_tracker.hpp"

namespace atl {

KFTracker::KFTracker() {
  this->configured = false;
  this->initialized = false;

  this->mode = -1;
  this->nb_states = 0;
  this->nb_dimensions = 0;
  this->sanity_dist = FLT_MAX;
  this->config_file = "";

  this->mu = VecX::Zero(1);

  this->B = MatX::Zero(1, 1);
  this->R = MatX::Zero(1, 1);

  this->C = MatX::Zero(1, 1);
  this->Q = MatX::Zero(1, 1);

  this->S = MatX::Zero(1, 1);
  this->I = MatX::Zero(1, 1);
  this->K = MatX::Zero(1, 1);

  this->mu_p = VecX::Zero(1);
  this->S_p = MatX::Zero(1, 1);
}

int KFTracker::configure(std::string config_file) {
  ConfigParser parser;
  std::string mode;

  // parse and load config file
  parser.addParam("nb_states", &this->nb_states);
  parser.addParam("nb_dimensions", &this->nb_dimensions);
  parser.addParam("sanity_dist", &this->sanity_dist);
  parser.addParam("motion_noise_matrix", &this->R);
  parser.addParam("measurement_matrix", &this->C);
  parser.addParam("measurement_noise_matrix", &this->Q);
  this->config_file = config_file;
  if (parser.load(config_file) != 0) {
    return -1;
  }

  this->configured = true;
  return 0;
}

int KFTracker::initialize(VecX mu) {
  // pre-check
  if (this->configured == false) {
    return -1;
  }

  // initialize
  this->mu = mu;

  this->B = MatX::Zero(this->nb_states, this->nb_states);
  // this->R;  // set during configuration

  // this->C;  // set during configuration
  // this->Q;  // set during configuration

  this->S = MatX::Identity(this->nb_states, this->nb_states);
  this->I = MatX::Identity(this->nb_states, this->nb_states);
  // this->K;

  this->mu_p = VecX::Zero(this->nb_states);
  this->S_p = MatX::Zero(this->nb_states, this->nb_states);

  // check
  if (this->checkDimensions() != 0) {
    return -2;
  }

  this->initialized = true;
  return 0;
}

int KFTracker::checkDimensions() {
  if (this->mu.size() != this->nb_states) {
    LOG_ERROR(EMUSIZE, this->nb_states, (int) this->mu.size());
    LOG_ERROR(ECHECKCONFIG, this->config_file.c_str());
    return -1;

  } else if (this->R.rows() != this->nb_states) {
    LOG_ERROR(ERROWSIZE, this->nb_states, (int) this->R.rows());
    LOG_ERROR(ECHECKCONFIG, this->config_file.c_str());
    return -1;

  } else if (this->R.cols() != this->nb_states) {
    LOG_ERROR(ERCOLSIZE, this->nb_states, (int) this->R.cols());
    LOG_ERROR(ECHECKCONFIG, this->config_file.c_str());
    return -1;

  } else if (this->C.rows() != this->nb_dimensions) {
    LOG_ERROR(ECROWSIZE, this->nb_dimensions, (int) this->C.rows());
    LOG_ERROR(ECHECKCONFIG, this->config_file.c_str());
    return -1;

  } else if (this->C.cols() != this->nb_states) {
    LOG_ERROR(ECCOLSIZE, this->nb_states, (int) this->C.cols());
    LOG_ERROR(ECHECKCONFIG, this->config_file.c_str());
    return -1;

  } else if (this->Q.rows() != this->nb_dimensions) {
    LOG_ERROR(EQROWSIZE, this->nb_dimensions, (int) this->Q.rows());
    LOG_ERROR(ECHECKCONFIG, this->config_file.c_str());
    return -1;

  } else if (this->Q.cols() != this->nb_dimensions) {
    LOG_ERROR(EQCOLSIZE, this->nb_dimensions, (int) this->Q.cols());
    LOG_ERROR(ECHECKCONFIG, this->config_file.c_str());
    return -1;
  }

  return 0;
}

int KFTracker::reset(VecX mu) {
  // configure
  if (this->configure(this->config_file) != 0) {
    this->configured = false;
    return -1;
  }

  // initialize
  if (this->initialize(mu) != 0) {
    this->initialized = false;
    return -2;
  }

  return 0;
}

int KFTracker::sanityCheck(Vec3 prev_pos, Vec3 curr_pos) {
  double dist;

  // pre-check
  if (this->initialized == false) {
    return -1;
  }

  // sanity check
  dist = (prev_pos - curr_pos).norm();
  if (dist > this->sanity_dist) {
    return -2;
  }

  return 0;
}

int KFTracker::estimate(MatX A, VecX y) {
  // pre-check
  if (this->initialized == false) {
    return -1;
  } else if (A.rows() != this->nb_states || A.cols() != this->nb_states) {
    LOG_ERROR(EASIZE, this->nb_states);
    return -2;
  } else if (y.size() != this->C.rows()) {
    LOG_ERROR(EYSIZE, (int) this->C.rows());
    return -2;
  }

  // prediction update
  mu_p = A * mu;
  S_p = A * S * A.transpose() + R;

  // measurement update
  K = S_p * C.transpose() * (C * S_p * C.transpose() + Q).inverse();
  mu = mu_p + K * (y - C * mu_p);
  S = (I - K * C) * S_p;

  return 0;
}

} // namespace atl
