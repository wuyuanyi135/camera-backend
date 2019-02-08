//
// Created by wuyuanyi on 02/01/19.
//

#ifndef CAMERA_BACKEND_ERROR_H
#define CAMERA_BACKEND_ERROR_H
#include <boost/exception/all.hpp>
#include <sstream>
#include "protos/camera_definitions.pb.h"
#include "camera_driver.h"
class adapter_error : public boost::exception, public std::exception {
  const char *what() const noexcept override {
    return "Adapter Error";
  }
};
class adapter_not_found_error : public adapter_error {
  const char *what() const noexcept override {
    return "Adapter not found error";
  }
};

class camera_error : public boost::exception, public std::exception {
 public:
  explicit camera_error(camera_driver::camera_descriptor& device) {mDevice = device;};
  const char *what() const noexcept override {
    std::stringstream ss;
    ss << "Camera Error: " << mDevice.manufacture << " " << mDevice.model << "#" << mDevice.id;
    return ss.str().c_str();
  }
 protected:
  camera_driver::camera_descriptor mDevice;
};

class configuration_error : public camera_error {
 public:
  explicit configuration_error(camera_driver::camera_descriptor &device) : camera_error(device) {}
 private:
  const char *what() const noexcept override {
    return (std::string(camera_error::what()) + " invalid configuration").c_str();
  }
};

class camera_capability_error: public camera_error {
 public:
  explicit camera_capability_error(camera_driver::camera_descriptor &device) : camera_error(device) {

  }
  const char *what() const noexcept override {
    return camera_error::what();
  }
};


typedef boost::error_info<struct tag_error_code, int> error_code;
typedef boost::error_info<struct tag_error_code, std::string> error_info;

#endif //CAMERA_BACKEND_ERROR_H
