#ifndef MANTID_BEAMLINE_DETECTORINFOTEST_H_
#define MANTID_BEAMLINE_DETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/make_unique.h"

using namespace Mantid;
using Beamline::DetectorInfo;
using PosVec = std::vector<Eigen::Vector3d>;
using RotVec = std::vector<Eigen::Quaterniond>;

class DetectorInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoTest *createSuite() { return new DetectorInfoTest(); }
  static void destroySuite(DetectorInfoTest *suite) { delete suite; }

  void test_constructor() {
    std::unique_ptr<DetectorInfo> detInfo;
    TS_ASSERT_THROWS_NOTHING(detInfo = Kernel::make_unique<DetectorInfo>());
    TS_ASSERT_EQUALS(detInfo->size(), 0);
    TS_ASSERT_THROWS_NOTHING(
        detInfo = Kernel::make_unique<DetectorInfo>(PosVec(1), RotVec(1)));
    TS_ASSERT_EQUALS(detInfo->size(), 1);
  }

  void test_constructor_with_monitors() {
    std::unique_ptr<DetectorInfo> info;
    std::vector<size_t> mons{0, 2};
    TS_ASSERT_THROWS_NOTHING(
        info = Kernel::make_unique<DetectorInfo>(PosVec(3), RotVec(3), mons));
    TS_ASSERT_EQUALS(info->size(), 3);
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0, 1, 2}));
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(PosVec(3), RotVec(3), {0, 0, 0}));
    TS_ASSERT_THROWS(DetectorInfo(PosVec(3), RotVec(3), {3}),
                     std::out_of_range);
  }

  void test_constructor_length_mismatch() {
    TS_ASSERT_THROWS(DetectorInfo(PosVec(3), RotVec(2)), std::runtime_error);
  }

  void test_copy() {
    const DetectorInfo source(PosVec(7), RotVec(7));
    const auto copy(source);
    TS_ASSERT_EQUALS(copy.size(), 7);
  }

  void test_move() {
    DetectorInfo source(PosVec(7), RotVec(7));
    const auto moved(std::move(source));
    TS_ASSERT_EQUALS(moved.size(), 7);
    TS_ASSERT_EQUALS(source.size(), 0);
  }

  void test_assign() {
    const DetectorInfo source(PosVec(7), RotVec(7));
    DetectorInfo assignee(PosVec(1), RotVec(1));
    assignee = source;
    TS_ASSERT_EQUALS(assignee.size(), 7);
  }

  void test_move_assign() {
    DetectorInfo source(PosVec(7), RotVec(7));
    DetectorInfo assignee(PosVec(1), RotVec(1));
    assignee = std::move(source);
    TS_ASSERT_EQUALS(assignee.size(), 7);
    TS_ASSERT_EQUALS(source.size(), 0);
  }

  void test_no_monitors() {
    DetectorInfo info(PosVec(3), RotVec(3));
    TS_ASSERT(!info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(!info.isMonitor(2));
  }

  void test_monitors() {
    std::vector<size_t> monitors{0, 2};
    DetectorInfo info(PosVec(3), RotVec(3), monitors);
    TS_ASSERT(info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(info.isMonitor(2));
  }

  void test_duplicate_monitors_ignored() {
    std::vector<size_t> monitors{0, 0, 2, 2};
    DetectorInfo info(PosVec(3), RotVec(3), monitors);
    TS_ASSERT(info.isMonitor(0));
    TS_ASSERT(!info.isMonitor(1));
    TS_ASSERT(info.isMonitor(2));
  }

  void test_masking() {
    DetectorInfo info(PosVec(3), RotVec(3));
    TS_ASSERT(!info.isMasked(0));
    TS_ASSERT(!info.isMasked(1));
    TS_ASSERT(!info.isMasked(2));
    info.setMasked(1, true);
    TS_ASSERT(!info.isMasked(0));
    TS_ASSERT(info.isMasked(1));
    TS_ASSERT(!info.isMasked(2));
    info.setMasked(1, false);
    TS_ASSERT(!info.isMasked(0));
    TS_ASSERT(!info.isMasked(1));
    TS_ASSERT(!info.isMasked(2));
  }

  void test_masking_copy() {
    DetectorInfo source(PosVec(1), RotVec(1));
    source.setMasked(0, true);
    DetectorInfo copy(source);
    TS_ASSERT(copy.isMasked(0));
    source.setMasked(0, false);
    TS_ASSERT(!source.isMasked(0));
    TS_ASSERT(copy.isMasked(0));
  }

  void test_constructors_set_positions_correctly() {
    Eigen::Vector3d pos0{1, 2, 3};
    Eigen::Vector3d pos1{2, 3, 4};
    PosVec positions{pos0, pos1};
    const DetectorInfo info(positions, RotVec(2));
    TS_ASSERT_EQUALS(info.position(0), pos0);
    TS_ASSERT_EQUALS(info.position(1), pos1);
    const DetectorInfo info_with_monitors(positions, RotVec(2), {1});
    TS_ASSERT_EQUALS(info_with_monitors.position(0), pos0);
    TS_ASSERT_EQUALS(info_with_monitors.position(1), pos1);
  }

  void test_constructors_set_rotations_correctly() {
    Eigen::Quaterniond rot0{1, 2, 3, 4};
    Eigen::Quaterniond rot1{2, 3, 4, 5};
    RotVec rotations{rot0, rot1};
    const DetectorInfo info(PosVec(2), rotations);
    TS_ASSERT_EQUALS(info.rotation(0).coeffs(), rot0.coeffs());
    TS_ASSERT_EQUALS(info.rotation(1).coeffs(), rot1.coeffs());
    const DetectorInfo info_with_monitors(PosVec(2), rotations);
    TS_ASSERT_EQUALS(info_with_monitors.rotation(0).coeffs(), rot0.coeffs());
    TS_ASSERT_EQUALS(info_with_monitors.rotation(1).coeffs(), rot1.coeffs());
  }

  void test_position_rotation_copy() {
    DetectorInfo source(PosVec(7), RotVec(7));
    source.setPosition(0, {1, 2, 3});
    source.setRotation(0, Eigen::Quaterniond::Identity());
    const auto copy(source);
    source.setPosition(0, {3, 2, 1});
    source.setRotation(0, Eigen::Quaterniond(Eigen::AngleAxisd(
                              30.0, Eigen::Vector3d{1, 2, 3})));
    TS_ASSERT_EQUALS(copy.size(), 7);
    TS_ASSERT_EQUALS(copy.position(0), Eigen::Vector3d(1, 2, 3));
    TS_ASSERT_EQUALS(copy.rotation(0).coeffs(),
                     Eigen::Quaterniond::Identity().coeffs());
  }

  void test_setPosition() {
    DetectorInfo info(PosVec(1), RotVec(1));
    Eigen::Vector3d pos{1, 2, 3};
    info.setPosition(0, pos);
    TS_ASSERT_EQUALS(info.position(0), pos);
  }

  void test_setRotattion() {
    DetectorInfo info(PosVec(1), RotVec(1));
    Eigen::Quaterniond rot{1, 2, 3, 4};
    info.setRotation(0, rot);
    TS_ASSERT_EQUALS(info.rotation(0).coeffs(), rot.coeffs());
  }
};

#endif /* MANTID_BEAMLINE_DETECTORINFOTEST_H_ */
