//
// Created by wuyuanyi on 19/11/18.
//

#ifndef CAMERA_BACKEND_CAMERA_INFO_H
#define CAMERA_BACKEND_CAMERA_INFO_H
#include <string>
#include <vector>
#include <memory>

namespace camera_driver {
class camera_device;
class camera_descriptor {
 public:
  std::string manufacture;
  const char* id;
  std::string model;
  std::string serial;
  std::string interface;
  bool connected;
};
}
#endif //CAMERA_BACKEND_CAMERA_INFO_H
