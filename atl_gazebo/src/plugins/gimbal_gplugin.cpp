#include "atl/gazebo/plugins/gimbal_gplugin.hpp"

namespace atl {
namespace gaz {

GimbalGPlugin::GimbalGPlugin() { printf("LOADING [libgimbal_gplugin.so]!\n"); }

void GimbalGPlugin::Load(gazebo::physics::ModelPtr model, sdf::ElementPtr sdf) {
  UNUSED(sdf);

  // set model and bind world update callback
  // clang-format off
  this->model = model;
  this->world = this->model->GetWorld();
  this->roll_joint = this->model->GetJoint("quad::gimbal::roll_motor_joint");
  this->pitch_joint = this->model->GetJoint("quad::gimbal::pitch_motor_joint");
  this->update_conn = gazebo::event::Events::ConnectWorldUpdateBegin(
    boost::bind(&GimbalGPlugin::onUpdate, this, _1)
  );
  // clang-format on

  // gimbal model intial pose
  this->model->SetGravityMode(false);
  this->gimbal.states(0) = this->roll_joint->Position(0);
  this->gimbal.states(2) = this->pitch_joint->Position(0);
  this->gimbal.joint_setpoints(0) = 0.0;
  this->gimbal.joint_setpoints(1) = 0.0;
  // this->gimbal.joint_setpoints(2) = 0.0;

  // gazebo node
  // clang-format off
  GazeboNode::configure();
  this->addPublisher<QUATERNION_MSG>(FRAME_ORIENTATION_GTOPIC);
  this->addPublisher<QUATERNION_MSG>(JOINT_ORIENTATION_GTOPIC);
  this->addSubscriber(SETPOINT_GTOPIC, &GimbalGPlugin::setAttitudeCallback, this);
  this->addSubscriber(TRACK_GTOPIC, &GimbalGPlugin::trackTargetCallback, this);
  // clang-format on
}

void GimbalGPlugin::simulate(double dt) {
  // pre-check
  if (dt < 0.0) {
    return;
  }

  // set gimbal frame orientation
  ignition::math::Pose3d pose = this->model->WorldPose();
  Quaternion q{pose.Rot().W(), pose.Rot().X(), pose.Rot().Y(), pose.Rot().Z()};
  this->gimbal.setFrameOrientation(q);

  // attitude control and update
  Vec2 motor_inputs = this->gimbal.attitudeControllerControl(dt);
  this->gimbal.update(motor_inputs, dt);

  // set model pose
  Vec4 gimbal_state = this->gimbal.getState();
  this->roll_joint->SetPosition(0, gimbal_state(0));
  this->pitch_joint->SetPosition(0, gimbal_state(2));
}

void GimbalGPlugin::onUpdate(const gazebo::common::UpdateInfo &info) {
  double dt;
  gazebo::common::Time diff;

  // simulate
  diff = info.simTime - this->prev_sim_time;
  dt = diff.nsec / 1000000000.0; // convert nsec to sec
  this->simulate(dt);
  this->prev_sim_time = info.simTime;

  // publish
  this->publishFrameOrientation();
  this->publishJointOrientation();
}

void GimbalGPlugin::publishFrameOrientation() {
  ignition::math::Quaternion<double> q;
  QUATERNION_MSG msg;

  // build msg
  q = this->model->WorldPose().Rot();
  msg.set_w(q.W());
  msg.set_x(q.X());
  msg.set_y(q.Y());
  msg.set_z(q.Z());

  // publish msg
  this->gaz_pubs[FRAME_ORIENTATION_GTOPIC]->Publish(msg);
}

void GimbalGPlugin::publishJointOrientation() {
  // obtain frame orientation
  const Vec3 frame_euler_W = quatToEuler321(this->gimbal.frame_orientation);

  // obtain joint orientation
  const Vec3 joint_euler_B{this->roll_joint->Position(0),
                           this->pitch_joint->Position(0),
                           0.0};

  // convert joint orientation from body frame to inertial frame
  const Vec3 joint_euler_W{frame_euler_W(0) + joint_euler_B(0),
                           frame_euler_W(1) + joint_euler_B(1),
                           0.0};

  // convert quaternion
  Quaternion q = euler321ToQuat(joint_euler_W);

  // build msg
  QUATERNION_MSG msg;
  msg.set_w(q.w());
  msg.set_x(q.x());
  msg.set_y(q.y());
  msg.set_z(q.z());

  // publish msg
  this->gaz_pubs[JOINT_ORIENTATION_GTOPIC]->Publish(msg);
}

void GimbalGPlugin::setAttitudeCallback(ConstVector3dPtr &msg) {
  const Vec2 euler_W(msg->x(), msg->y());
  this->gimbal.setAttitude(euler_W);
}

void GimbalGPlugin::trackTargetCallback(ConstVector3dPtr &msg) {
  const Vec3 target_C(msg->x(), msg->y(), msg->z());
  this->gimbal.trackTarget(target_C);
}

GZ_REGISTER_MODEL_PLUGIN(GimbalGPlugin)
} // namespace gaz
} // namespace atl
