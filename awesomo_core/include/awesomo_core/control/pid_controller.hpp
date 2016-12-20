#ifndef __AWESOMO_CONTROL_PID_CONTROLLER_HPP__
#define __AWESOMO_CONTROL_PID_CONTROLLER_HPP__

#include <float.h>
#include <math.h>

namespace awesomo {

class PID {
public:
  double error_prev;
  double error_sum;

  double error_p;
  double error_i;
  double error_d;

  double k_p;
  double k_i;
  double k_d;

  PID(void);
  PID(double k_p, double k_i, double k_d);
  double calculate(double setpoint, double input, double dt);
  void reset(void);
};

}  // end of awesomo namespace
#endif
