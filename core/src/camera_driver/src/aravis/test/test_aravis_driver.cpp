//
// Created by wuyuanyi on 31/12/18.
//

#define BOOST_TEST_MODULE TEST_ARAVIS_DRIVER
#include <boost/test/unit_test.hpp>
#include <boost/exception/all.hpp>
#include <arv.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <camera_driver.h>
#include "aravis/aravis_adapter.h"
#include "lodepng.h"
#include "logging.h"

struct adapter_fixture {
  adapter_fixture() : instance(new aravis_camera_driver::aravis_adapter()) {
    BOOST_TEST_MESSAGE("Created Adapter");
    BOOST_TEST_MESSAGE("Adapter Name: " << instance->name());
    BOOST_TEST_MESSAGE("Adapter Version: " << instance->version());
  }

  camera_driver::adapter *instance;
  camera_driver::camera_descriptor selected_camera_descriptor;
  std::shared_ptr<camera_driver::camera_device> selected_camera;
  ~adapter_fixture() {
    delete instance;
  }

  bool interactive_select_camera() {
    std::vector<camera_driver::camera_descriptor> cameraList;
    instance->camera_list(cameraList);
    if (cameraList.empty()) {
      return false;
    } else {
      BOOST_TEST_MESSAGE("Found " << cameraList.size() << " Camera(s).");
      for (int i = 0; i < cameraList.size(); ++i) {
        BOOST_TEST_MESSAGE(
            "\t #" << i << ": " << cameraList[i].manufacture << " " << cameraList[i].model << ": ID="
                   << cameraList[i].id
                   << " Serial=" << cameraList[i].serial);
      }

      int choice;
      if (cameraList.size() == 1) {
        choice = 0;
      } else {
        BOOST_TEST_MESSAGE("Choose the camera for test:");
        std::cin >> choice;
      }

      selected_camera_descriptor = cameraList[choice];
      selected_camera = instance->create_camera(selected_camera_descriptor.id);
      return true;
    }
  }
};

BOOST_AUTO_TEST_SUITE(test_aravis_camera_driver)

BOOST_AUTO_TEST_CASE(test_aravis_environment) {
  try {
    adapter_fixture adapter;

    if (!adapter.interactive_select_camera()) {
      BOOST_TEST_MESSAGE("No camera found. Skip further tests");
      BOOST_TEST_CHECK(true);
      return;
    }

    adapter.selected_camera->open_camera();
    BOOST_TEST_CHECK(adapter.selected_camera->opened());

  } catch (boost::exception &ex) {
    BOOST_TEST_MESSAGE(boost::current_exception_diagnostic_information(true));
    throw;
  }

  // TODO: test re-emulate the cameras. see if the pointer may be a problem. If it is, use cached singleton!!!.
}

BOOST_AUTO_TEST_CASE(test_aravis_reenumerate) {
  try {
    adapter_fixture adapter;
    std::vector<camera_driver::camera_descriptor> cameraList;
    adapter.instance->camera_list(cameraList);
    if (cameraList.empty()) {
      BOOST_TEST_MESSAGE("No camera found. Skip further tests");
      BOOST_TEST_CHECK(true);
      return;
    }
    BOOST_TEST_MESSAGE("Found " << cameraList.size() << " Camera(s).");

    const std::shared_ptr<camera_driver::camera_device> camera1 = adapter.instance->create_camera(cameraList[0].id);

    BOOST_TEST_MESSAGE("First open");
    camera1->open_camera();
    BOOST_TEST_CHECK(camera1->opened());

    cameraList.clear();
    adapter.instance->camera_list(cameraList);
    // re-enumerated device must share the same shared_ptr! so it should still be accessible and opened

    BOOST_TEST_CHECK(camera1->opened());
    const std::shared_ptr<camera_driver::camera_device> camera2 = adapter.instance->create_camera(cameraList[0].id);
    // camera2 should fail to open because now adapter does not cache objects
    try {
      camera2->open_camera();
      BOOST_TEST_FAIL("No error was thrown");
    } catch (boost::exception &ex) {
      BOOST_TEST_MESSAGE("EXPECTED ERROR THROWN: " << boost::current_exception_diagnostic_information(true));
    }

  } catch (boost::exception &ex) {
    BOOST_TEST_MESSAGE(boost::current_exception_diagnostic_information(true));
    throw;
  }
}

BOOST_AUTO_TEST_CASE(test_aravis_camera_configuration) {
  try {
    adapter_fixture adapter;
    if (!adapter.interactive_select_camera()) {
      BOOST_TEST_MESSAGE("No camera found, skip tests");
      BOOST_TEST_CHECK(true);
      return;
    }

    adapter.selected_camera->open_camera();
    BOOST_ASSERT(adapter.selected_camera->opened());

    camera_driver::camera_parameter_read default_params;
    adapter.selected_camera->get_configuration(default_params);

#define PRINT_CONFIGURATION(var, field) BOOST_TEST_MESSAGE(#field << "=" << var.field.value << ", min=" <<var.field.min << ", max=" << var.field.max);
    PRINT_CONFIGURATION(default_params, black_level);
    PRINT_CONFIGURATION(default_params, exposure);
    PRINT_CONFIGURATION(default_params, frame_rate);
    PRINT_CONFIGURATION(default_params, frame_number);
    PRINT_CONFIGURATION(default_params, gamma);
    PRINT_CONFIGURATION(default_params, gain);

    camera_driver::camera_parameter_write params{
        .black_level = {
            .value = default_params.black_level.max,
            .should_update = true,
        },
        .exposure = {
            .value = default_params.exposure.min,
            .should_update = true,
        },
        .frame_number = {
            .value = 11,
            .should_update = true,
        },
        .frame_rate = {
            .value = 5,
            .should_update = true,
        },
        .gain = {
            .value = 10.0,
            .should_update = true,
        },
        .gamma = {
            .value = default_params.gamma.min,
            .should_update = true,
        }
    };

    adapter.selected_camera->set_configuration(params);
    camera_driver::camera_parameter_read modified;
    adapter.selected_camera->get_configuration(modified);
    PRINT_CONFIGURATION(modified, black_level);
    PRINT_CONFIGURATION(modified, exposure);
    PRINT_CONFIGURATION(modified, frame_rate);
    PRINT_CONFIGURATION(modified, frame_number);
    PRINT_CONFIGURATION(modified, gamma);
    PRINT_CONFIGURATION(modified, gain);

  } catch (boost::exception &ex) {
    BOOST_TEST_ERROR(boost::current_exception_diagnostic_information(true));
  }
}

BOOST_AUTO_TEST_CASE(test_aravis_capture) {
  adapter_fixture adapter;
  if (!adapter.interactive_select_camera()) {
    BOOST_TEST_MESSAGE("No camera found, skip tests");
    BOOST_TEST_CHECK(true);
    return;
  }
  adapter.selected_camera->open_camera();
  assert(adapter.selected_camera->opened());
  camera_driver::frame frame{};
  adapter.selected_camera->capture(frame);

  BOOST_TEST_MESSAGE("Frame Height: " << frame.height);
  BOOST_TEST_MESSAGE("Frame Width: " << frame.width);
  BOOST_TEST_MESSAGE("Frame Pixel Format: " << frame.pixel_format);
  BOOST_TEST_MESSAGE("Frame Size: " << frame.size);
  BOOST_TEST_MESSAGE("Frame Id: " << frame.id);
  BOOST_TEST_MESSAGE("Frame Timestamp: " << frame.time_stamp);

  BOOST_TEST_CHECK(true);
}
BOOST_AUTO_TEST_CASE(test_aravis_capture_async) {
  adapter_fixture adapter;
  if (!adapter.interactive_select_camera()) {
    BOOST_TEST_MESSAGE("No camera found, skip tests");
    BOOST_TEST_CHECK(true);
    return;
  }
  adapter.selected_camera->open_camera();
  assert(adapter.selected_camera->opened());

  adapter.selected_camera->register_capture_start_event_handler(
      [](camera_driver::camera_device &camera) {
        BOOST_TEST_MESSAGE("Async acquisition started");
      },
      [](camera_driver::camera_device &camera) {
        BOOST_TEST_MESSAGE("Async acquisition stopped");
      }
  );

  camera_driver::camera_parameter_write param{};
  param.frame_number.value = 0;
  param.frame_number.should_update = true;

  adapter.selected_camera->set_configuration(param);

  for (int i = 0; i < 50; ++i) {
    adapter.selected_camera->capture_async([](camera_driver::frame &frame) {
      BOOST_TEST_MESSAGE("Frame received! " << frame.id);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    adapter.selected_camera->stop_capture_async();
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

}

BOOST_AUTO_TEST_CASE(test_aravis_capture_async_no_shutdown) {
  adapter_fixture adapter;
  if (!adapter.interactive_select_camera()) {
    BOOST_TEST_MESSAGE("No camera found, skip tests");
    BOOST_TEST_CHECK(true);
    return;
  }
  adapter.selected_camera->open_camera();
  assert(adapter.selected_camera->opened());

  adapter.selected_camera->register_capture_start_event_handler(
      [](camera_driver::camera_device &camera) {
        BOOST_TEST_MESSAGE("Async acquisition started");
      },
      [](camera_driver::camera_device &camera) {
        BOOST_TEST_MESSAGE("Async acquisition stopped");
      }
  );

  camera_driver::camera_parameter_write param{};
  param.frame_number.value = 0;
  param.frame_number.should_update = true;
  adapter.selected_camera->set_configuration(param);

  // without proper stop.
  adapter.selected_camera->capture_async([](camera_driver::frame &frame) {
    BOOST_TEST_MESSAGE("Frame received! " << frame.id);
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));
}

BOOST_AUTO_TEST_CASE(test_aravis_read_status) {
  adapter_fixture adapter;
  if (!adapter.interactive_select_camera()) {
    BOOST_TEST_MESSAGE("No camera found, skip tests");
    BOOST_TEST_CHECK(true);
    return;
  }
  adapter.selected_camera->open_camera();
  assert(adapter.selected_camera->opened());
  camera_driver::status status;
  adapter.selected_camera->get_status(status);
  BOOST_TEST_MESSAGE("Temperature: " << status.temperature);
}
BOOST_AUTO_TEST_CASE(test_aravis_reset) {
  adapter_fixture adapter;
  if (!adapter.interactive_select_camera()) {
    BOOST_TEST_MESSAGE("No camera found, skip tests");
    BOOST_TEST_CHECK(true);
    return;
  }
  adapter.selected_camera->open_camera();
  assert(adapter.selected_camera->opened());
  BOOST_TEST_MESSAGE("Resetting");
  adapter.selected_camera->reset();

  // after reset, the camera instance should be killed. Any access should throw error.
  try {
    adapter.selected_camera->opened();
    BOOST_TEST_FAIL("After reset, the camera is still accessible without exception?!");
  } catch (boost::exception &ex) {
    BOOST_TEST_MESSAGE("After reset, the camera is not accessible and throw "
                           << boost::current_exception_diagnostic_information(true));
  }

  std::this_thread::sleep_for(std::chrono::seconds(5));

  BOOST_TEST_MESSAGE("Reloading camera list");
  if (!adapter.interactive_select_camera()) {
    BOOST_TEST_MESSAGE("No camera found, skip tests");
    BOOST_TEST_CHECK(true);
    return;
  }
  adapter.selected_camera->open_camera();
  assert(adapter.selected_camera->opened());
}

void control_lost_cb(ArvGvDevice *gv_device) {
/* Control of the device is lost. Display a message and force application exit */
  BOOST_TEST_MESSAGE("control lost!");
}
//BOOST_AUTO_TEST_CASE(test_aravis_device_status) {
//
//  ArvDevice *const device = arv_open_device(nullptr);
//  g_signal_connect (device, "control-lost", G_CALLBACK (control_lost_cb), NULL);
//  for (int i = 0; i < 10; ++i) {
//    std::this_thread::sleep_for(std::chrono::seconds(1));
//  }
//}

BOOST_AUTO_TEST_SUITE_END()
