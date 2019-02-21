//
// Created by wuyuanyi on 08/01/19.
//

#ifndef CAMERA_BACKEND_ARAVIS_ADAPTER_H
#define CAMERA_BACKEND_ARAVIS_ADAPTER_H
#include <unordered_map>

#include "adapter.h"
namespace aravis_camera_driver {
class aravis_adapter : public camera_driver::adapter {
 public:
  std::string description() const override;
  std::string name() const override;
  std::string version() const override;
  camera_driver::adapter_capability *const capabilities() override;
  void camera_list(std::vector<camera_driver::camera_descriptor> &cameraList) override;
  ~aravis_adapter() override;
  aravis_adapter();
  std::shared_ptr<camera_driver::camera_device> create_camera(camera_driver::camera_descriptor &cd) override;

 private:
  camera_driver::adapter_capability mCapabilities{
    .should_shutdown = false
  };
};
}
#endif //CAMERA_BACKEND_ARAVIS_ADAPTER_H
