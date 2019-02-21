//
// Created by wuyuanyi on 08/01/19.
//

#ifndef CAMERA_BACKEND_EXCEPTIONS_H
#define CAMERA_BACKEND_EXCEPTIONS_H
#include <boost/exception/all.hpp>
namespace camera_driver {
class framework_error : public boost::exception, public std::exception {
 public:
  const char *what() const noexcept override {
    return "Framework error";
  }
};

class id_not_found_error : public framework_error {

};

class camera_error : public boost::exception, public std::exception {
 public:
  explicit camera_error(const camera_device *device) : mDevice(device) {}
  explicit camera_error() : mDevice() {}
 private:
  const camera_driver::camera_device *mDevice;
};

class parameter_not_supported_error : public camera_error {
 private:
  std::string mParameter;
 public:
  parameter_not_supported_error(const camera_device *device, std::string &parameterName)
      : camera_error(device), mParameter(parameterName) {
  }
};

template<typename T>
class parameter_out_of_range_error : public camera_error {
 public:
  parameter_out_of_range_error(const camera_device *device, T min, T max) : camera_error(device), max(max), min(min) {}
 private:
  T min;
  T max;
};

class write_parameter_error : public camera_error {
 public:
  write_parameter_error(const camera_device *device, int errorCode) : camera_error(device), errorCode(errorCode) {}
 private:
  int errorCode;

};

class camera_creation_error : public camera_error {
 public:
  explicit camera_creation_error() : camera_error() {

  }
};

class camera_not_open_error : public camera_error {
 public:
  explicit camera_not_open_error(const camera_device *device) : camera_error(device) {

  }
};

class camera_start_failed_error : public camera_error {
 public:
  explicit camera_start_failed_error(const camera_device *device) : camera_error(device) {

  }
};

class camera_capture_started_error : public camera_error {
 public:
  explicit camera_capture_started_error(const camera_device *device) : camera_error(device) {

  }
};

class camera_shutdown_failed_error : public camera_error {
 public:
  explicit camera_shutdown_failed_error(const camera_device *device) : camera_error(device) {

  }
};

class invalid_camera_error: public camera_error {
 public:
  explicit invalid_camera_error(const camera_device *device) : camera_error(device) {

  }
};

} // namespace



#endif //CAMERA_BACKEND_EXCEPTIONS_H
