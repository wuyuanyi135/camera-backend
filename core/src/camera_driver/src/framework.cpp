//
// Created by wuyuanyi on 08/01/19.
//

#include <assert.h>
#include <iostream>

#include "framework.h"
#include "camera_driver.h"
#include "exceptions.h"
namespace camera_driver {
std::unique_ptr<adapter>
    framework::INTERFACE_REGISTRATION = std::unique_ptr<adapter>(new aravis_camera_driver::aravis_adapter());

std::shared_ptr<framework> framework::mInstance;

std::shared_ptr<framework> framework::get_instance() {
  if (!mInstance) {
    mInstance = std::shared_ptr<framework>(new framework());
  }
  return mInstance;
}
framework::framework() {
}

adapter *framework::get_adapter() {
  return &*INTERFACE_REGISTRATION;
}
std::vector<camera_driver::camera_descriptor> framework::camera_list() {
  std::vector<camera_driver::camera_descriptor> list;
  get_adapter()->camera_list(list);

  return list;
}

void framework::select_camera_by_id(std::string id) {

  if (this->selected_camera) {
    if (id == this->selected_camera->id) {
      return;
    }
    this->selected_camera.reset();
  }
  this->selected_camera = this->get_adapter()->create_camera(id);
}

}