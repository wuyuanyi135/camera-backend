//
// Created by wuyuanyi on 08/01/19.
//
#include <arv.h>
#include <sstream>
#include <iostream>
#include "aravis_adapter.h"
#include "aravis_camera.h"
#include "logging.h"
namespace aravis_camera_driver {

std::string aravis_adapter::description() const {
  return "Aravis GenICam Driver";
}
std::string aravis_adapter::name() const {
  return "Aravis";
}
std::string aravis_adapter::version() const {
  std::stringstream v;
  v << ARAVIS_MAJOR_VERSION << "." << ARAVIS_MINOR_VERSION << "." << ARAVIS_MICRO_VERSION;
  return v.str();
}
camera_driver::adapter_capability *const aravis_adapter::capabilities() {
  return &mCapabilities;
}
void aravis_adapter::camera_list(std::vector<camera_driver::camera_descriptor> &cameraList) {
  // rebuild the cache. The camera should ensure the same instance being returned for the camera.
  mCameraCache.clear();
  arv_update_device_list();
  CDINFO("Updating devices");

  unsigned int n = arv_get_n_devices();
  CDINFO(n << " devices detected");

  for (unsigned int i = 0; i < n; ++i) {

    camera_driver::camera_descriptor cd{
        .manufacture = arv_get_device_vendor(i),
        .id = arv_get_device_id(i),
        .model = arv_get_device_model(i),
        .serial = arv_get_device_serial_nbr(i),
        .interface = arv_get_device_protocol(i),
    };
    cameraList.emplace_back(cd);
    camera_driver::camera_container container{
        .camera_descriptor = cd,
        .adapter = this,
        .device = aravis_camera::get_camera_instance(cd),
    };
    container.camera_descriptor.connected = container.device->opened();
    std::pair<std::string, camera_driver::camera_container> p(cd.id, container);
    mCameraCache.insert(p);
    CDINFO("Camera " << cd.id << " registered");
  }
}

aravis_adapter::aravis_adapter() {
  CDINFO("Aravis adapter Loaded");
}
bool aravis_adapter::get_camera_by_id(std::string id, camera_driver::camera_container &container) {
  if (mCameraCache.find(id) == mCameraCache.end()) {
    return false;
  }
  container = mCameraCache[id];
  return true;
}
aravis_adapter::~aravis_adapter() {
  arv_shutdown();
}

}
