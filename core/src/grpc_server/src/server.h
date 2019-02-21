//
// Created by wuyuanyi on 01/01/19.
//

#ifndef CAMERA_BACKEND_SERVER_H
#define CAMERA_BACKEND_SERVER_H
#include <vector>
#include <grpc++/grpc++.h>
#include "protos/camera_service.grpc.pb.h"
#include "adapter.h"
#include "camera_parameter.h"
#include "frame.h"
#include "framework.h"

class camera_backend_server : public mvcam::MicroVisionCameraService::Service {
 private:
  std::shared_ptr<camera_driver::framework> mFramework;
  void load_adapters();

  // modify this function to register new capabilities
  void transform_adapter(camera_driver::adapter *src,
                         mvcam::AdapterInfo *dest);
  void transform_device_info(camera_driver::camera_device &src, mvcam::DeviceInfo *dest);
  void transform_frame(const camera_driver::frame &frame, mvcam::Frame *dest);


  /// modify when new parameters are added
  /// \param camera
  /// \param dest
  void get_configuration_from_camera(camera_driver::camera_device &camera, mvcam::Configuration *dest);
  void configure_camera(camera_driver::camera_device &camera, const mvcam::ConfigureRequest *configuration);

  /// modify when new status is added
  /// \param camera
  /// \param dest
  void get_status_from_camera(camera_driver::camera_device &camera, mvcam::Status *dest);


 private:
  void filter_adapter_by_name(const std::string &name, std::vector<camera_driver::adapter*> adapter);

  template<typename T>
  void apply_parameter(camera_driver::camera_device &camera,
                       camera_driver::parameter_write<T> &dest,
                       const mvcam::Parameter &param,
                       std::string fieldName,
                       bool capability
  );


  /// wrapper function providing the callback function the camera container corresponding to the id. If id does not exist, throw exception.
  //  In callback, throw exception to raise a grpc error status.
  /// \param id
  /// \param callback
  /// \return
  grpc::Status index_camera_call_wrapper(std::string id,
                                         std::function<void(camera_driver::camera_device&)> callback);
 public:
  static std::unique_ptr<grpc::Server> start_server();

 public:
  camera_backend_server();
  ~camera_backend_server() override;
  grpc::Status GetAvailableAdapters(::grpc::ServerContext *context,
                                    const ::google::protobuf::Empty *request,
                                    mvcam::AvailableAdaptersResponse *response) override;
  grpc::Status GetDevices(::grpc::ServerContext *context,
                          const mvcam::AdapterRequest *request,
                          mvcam::DeviceListResponse *response) override;
  grpc::Status QueryDeviceById(::grpc::ServerContext *context,
                               const mvcam::IdRequest *request,
                               mvcam::DeviceInfo *response) override;
  grpc::Status OpenCamera(::grpc::ServerContext *context,
                          const mvcam::IdRequest *request,
                          ::google::protobuf::Empty *response) override;
  grpc::Status ShutdownCamera(::grpc::ServerContext *context,
                              const mvcam::IdRequest *request,
                              ::google::protobuf::Empty *response) override;
  grpc::Status ConfigureCamera(::grpc::ServerContext *context,
                               const mvcam::ConfigureRequest *request,
                               ::google::protobuf::Empty *response) override;
  grpc::Status GetConfiguration(::grpc::ServerContext *context,
                                const mvcam::IdRequest *request,
                                mvcam::Configuration *response) override;
  grpc::Status ResetDevice(::grpc::ServerContext *context,
                           const ::mvcam::IdRequest *request,
                           ::google::protobuf::Empty *response) override;
  grpc::Status ControlDeviceState(::grpc::ServerContext *context,
                                  const ::mvcam::DeviceControlRequest *request,
                                  ::google::protobuf::Empty *response) override;
  /// This function does not check capabilities. Front end should decide whether the status entry should be displayed or not.
  /// \param context
  /// \param request
  /// \param response
  /// \return
  grpc::Status GetStatus(::grpc::ServerContext *context,
                         const mvcam::IdRequest *request,
                         mvcam::Status *response) override;
  grpc::Status Capture(::grpc::ServerContext *context,
                       const ::mvcam::IdRequest *request,
                       ::mvcam::Frame *response) override;
  grpc::Status Streaming(::grpc::ServerContext *context,
                         const ::mvcam::StreamingRequest *request,
                         ::grpc::ServerWriter<::mvcam::FrameStream> *writer) override;
  void transform_device_capabilities(camera_driver::camera_capability *src, mvcam::CameraCapability *dest) const;
};
#endif //CAMERA_BACKEND_SERVER_H
