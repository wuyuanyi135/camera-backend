//
// Created by wuyuanyi on 08/01/19.
//

#ifndef CAMERA_BACKEND_FRAMEWORK_H
#define CAMERA_BACKEND_FRAMEWORK_H
#include <memory>
#include <unordered_map>
#include "adapter.h"
#include "aravis/aravis_adapter.h"
namespace camera_driver {
class framework {
 private:
  static std::shared_ptr<framework> mInstance;
  static std::unique_ptr<adapter> INTERFACE_REGISTRATION;


 public:
  static std::shared_ptr<framework> get_instance();

 private:
  framework();

 public:
  std::shared_ptr<camera_driver::camera_device> selected_camera;
  adapter* get_adapter();
  std::vector<camera_driver::camera_descriptor> camera_list();
  void select_camera_by_id(std::string);
};
}
#endif //CAMERA_BACKEND_FRAMEWORK_H
