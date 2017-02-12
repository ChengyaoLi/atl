#include "awesomo_core/control/landing_controller.hpp"


namespace awesomo {

// TRAJECTORY
Trajectory::Trajectory(void) {
  this->loaded = false;
}

int Trajectory::load(std::string filepath) {
  MatX traj_data;

  // pre-check
  if (file_exists(filepath) == false) {
    log_err("File not found: %s", filepath.c_str());
    return -1;
  }

  // load trajectory file
  // assumes each column is: x,vx,z,vz,az,theta
  csv2mat(filepath, true, traj_data);
  if (traj_data.rows() == 0) {
    log_err(ETROWS, filepath.c_str());
    return -2;
  } else if (traj_data.cols() != 6) {
    log_err(ETCOLS, filepath.c_str());
    return -2;
  }

  // set trajectory class
  this->x = traj_data.col(0);
  this->vx = traj_data.col(1);
  this->z = traj_data.col(2);
  this->vz = traj_data.col(3);
  this->az = traj_data.col(4);
  this->theta = traj_data.col(5);

  this->loaded = true;
  return 0;
}

void Trajectory::reset(void) {
  this->x = VecX();
  this->z = VecX();
  this->vx = VecX();
  this->vz = VecX();
  this->az = VecX();
  this->theta = VecX();

  this->loaded = false;
}


// TRAJECTORY INDEX
TrajectoryIndex::TrajectoryIndex(void) {
  this->loaded = false;

  this->traj_dir = "";
  this->index_data = MatX();
  this->pos_thres = 0.0;
  this->vel_thres = 0.0;
}

int TrajectoryIndex::load(std::string index_file,
                          double pos_thres,
                          double vel_thres) {
  // pre-check
  if (file_exists(index_file) == false) {
    log_err("File not found: %s", index_file.c_str());
    return -1;
  }

  // load trajectory index
  // assumes each column is: (index, p0_x, p0_z, pf_x, pf_z, z)
  csv2mat(index_file, true, this->index_data);
  this->traj_dir = std::string(dirname((char *) index_file.c_str()));
  this->pos_thres = pos_thres;
  this->vel_thres = vel_thres;

  if (this->index_data.rows() == 0) {
    log_err(ETIROWS, index_file.c_str());
    return -2;
  } else if (this->index_data.cols() != 6) {
    log_err(ETICOLS, index_file.c_str());
    return -2;
  }

  this->loaded = true;
  return 0;
}

int TrajectoryIndex::find(Vec2 p0, Vec2 pf, double v, Trajectory &traj) {
  bool p_ok, v_ok;
  std::vector<double> p0_matches;
  std::vector<VecX> matches;
  std::string traj_file;

  // pre-check
  if (this->loaded == false) {
    return -1;
  }

  // NOTE: the following is not the most efficient way of implementing
  // a lookup table, a better way could involve a search tree and traverse it
  // or even a bucket based approach. The following implements a list traversal
  // type search, which is approx O(n), ok for small lookups.

  // find rows in the index that have same approx
  // start height (p0_z) and velocity (v)
  for (int i = 0; i < this->index_data.rows(); i++) {
    p_ok = fabs(p0(1) - this->index_data(i, 2)) < this->pos_thres;
    v_ok = fabs(v - this->index_data(i, 5)) < this->vel_thres;

    if (p_ok && v_ok) {
      p0_matches.push_back(i);
    }
  }

  // filter those that have approx end position (pf_x)
  for (int i = 0; i < p0_matches.size(); i++) {
    if (fabs(pf(0) - this->index_data(p0_matches[i], 3)) < this->pos_thres) {
      matches.push_back(this->index_data.row(p0_matches[i]));
    }
  }

  if (matches.size() == 0) {
    log_err(ETIFAIL, p0(0), p0(1), pf(0), pf(1), v);
    return -2;  // found no trajectory
  }

  // load trajectory
  traj_file = this->traj_dir + "/";
  traj_file += std::to_string((int) matches[0](0)) + ".csv";
  if (traj.load(traj_file) != 0) {
    log_err(ETLOAD, traj_file.c_str());
    return -3;
  }

  return 0;
}


// LANDING CONTROLLER
LandingController::LandingController(void) {
  this->configured = false;

  this->pctrl_dt = 0.0;
  this->x_controller = PID(0.0, 0.0, 0.0);
  this->y_controller = PID(0.0, 0.0, 0.0);
  this->z_controller = PID(0.0, 0.0, 0.0);
  this->hover_throttle = 0.0;

  this->vctrl_dt = 0.0;
  this->vx_controller = PID(0.0, 0.0, 0.0);
  this->vy_controller = PID(0.0, 0.0, 0.0);
  this->vz_controller = PID(0.0, 0.0, 0.0);

  this->roll_limit[0] = 0.0;
  this->roll_limit[1] = 0.0;
  this->pitch_limit[0] = 0.0;
  this->pitch_limit[1] = 0.0;
  this->throttle_limit[0] = 0.0;
  this->throttle_limit[1] = 0.0;

  this->pctrl_setpoints << 0.0, 0.0, 0.0;
  this->pctrl_outputs << 0.0, 0.0, 0.0, 0.0;
  this->vctrl_setpoints << 0.0, 0.0, 0.0;
  this->vctrl_outputs << 0.0, 0.0, 0.0, 0.0;
  this->att_cmd = AttitudeCommand();
}

int LandingController::configure(std::string config_file) {
  ConfigParser parser;

  // load config
  // clang-format off
  parser.addParam<double>("roll_controller.k_p", &this->y_controller.k_p);
  parser.addParam<double>("roll_controller.k_i", &this->y_controller.k_i);
  parser.addParam<double>("roll_controller.k_d", &this->y_controller.k_d);

  parser.addParam<double>("pitch_controller.k_p", &this->x_controller.k_p);
  parser.addParam<double>("pitch_controller.k_i", &this->x_controller.k_i);
  parser.addParam<double>("pitch_controller.k_d", &this->x_controller.k_d);

  parser.addParam<double>("throttle_controller.k_p", &this->z_controller.k_p);
  parser.addParam<double>("throttle_controller.k_i", &this->z_controller.k_i);
  parser.addParam<double>("throttle_controller.k_d", &this->z_controller.k_d);
  parser.addParam<double>("throttle_controller.hover_throttle", &this->hover_throttle);

  parser.addParam<double>("vx_controller.k_p", &this->vx_controller.k_p);
  parser.addParam<double>("vx_controller.k_i", &this->vx_controller.k_i);
  parser.addParam<double>("vx_controller.k_d", &this->vx_controller.k_d);

  parser.addParam<double>("vy_controller.k_p", &this->vy_controller.k_p);
  parser.addParam<double>("vy_controller.k_i", &this->vy_controller.k_i);
  parser.addParam<double>("vy_controller.k_d", &this->vy_controller.k_d);

  parser.addParam<double>("vz_controller.k_p", &this->vz_controller.k_p);
  parser.addParam<double>("vz_controller.k_i", &this->vz_controller.k_i);
  parser.addParam<double>("vz_controller.k_d", &this->vz_controller.k_d);

  parser.addParam<double>("roll_limit.min", &this->roll_limit[0]);
  parser.addParam<double>("roll_limit.max", &this->roll_limit[1]);

  parser.addParam<double>("pitch_limit.min", &this->pitch_limit[0]);
  parser.addParam<double>("pitch_limit.max", &this->pitch_limit[1]);

  parser.addParam<double>("throttle_limit.min", &this->throttle_limit[0]);
  parser.addParam<double>("throttle_limit.max", &this->throttle_limit[1]);
  // clang-format on

  if (parser.load(config_file) != 0) {
    return -1;
  }

  // convert roll and pitch limits from degrees to radians
  this->roll_limit[0] = deg2rad(this->roll_limit[0]);
  this->roll_limit[1] = deg2rad(this->roll_limit[1]);
  this->pitch_limit[0] = deg2rad(this->pitch_limit[0]);
  this->pitch_limit[1] = deg2rad(this->pitch_limit[1]);

  // initialize setpoints to zero
  this->pctrl_setpoints << 0.0, 0.0, 0.0;
  this->pctrl_outputs << 0.0, 0.0, 0.0, 0.0;
  this->vctrl_setpoints << 0.0, 0.0, 0.0;
  this->vctrl_outputs << 0.0, 0.0, 0.0, 0.0;

  this->configured = true;
  return 0;
}

Vec4 LandingController::calculatePositionErrors(Vec3 errors,
                                                 double yaw,
                                                 double dt) {
  double r, p, y, t;
  Vec3 euler;
  Mat3 R;

  // check rate
  this->pctrl_dt += dt;
  if (this->pctrl_dt < 0.01) {
    return this->pctrl_outputs;
  }

  // roll, pitch, yaw and throttle (assuming NWU frame)
  // clang-format off
  r = -this->y_controller.calculate(errors(1), 0.0, this->pctrl_dt);
  p = this->x_controller.calculate(errors(0), 0.0, this->pctrl_dt);
  y = yaw;
  t = this->hover_throttle + this->z_controller.calculate(errors(2), 0.0, this->pctrl_dt);
  t /= fabs(cos(r) * cos(p));  // adjust throttle for roll and pitch
  // clang-format o

  // limit roll, pitch
  r = (r < this->roll_limit[0]) ? this->roll_limit[0] : r;
  r = (r > this->roll_limit[1]) ? this->roll_limit[1] : r;
  p = (p < this->pitch_limit[0]) ? this->pitch_limit[0] : p;
  p = (p > this->pitch_limit[1]) ? this->pitch_limit[1] : p;

  // limit throttle
  t = (t < 0) ? 0.0 : t;
  t = (t > 1.0) ? 1.0 : t;

  // keep track of setpoints and outputs
  this->pctrl_setpoints = errors;
  this->pctrl_outputs << r, p, y, t;
  this->pctrl_dt = 0.0;

  return pctrl_outputs;
}

Vec4 LandingController::calculateVelocityErrors(Vec3 errors,
                                                double yaw,
                                                double dt) {
  double r, p, y, t;
  Vec3 euler;
  Mat3 R;

  // check rate
  this->vctrl_dt += dt;
  if (this->vctrl_dt < 0.01) {
    return this->vctrl_outputs;
  }

  // roll, pitch, yaw and throttle (assuming NWU frame)
  // clang-format off
  r = -this->vy_controller.calculate(errors(1), 0.0, this->vctrl_dt);
  p = this->vx_controller.calculate(errors(0), 0.0, this->vctrl_dt);
  y = 0.0;
  t = this->vz_controller.calculate(errors(2), 0.0, this->vctrl_dt);
  t /= fabs(cos(r) * cos(p));  // adjust throttle for roll and pitch
  // clang-format on

  // limit roll, pitch, throttle
  r = (r < this->roll_limit[0]) ? this->roll_limit[0] : r;
  r = (r > this->roll_limit[1]) ? this->roll_limit[1] : r;
  p = (p < this->pitch_limit[0]) ? this->pitch_limit[0] : p;
  p = (p > this->pitch_limit[1]) ? this->pitch_limit[1] : p;
  t = (t < this->throttle_limit[0]) ? this->throttle_limit[0] : t;
  t = (t > this->throttle_limit[1]) ? this->throttle_limit[1] : t;

  // keep track of setpoints and outputs
  this->vctrl_setpoints = errors;
  this->vctrl_outputs << r, p, y, t;
  this->vctrl_dt = 0.0;

  return this->vctrl_outputs;
}

AttitudeCommand LandingController::calculate(Vec3 pos_errors,
                                             Vec3 vel_errors,
                                             double yaw,
                                             double dt) {
  this->calculatePositionErrors(pos_errors, yaw, dt);
  this->calculateVelocityErrors(vel_errors, yaw, dt);
  this->att_cmd = AttitudeCommand(this->pctrl_outputs + this->vctrl_outputs);
  return this->att_cmd;
}


AttitudeCommand LandingController::calculate(Vec3 target_pos_bf,
                                             Vec3 target_vel_bf,
                                             Vec3 pos,
                                             Vec3 pos_prev,
                                             double yaw,
                                             double dt) {
  Vec3 perrors, verrors;
  double dz;

  dz = (pos(2) - pos_prev(2)) / dt;

  perrors(0) = target_pos_bf(0);
  perrors(1) = target_pos_bf(1);
  perrors(2) = pos_prev(2) - pos(2);

  verrors(0) = target_vel_bf(0);
  verrors(1) = target_vel_bf(1);
  verrors(2) = -0.2 - dz;

  return this->calculate(perrors, verrors, yaw, dt);
}

// int LandingController::executeTrajectory(Trajectory traj,
//                                          Vec3 quad_pos,
//                                          Vec3 target_pos) {
//   int retval;
//
//   // load trajectory
//   // retval = this->loadTrajectory(filepath);
//   // if (retval != 0) {
//   //   log_err("Failed to load trajectory [%s]!", filepath.c_str());
//   // }
//
//
//   return 0;
// }

void LandingController::reset(void) {
  this->x_controller.reset();
  this->y_controller.reset();
  this->z_controller.reset();

  this->vx_controller.reset();
  this->vy_controller.reset();
  this->vz_controller.reset();
}

void LandingController::printOutputs(void) {
  double r, p, t;

  r = rad2deg(this->pctrl_outputs(0));
  p = rad2deg(this->pctrl_outputs(1));
  t = this->pctrl_outputs(3);

  std::cout << "roll: " << std::setprecision(2) << r << "\t";
  std::cout << "pitch: " << std::setprecision(2) << p << "\t";
  std::cout << "throttle: " << std::setprecision(2) << t << std::endl;
}

void LandingController::printErrors(void) {
  double p, i, d;

  p = this->x_controller.error_p;
  i = this->x_controller.error_i;
  d = this->x_controller.error_d;

  std::cout << "x_controller: " << std::endl;
  std::cout << "\terror_p: " << std::setprecision(2) << p << "\t";
  std::cout << "\terror_i: " << std::setprecision(2) << i << "\t";
  std::cout << "\terror_d: " << std::setprecision(2) << i << std::endl;

  p = this->y_controller.error_p;
  i = this->y_controller.error_i;
  d = this->y_controller.error_d;

  std::cout << "y_controller: " << std::endl;
  std::cout << "\terror_p: " << std::setprecision(2) << p << "\t";
  std::cout << "\terror_i: " << std::setprecision(2) << i << "\t";
  std::cout << "\terror_d: " << std::setprecision(2) << i << std::endl;

  p = this->z_controller.error_p;
  i = this->z_controller.error_i;
  d = this->z_controller.error_d;

  std::cout << "z_controller: " << std::endl;
  std::cout << "\terror_p: " << std::setprecision(2) << p << "\t";
  std::cout << "\terror_i: " << std::setprecision(2) << i << "\t";
  std::cout << "\terror_d: " << std::setprecision(2) << i << std::endl;
}

}  // end of awesomo namespace