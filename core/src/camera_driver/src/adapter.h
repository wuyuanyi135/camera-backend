//
// Created by wuyuanyi on 19/11/18.
//

#ifndef CAMERA_BACKEND_ADAPTER_H
#define CAMERA_BACKEND_ADAPTER_H
#include "string"
#include "vector"
#include "camera_descriptor.h"
namespace camera_driver {
// Adapter should only create camera. Framework will keep the copy and cache the cameras
class adapter {
 public:
  virtual std::string description() const = 0;
  virtual std::string name() const = 0;
  virtual std::string version() const = 0;


  /// Camera functions
 public:
  virtual void camera_list(std::vector<camera_descriptor> &cameraList) = 0;

 public:
  /// the session initialization should be in the constructor.  Throw exception if the adapter
  ///  failed to start
  adapter() = default;
  virtual ~adapter() {};

 public:
  /// Create camera with given id. if the creation fails, throw error. The driver should handle if the instance cannot be re-created.
  /// \param cd query id
  /// \param descriptor output variable
  /// \return
  virtual std::shared_ptr<camera_driver::camera_device> create_camera(std::string id) = 0;
};
}
#endif //CAMERA_BACKEND_ADAPTER_H
