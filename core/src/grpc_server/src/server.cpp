//
// Created by wuyuanyi on 01/01/19.
//

#include <boost/make_shared.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/asio.hpp>
#include <future>
#include <grpc++/server_builder.h>

#include "server.h"
#include "error.h"
#include "logging.h"
#include "config.h"

# define OPEN_GUARD if(!mFramework->selected_camera) {\
return grpc::Status(grpc::StatusCode::UNAVAILABLE, std::string("Camera not selected")); \
}\
if (!mFramework->selected_camera->opened()) { \
return grpc::Status(grpc::StatusCode::UNAVAILABLE, std::string("Camera not opened")); \
}

grpc::Status camera_backend_server::GetDevices(::grpc::ServerContext *context,
                                               const ::mvcam::GetDevicesReq *request,
                                               ::mvcam::GetDevicesRes *response) {
  for (auto &it: mFramework->camera_list()) {
    mvcam::DeviceInfo *pInfo = response->add_devices();
    transform_device_info(it, pInfo);
  }

  return grpc::Status::OK;
}

grpc::Status camera_backend_server::OpenCamera(::grpc::ServerContext *context,
                                               const ::mvcam::OpenCameraReq *request,
                                               ::mvcam::OpenCameraRes *response) {
  try {
    mFramework->select_camera_by_id(request->id());
    mFramework->selected_camera->open_camera();
  } catch (std::exception &ex) {
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Failed to open camera: ") + ex.what());
  }
  return grpc::Status::OK;
}

grpc::Status camera_backend_server::ShutdownCamera(::grpc::ServerContext *context,
                                                   const ::mvcam::ShutdownCameraReq *request,
                                                   ::mvcam::ShutdownCameraRes *response) {
  try {
    mFramework->selected_camera->shutdown_camera();
  } catch (std::exception &ex) {
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Failed to shutdown camera: ") + ex.what());
  }
  return grpc::Status::OK;
}

grpc::Status camera_backend_server::Opened(::grpc::ServerContext *context,
                                           const ::mvcam::OpenedReq *request,
                                           ::mvcam::OpenedRes *response) {
  if (!mFramework->selected_camera) {
    response->set_opened(false);
    return grpc::Status::OK;
  }
  try {
    response->set_opened(mFramework->selected_camera->opened());
  } catch (std::exception& ex) {
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string(std::string("Failed to get camera open status: ") + ex.what()));
  }

  return grpc::Status::OK;
}

grpc::Status camera_backend_server::Capturing(::grpc::ServerContext *context,
                                              const ::mvcam::CapturingReq *request,
                                              ::mvcam::CapturingRes *response) {
  if (!mFramework->selected_camera) {
    return grpc::Status(grpc::StatusCode::UNAVAILABLE, std::string("Camera not selected"));
  }
  OPEN_GUARD;
  response->set_capturing(mFramework->selected_camera->capturing());
  return grpc::Status::OK;
}

grpc::Status camera_backend_server::ConfigureCamera(::grpc::ServerContext *context,
                                                    const ::mvcam::ConfigureCameraReq *request,
                                                    ::mvcam::ConfigureCameraRes *response) {
  OPEN_GUARD
  try {
    configure_camera(*mFramework->selected_camera, request);
  } catch (std::exception &ex) {
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Failed to configure camera: ") + ex.what());
  }
  return grpc::Status::OK;
}
grpc::Status camera_backend_server::GetConfiguration(::grpc::ServerContext *context,
                                                     const ::mvcam::GetConfigureReq *request,
                                                     ::mvcam::GetConfigurationRes *response) {
  OPEN_GUARD
  try {
    get_configuration_from_camera(*mFramework->selected_camera, response->mutable_config());
  } catch (std::exception &ex) {
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Failed to get configuration: ") + ex.what());
  }
  return grpc::Status::OK;
}
grpc::Status camera_backend_server::GetStatus(::grpc::ServerContext *context,
                                              const ::mvcam::GetStatusReq *request,
                                              ::mvcam::GetStatusRes *response) {
  OPEN_GUARD
  try {
    get_status_from_camera(*mFramework->selected_camera, response->mutable_status());
  } catch (std::exception &ex) {
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Failed to get status: ") + ex.what());
  }
  return grpc::Status::OK;
}
grpc::Status camera_backend_server::Capture(::grpc::ServerContext *context,
                                            const ::mvcam::CaptureReq *request,
                                            ::mvcam::CaptureRes *response) {
  OPEN_GUARD
  try {
    camera_driver::frame frame;
    mFramework->selected_camera->capture(frame);
    transform_frame(frame, response->mutable_frame());
  } catch (std::exception &ex) {
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Failed to capture frame: ") + ex.what());
  }
  return grpc::Status::OK;
}
grpc::Status camera_backend_server::Streaming(::grpc::ServerContext *context,
                                              const ::mvcam::StreamingReq *request,
                                              ::grpc::ServerWriter<::mvcam::FrameStream> *writer) {
  OPEN_GUARD
  mvcam::FrameStream fs;
  std::shared_ptr<camera_driver::camera_device> camera = mFramework->selected_camera;

  if (camera->capturing()) {
    const char *const errMsg = "Cannot start streaming: already capturing";
    CDERROR(errMsg);
    return grpc::Status(grpc::UNAVAILABLE, errMsg);
  }

  const google::protobuf::uint64 batchSize = request->batch_size();
  const google::protobuf::uint64 numberFrames = request->number_frames();

  // change the camera property continuous capture configuration
  camera_driver::camera_parameter_write param{};
  param.frame_number.value = numberFrames;
  param.frame_number.should_update = true;
  camera->set_configuration(param);

  // prepare the send buffer
  if (batchSize <= 1) {
    // no-cache frame sending
    fs.mutable_frames()->Add();
  } else {
    for (int i = 0; i < batchSize; ++i) {
      fs.mutable_frames()->Add();
    }
  }

  boost::asio::io_service ioService;
  boost::asio::io_service::work work(ioService);  // prevent ioService.run exit

  int writeIndex = 0;
  int writtenFrames = 0;
  try {
    camera->capture_async([&ioService, &fs, &writeIndex, &writtenFrames, batchSize, this, writer]
                              (camera_driver::frame &frame) {
      // when new frame arrives, post a new job.
      ioService.post([frame
          {std::move(frame)}, &fs, &writeIndex, &writtenFrames, batchSize, this, writer, &ioService]() {
        if (batchSize <= 1) {
          transform_frame(frame, fs.mutable_frames()->Mutable(0));
          if (!writer->Write(fs)) {
            ioService.stop();
            return;
          }
          writtenFrames += batchSize;
        } else {
          transform_frame(frame, fs.mutable_frames()->Mutable(writeIndex++));

          // check whether batch size is fulfilled
          if (writeIndex >= batchSize) {
            writeIndex = 0;
            if (!writer->Write(fs)) {
              ioService.stop();
              return;
            }
            writtenFrames += batchSize;
          }
        }
      });
    });
  }
  catch (boost::exception &ex) {
    camera->stop_capture_async();
    return grpc::Status(grpc::INTERNAL, boost::current_exception_diagnostic_information(true));
  }

  const std::future<void> &future = std::async([&context, &ioService]() {
    while (!context->IsCancelled() && !ioService.stopped()) {
      // this thread must exit to make the server end the stream.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    };
    if (!ioService.stopped()) {
      ioService.stop();
    }
  });

  while (!ioService.stopped()) {
    ioService.run_one();
    if (writtenFrames >= numberFrames && numberFrames != 0) {
      // with frame limit and exceeded.
      ioService.stop();
    }
  }

  camera->stop_capture_async();
  future.wait();
  return grpc::Status::OK;
}

grpc::Status camera_backend_server::WorkingStateStreaming(::grpc::ServerContext *context,
                                                          const ::mvcam::WorkingStateStreamingReq *request,
                                                          ::grpc::ServerWriter<::mvcam::WorkingStateStream> *writer) {
  OPEN_GUARD
  std::shared_ptr<camera_driver::camera_device> camera = mFramework->selected_camera;

  boost::asio::io_service ioService;
  boost::asio::io_service::work work(ioService);  // prevent ioService.run exit

  const std::future<void> &future = std::async([&context, &ioService, this, writer]() {
    while (!context->IsCancelled() && !ioService.stopped()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(config_provider::get_instance()->read(
          "working_state_check_interval_ms",
          1000)));

      // check state and send
      bool opened = mFramework->selected_camera->opened();
      bool capturing = mFramework->selected_camera->capturing();

      if (this->currentOpened != opened || this->currentCapturing != capturing) {
        this->currentCapturing = capturing;
        this->currentOpened = opened;
        mvcam::WorkingStateStream ws;
        ws.set_capturing(capturing);
        ws.set_opened(opened);
        if(!writer->Write(ws)) {
          ioService.stop();
          return;
        }
      }
    }
  });

  while (!ioService.stopped()) {
    ioService.run_one();
  }
  future.wait();
  return grpc::Status::OK;
}

grpc::Status camera_backend_server::ResetDevice(::grpc::ServerContext *context,
                                                const ::mvcam::ResetDeviceReq *request,
                                                ::mvcam::ResetDeviceRes *response) {
  OPEN_GUARD
  try {
    mFramework->selected_camera->reset();
  } catch (std::exception &ex) {
    return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Failed to reset: ") + ex.what());
  }
  return grpc::Status::OK;
}

grpc::Status camera_backend_server::ControlDeviceState(::grpc::ServerContext *context,
                                                       const ::mvcam::ControlDeviceStateReq *request,
                                                       ::mvcam::ControlDeviceStateRes *response) {
  OPEN_GUARD
  return Service::ControlDeviceState(context, request, response);
}

grpc::Status camera_backend_server::GetAdapter(::grpc::ServerContext *context,
                                               const ::mvcam::GetAdapterReq *request,
                                               ::mvcam::GetAdapterRes *response) {
  transform_adapter(mFramework->get_adapter(), response->mutable_adapter());
  return grpc::Status::OK;
}


