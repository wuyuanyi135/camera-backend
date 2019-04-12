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
  bool currentOpened = false;
  bool currentCapturing = false;
 private:
  std::shared_ptr<camera_driver::framework> mFramework;
  void load_adapters();

  void transform_adapter(camera_driver::adapter *src,
                         mvcam::AdapterInfo *dest);
  void transform_device_info(camera_driver::camera_descriptor &src, mvcam::DeviceInfo *dest);
  void transform_frame(const camera_driver::frame &frame, mvcam::Frame *dest);


  /// modify when new parameters are added
  /// \param camera
  /// \param dest
  void get_configuration_from_camera(camera_driver::camera_device &camera, mvcam::Configuration *dest);
  void configure_camera(camera_driver::camera_device &camera, const mvcam::ConfigureCameraReq *configuration);

  /// modify when new status is added
  /// \param camera
  /// \param dest
  void get_status_from_camera(camera_driver::camera_device &camera, mvcam::Status *dest);


 private:

  template<typename T>
  void apply_parameter(camera_driver::camera_device &camera,
                       camera_driver::parameter_write<T> &dest,
                       const mvcam::Parameter &param,
                       std::string fieldName
  );

 public:
  static std::unique_ptr<grpc::Server> start_server();

 public:
  camera_backend_server();
  ~camera_backend_server() override;
  grpc::Status GetAdapter(::grpc::ServerContext *context,
                          const ::mvcam::GetAdapterReq *request,
                          ::mvcam::GetAdapterRes *response) override;
  grpc::Status GetDevices(::grpc::ServerContext *context,
                          const ::mvcam::GetDevicesReq *request,
                          ::mvcam::GetDevicesRes *response) override;
  grpc::Status OpenCamera(::grpc::ServerContext *context,
                          const ::mvcam::OpenCameraReq *request,
                          ::mvcam::OpenCameraRes *response) override;
  grpc::Status ShutdownCamera(::grpc::ServerContext *context,
                              const ::mvcam::ShutdownCameraReq *request,
                              ::mvcam::ShutdownCameraRes *response) override;
  grpc::Status Opened(::grpc::ServerContext *context,
                      const ::mvcam::OpenedReq *request,
                      ::mvcam::OpenedRes *response) override;
  grpc::Status Capturing(::grpc::ServerContext *context,
                         const ::mvcam::CapturingReq *request,
                         ::mvcam::CapturingRes *response) override;
  grpc::Status ConfigureCamera(::grpc::ServerContext *context,
                               const ::mvcam::ConfigureCameraReq *request,
                               ::mvcam::ConfigureCameraRes *response) override;
  grpc::Status GetConfiguration(::grpc::ServerContext *context,
                                const ::mvcam::GetConfigureReq *request,
                                ::mvcam::GetConfigurationRes *response) override;
  grpc::Status GetStatus(::grpc::ServerContext *context,
                         const ::mvcam::GetStatusReq *request,
                         ::mvcam::GetStatusRes *response) override;
  grpc::Status Capture(::grpc::ServerContext *context,
                       const ::mvcam::CaptureReq *request,
                       ::mvcam::CaptureRes *response) override;
  grpc::Status Streaming(::grpc::ServerContext *context,
                         const ::mvcam::StreamingReq *request,
                         ::grpc::ServerWriter<::mvcam::FrameStream> *writer) override;
  grpc::Status WorkingStateStreaming(::grpc::ServerContext *context,
                                     const ::mvcam::WorkingStateStreamingReq *request,
                                     ::grpc::ServerWriter<::mvcam::WorkingStateStream> *writer) override;
  grpc::Status ResetDevice(::grpc::ServerContext *context,
                           const ::mvcam::ResetDeviceReq *request,
                           ::mvcam::ResetDeviceRes *response) override;
  grpc::Status ControlDeviceState(::grpc::ServerContext *context,
                                  const ::mvcam::ControlDeviceStateReq *request,
                                  ::mvcam::ControlDeviceStateRes *response) override;

};
#endif //CAMERA_BACKEND_SERVER_H
