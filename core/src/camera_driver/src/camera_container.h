//
// Created by wuyuanyi on 05/01/19.
//

#ifndef CAMERA_BACKEND_CAMERA_CONTAINER_H
#define CAMERA_BACKEND_CAMERA_CONTAINER_H
#include "camera_driver.h"
#include "camera_descriptor.h"
#include <memory>
namespace camera_driver {
class adapter;
/// container structure linking the camera descriptor, camera instance, and the adapter.
struct camera_container {
  camera_driver::camera_descriptor camera_descriptor;
  camera_driver::adapter* adapter; // the adapter will not be released unless error.
  std::shared_ptr<camera_driver::camera_device> device;
};
}
#endif //CAMERA_BACKEND_CAMERA_CONTAINER_H
