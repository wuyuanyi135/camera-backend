//
// Created by wuyuanyi on 19/11/18.
//

#ifndef CAMERA_BACKEND_CAMERA_INFO_H
#define CAMERA_BACKEND_CAMERA_INFO_H
#include <string>
#include <vector>
#include <memory>


namespace camera_driver {
class adapter;
class camera_device;
class camera_descriptor {
 public:
  std::string manufacture;
  std::string id;
  std::string model;
  std::string serial;
  std::string interface;
};
}
#endif //CAMERA_BACKEND_CAMERA_INFO_H
