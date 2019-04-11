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
  /// list camera and update cache if any
  /// The same device must be returned in the same shared_ptr. If it is removed, the shared_ptr should be reset. Next time the camera does not have to been in the same pointer.
  /// The derived camera class can implement some static function to provide the camera single instance. The camera instance will not be directly created by framework so as long
  /// as the adapter is able to create the instance, it will be fine.
  /// \param cameraList
  virtual void camera_list(std::vector<camera_descriptor> &cameraList) = 0;

 public:
  /// the session initialization should be in the constructor.  Throw exception if the adapter
  ///  failed to start
  adapter() = default;
  virtual ~adapter() {};

 public:
  /// Create camera with given id. if the creation fails, throw error.
  /// \param cd query id
  /// \param descriptor output variable
  /// \return
  virtual std::shared_ptr<camera_driver::camera_device> create_camera(camera_descriptor &cd) = 0;
};
}
#endif //CAMERA_BACKEND_ADAPTER_H
