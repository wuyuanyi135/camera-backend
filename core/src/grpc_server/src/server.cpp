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
#include "capability.h"
#include "logging.h"
#include "config.h"

grpc::Status camera_backend_server::GetAvailableAdapters(::grpc::ServerContext *context,
                                                         const ::google::protobuf::Empty *request,
                                                         mvcam::AvailableAdaptersResponse *response) {
  for (auto &adapter : mFramework->adapters()) {
    transform_adapter(adapter, response->add_adapters());
  }

  return grpc::Status::OK;
}
grpc::Status camera_backend_server::GetDevices(::grpc::ServerContext *context,
                                               const mvcam::AdapterRequest *request,
                                               mvcam::DeviceListResponse *response) {
  try {
    std::vector<camera_driver::adapter *> adapter(mFramework->adapters());
    if (!request->name().empty()) {
      filter_adapter_by_name(request->name(), adapter);
    }

    update_id_index(adapter);

    for (const std::pair<std::string, camera_driver::camera_container> it: mCameraCache) {
      mvcam::DeviceInfo *pInfo = response->add_devices();
      transform_device_info(it.second, pInfo);
    }
    return grpc::Status::OK;
  } catch (boost::exception &ex) {
    return grpc::Status(grpc::INTERNAL, boost::current_exception_diagnostic_information(true));
  }
}

grpc::Status camera_backend_server::QueryDeviceById(::grpc::ServerContext *context,
                                                    const mvcam::IdRequest *request,
                                                    mvcam::DeviceInfo *response) {
  // find the device from cache
  return index_camera_call_wrapper(
      request->id(),
      [response, this](camera_driver::camera_container &container) {
        transform_device_info(container, response);
      });
}

grpc::Status camera_backend_server::OpenCamera(::grpc::ServerContext *context,
                                               const mvcam::IdRequest *request,
                                               google::protobuf::Empty *response) {
  return index_camera_call_wrapper(request->id(), [](camera_driver::camera_container &container) {
    // Guarded by can_open capability
    if (!container.device->capabilities()->should_open) {
      camera_capability_error ex(container.camera_descriptor);
//      ex << error_info("No capability: can_open");
      BOOST_THROW_EXCEPTION(ex);
    }
    container.device->open_camera();
  });
}

grpc::Status camera_backend_server::ShutdownCamera(::grpc::ServerContext *context,
                                                   const mvcam::IdRequest *request,
                                                   ::google::protobuf::Empty *response) {
  return index_camera_call_wrapper(request->id(), [](camera_driver::camera_container &container) {
    if (!container.device->capabilities()->can_shutdown) {
      camera_capability_error ex(container.camera_descriptor);
      ex << error_info("No capability: can_shutdown");
      BOOST_THROW_EXCEPTION(ex);
    }
    container.device->shutdown_camera();
  });
}

grpc::Status camera_backend_server::ConfigureCamera(::grpc::ServerContext *context,
                                                    const mvcam::ConfigureRequest *request,
                                                    ::google::protobuf::Empty *response) {
  return index_camera_call_wrapper(request->id().id(), [this, request](camera_driver::camera_container &container) {
    configure_camera(container, request);
  });
}

grpc::Status camera_backend_server::GetConfiguration(::grpc::ServerContext *context,
                                                     const mvcam::IdRequest *request,
                                                     mvcam::Configuration *response) {
  return index_camera_call_wrapper(request->id(), [this, response](camera_driver::camera_container &container) {
    get_configuration_from_camera(container, response);
  });
}

grpc::Status camera_backend_server::GetStatus(::grpc::ServerContext *context,
                                              const mvcam::IdRequest *request,
                                              mvcam::Status *response) {
  return index_camera_call_wrapper(request->id(), [this, response](camera_driver::camera_container &container) {
    get_status_from_camera(container, response);
  });
}

grpc::Status camera_backend_server::Capture(::grpc::ServerContext *context,
                                            const ::mvcam::IdRequest *request,
                                            ::mvcam::Frame *response) {
  return index_camera_call_wrapper(request->id(), [this, response](camera_driver::camera_container &container) {
    if (!container.device->capabilities()->can_capture) {
      camera_capability_error ex(container.camera_descriptor);
      ex << error_info("No capability: can_capture");
      BOOST_THROW_EXCEPTION(ex);
    }

    try {
      camera_driver::frame frame;
      container.device->capture(frame);
      transform_frame(frame, response);
    } catch (boost::exception &ex) {
      throw;
    }
  });
}

grpc::Status camera_backend_server::Streaming(::grpc::ServerContext *context,
                                              const ::mvcam::StreamingRequest *request,
                                              ::grpc::ServerWriter<::mvcam::FrameStream> *writer) {
  mvcam::FrameStream fs;
  std::string id = request->id().id();

  camera_driver::camera_container container;
  if (mCameraCache.find(id) != mCameraCache.end()) {
    // found camera
    container = mCameraCache[id];
  } else {
    return grpc::Status(grpc::NOT_FOUND, "Camera not found");
  }
  if (!container.device->capabilities()->can_capture_async) {
    return grpc::Status(grpc::UNAVAILABLE, "No capability: can_capture_async");
  }

  if (container.device->capturing()) {
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
  container.device->set_configuration(param);

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
    container.device->capture_async([&ioService, &fs, &writeIndex, &writtenFrames, batchSize, this, writer](
        camera_driver::frame &frame) {
      ioService.post([frame
          {std::move(frame)}, &fs, &writeIndex, &writtenFrames, batchSize, this, writer]() {
        if (batchSize <= 1) {
          transform_frame(frame, fs.mutable_frames()->Mutable(0));
          writer->Write(fs);
          writtenFrames += batchSize;
        } else {
          transform_frame(frame, fs.mutable_frames()->Mutable(writeIndex++));

          // check whether batch size is fulfilled
          if (writeIndex >= batchSize) {
            writeIndex = 0;
            writer->Write(fs);
            writtenFrames += batchSize;
          }
        }
      });
    });
  }
  catch (boost::exception &ex) {
    container.device->stop_capture_async();
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

  container.device->stop_capture_async();
  future.wait();
  return grpc::Status::OK;
}

grpc::Status camera_backend_server::ResetDevice(::grpc::ServerContext *context,
                                                const ::mvcam::IdRequest *request,
                                                ::google::protobuf::Empty *response) {

  return index_camera_call_wrapper(request->id(), [this, response](camera_driver::camera_container &container) {
    if (!container.device->capabilities()->can_reset) {
      camera_capability_error ex(container.camera_descriptor);
      ex << error_info("No capability: can_reset");
      BOOST_THROW_EXCEPTION(ex);
    }

    try {
      container.device->reset();
    } catch (boost::exception &ex) {
      throw;
    }
  });
}

grpc::Status camera_backend_server::ControlDeviceState(::grpc::ServerContext *context,
                                                       const ::mvcam::DeviceControlRequest *request,
                                                       ::google::protobuf::Empty *response) {
  return index_camera_call_wrapper(request->id().id(), [this, response](camera_driver::camera_container &container) {
    if (!container.device->capabilities()->can_suspend) {
      camera_capability_error ex(container.camera_descriptor);
      ex << error_info("No capability: can_suspend");
      BOOST_THROW_EXCEPTION(ex);
    }

    try {
      container.device->suspend();
    } catch (boost::exception &ex) {
      throw;
    }
  });
}

