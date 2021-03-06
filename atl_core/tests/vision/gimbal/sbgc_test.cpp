#include "atl/vision/gimbal/sbgc.hpp"
#include "atl/atl_test.hpp"

namespace atl {

TEST(SBGC, ConnectDisconnect) {
  SBGC sbgc("/dev/ttyUSB0");
  EXPECT_EQ(0, sbgc.connect());
  EXPECT_EQ(0, sbgc.disconnect());
}

TEST(SBGC, SendFrame) {
  int retval;
  SBGCFrame frame;
  SBGC sbgc("/dev/ttyUSB0");

  // setup
  sbgc.connect();

  // turn motors on
  frame.buildFrame(CMD_MOTORS_ON);
  retval = sbgc.sendFrame(frame);
  EXPECT_EQ(retval, 0);
  sleep(1);

  // turn motors off
  frame.buildFrame(CMD_MOTORS_OFF);
  retval = sbgc.sendFrame(frame);
  EXPECT_EQ(retval, 0);
  sleep(1);
}

TEST(SBGC, ReadFrame) {
  int retval;
  SBGCFrame frame;
  SBGC sbgc("/dev/ttyUSB0");

  // setup
  sbgc.connect();

  // read frame
  frame.buildFrame(CMD_BOARD_INFO);
  sbgc.sendFrame(frame);
  retval = sbgc.readFrame(CMD_BOARD_INFO_FRAME_SIZE, frame);

  // assert
  EXPECT_EQ(18, frame.data_size);
  EXPECT_EQ(0, 0);
}

TEST(SBGC, getBoardInfo) {
  SBGC sbgc("/dev/ttyUSB0");

  // setup
  sbgc.connect();

  // test get board info
  sbgc.getBoardInfo();
  printf("board version: %d\n", sbgc.board_version);
  printf("firmware version: %d\n", sbgc.firmware_version);
  printf("debug mode: %d\n", sbgc.debug_mode);
  printf("board features: %d\n", sbgc.board_features);
  printf("connection flags: %d\n", sbgc.connection_flags);

  EXPECT_EQ(30, sbgc.board_version);
  EXPECT_EQ(2604, sbgc.firmware_version);
}

TEST(SBGC, getRealTimeData) {
  SBGC sbgc("/dev/ttyUSB0");

  // setup
  sbgc.connect();

  // test get imu data
  for (int i = 0; i < 100; ++i) {
    sbgc.getRealtimeData();
    sbgc.data.printData();
    // printf("roll %f \n", sbgc.data.rc_angles(0));
    // printf("pitch %f \n", sbgc.data.rc_angles(1));
    // printf("yaw %f \n", sbgc.data.rc_angles(2));
  }
}

TEST(SBGC, setAngle) {
  SBGC sbgc("/dev/ttyUSB0");

  EXPECT_EQ(0, sbgc.connect());
  sbgc.on();

  sbgc.setAngle(0, -90, 0);
  sleep(2);
  for (int angle = -95; angle < 20; angle += 3) {
    sbgc.setAngle(0, angle, 0);
  }
  sbgc.off();
}

TEST(SBGC, setSpeedAngle) {
  SBGC sbgc("/dev/ttyUSB0");

  EXPECT_EQ(0, sbgc.connect());
  sbgc.on();

  sbgc.setSpeedAngle(0, 10, 0, 0, -2, 0);
  sleep(3);

  sbgc.off();
}

} // namespace atl
