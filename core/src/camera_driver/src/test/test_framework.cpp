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

BOOST_AUTO_TEST_CASE(test_framework_list_adapters) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  const std::vector<camera_driver::adapter *> &adapters = framework->adapters();

  BOOST_TEST_MESSAGE("There are " << adapters.size() << " adapters");

  for (auto& adapter : adapters) {
    BOOST_TEST_MESSAGE( adapter->name() << ": " << adapter->description());
  }
}

BOOST_AUTO_TEST_CASE(test_framework_list_cameras) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  framework->update_cache(nullptr);
  std::vector<std::shared_ptr<camera_driver::camera_device>> cameraList = framework->camera_list();
  BOOST_TEST_MESSAGE("There are " << cameraList.size() << " cameras");

  for (auto& cam : cameraList) {
    BOOST_TEST_MESSAGE(cam->camera_descriptor_ref.id);
  }
}

BOOST_AUTO_TEST_CASE(test_framework_open_camera) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  framework->update_cache(nullptr);
  std::vector<std::shared_ptr<camera_driver::camera_device>> cameraList = framework->camera_list();
  if (cameraList.empty()) {
    return;
  }

  cameraList.front()->open_camera();
  BOOST_TEST_CHECK(cameraList.front()->opened());

  cameraList.front()->shutdown_camera();
  BOOST_TEST_CHECK(!cameraList.front()->opened());
}

BOOST_AUTO_TEST_CASE(test_framework_list_open_list) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  framework->update_cache(nullptr);
  std::vector<std::shared_ptr<camera_driver::camera_device>> cameraList = framework->camera_list();
  if (cameraList.empty()) {
    return;
  }

  std::shared_ptr<camera_driver::camera_device> &selectedCamera = cameraList.front();
  selectedCamera->open_camera();
  BOOST_TEST_CHECK(selectedCamera->opened());

  std::vector<std::shared_ptr<camera_driver::camera_device>> cameraListNew = framework->camera_list();
  BOOST_TEST_CHECK(selectedCamera->opened());
  BOOST_TEST_CHECK(cameraListNew.front()->opened());
  cameraListNew.front()->shutdown_camera();
  BOOST_TEST_CHECK(!selectedCamera->opened());

}
BOOST_AUTO_TEST_CASE(test_framework_list_multiple_times) {
  const std::shared_ptr<camera_driver::framework> &framework = camera_driver::framework::get_instance();
  for (int i = 0; i < 10; ++i) {
    framework->update_cache(nullptr);
    std::vector<std::shared_ptr<camera_driver::camera_device>> cameraList = framework->camera_list();
    BOOST_TEST_MESSAGE("ID: " << cameraList.front()->camera_descriptor_ref.id);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }


}
BOOST_AUTO_TEST_SUITE_END()
