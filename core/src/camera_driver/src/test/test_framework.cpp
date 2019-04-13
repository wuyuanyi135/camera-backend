//
// Created by wuyuanyi on 16/01/19.
//
#define BOOST_TEST_MODULE TEST_FRAMEWORK
#include <boost/test/unit_test.hpp>
#include <thread>
#include <chrono>
#include "camera_driver.h"
#include "framework.h"
BOOST_AUTO_TEST_SUITE(test_framework)

BOOST_AUTO_TEST_CASE(test_framework_singleton) {
  const std::shared_ptr<camera_driver::framework> &framework1 = camera_driver::framework::get_instance();
  const std::shared_ptr<camera_driver::framework> &framework2 = camera_driver::framework::get_instance();
  BOOST_TEST_CHECK(framework1 == framework2);
}

BOOST_AUTO_TEST_CASE(test_framework_list_adapter) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  const camera_driver::adapter * adapter = framework->get_adapter();


  BOOST_TEST_MESSAGE( adapter->name() << ": " << adapter->description());
}

BOOST_AUTO_TEST_CASE(test_framework_list_cameras) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  std::vector<camera_driver::camera_descriptor> cameraList = framework->camera_list();
  BOOST_TEST_MESSAGE("There are " << cameraList.size() << " cameras");

  for (auto& cam : cameraList) {
    BOOST_TEST_MESSAGE(cam.id);
  }
}

BOOST_AUTO_TEST_CASE(test_framework_open_camera) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  std::vector<camera_driver::camera_descriptor> cameraList = framework->camera_list();
  if (cameraList.empty()) {
    return;
  }

  framework->select_camera_by_id(cameraList.front().id);
  const std::shared_ptr<camera_driver::camera_device> camera = framework->selected_camera;
  camera->open_camera();
  BOOST_TEST_CHECK(camera->opened());

  camera->shutdown_camera();
  BOOST_TEST_CHECK(!camera->opened());
}

BOOST_AUTO_TEST_CASE(test_framework_list_open_list) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  std::vector<camera_driver::camera_descriptor> cameraList = framework->camera_list();
  if (cameraList.empty()) {
    return;
  }

  framework->select_camera_by_id(cameraList.front().id);

  std::shared_ptr<camera_driver::camera_device> selectedCamera = framework->selected_camera;
  selectedCamera->open_camera();
  BOOST_TEST_CHECK(selectedCamera->opened());

  std::vector<camera_driver::camera_descriptor> cameraListNew = framework->camera_list();
  BOOST_TEST_CHECK(selectedCamera->opened());



  selectedCamera->shutdown_camera();
  BOOST_TEST_CHECK(!selectedCamera->opened());

}
BOOST_AUTO_TEST_CASE(test_framework_list_multiple_times) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  for (int i = 0; i < 10; ++i) {
    std::vector<camera_driver::camera_descriptor> cameraList = framework->camera_list();
    BOOST_TEST_MESSAGE("ID: " << cameraList.front().id);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

BOOST_AUTO_TEST_CASE(test_framework_open_multiple_times) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  std::vector<camera_driver::camera_descriptor> cameraList = framework->camera_list();
  framework->select_camera_by_id(cameraList.front().id);
  std::shared_ptr<camera_driver::camera_device> selectedCamera = framework->selected_camera;
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());
  try { selectedCamera->shutdown_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(!selectedCamera->opened());
  try { selectedCamera->shutdown_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(!selectedCamera->opened());
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());
  try { selectedCamera->shutdown_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(!selectedCamera->opened());
}

BOOST_AUTO_TEST_CASE(test_framework_open_multiple_times_and_list) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  std::vector<camera_driver::camera_descriptor> cameraList = framework->camera_list();
  framework->select_camera_by_id(cameraList.front().id);
  std::shared_ptr<camera_driver::camera_device> selectedCamera = framework->selected_camera;
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());

  cameraList = framework->camera_list();
  framework->select_camera_by_id(cameraList.front().id);
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());

  cameraList = framework->camera_list();
  framework->select_camera_by_id(cameraList.front().id);
  selectedCamera = framework->selected_camera;
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());

  try { selectedCamera->shutdown_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(!selectedCamera->opened());

  try { selectedCamera->shutdown_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(!selectedCamera->opened());

  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());
  try { selectedCamera->open_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(selectedCamera->opened());
  try { selectedCamera->shutdown_camera(); } catch (std::exception &e) {}
  BOOST_TEST_CHECK(!selectedCamera->opened());
}
BOOST_AUTO_TEST_SUITE_END()
