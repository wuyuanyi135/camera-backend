//
// Created by wuyuanyi on 06/01/19.
//

#include <boost/make_shared.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

#include <cmath>

#include "server.h"
#include "error.h"
#include "capability.h"
#include "logging.h"
#include "config.h"

camera_backend_server::camera_backend_server() : Service() {
  load_adapters();
}
camera_backend_server::~camera_backend_server() {

}

void camera_backend_server::load_adapters() {
  mFramework = camera_driver::framework::get_instance();
  CDINFO("Adapter framework loaded");
}

void camera_backend_server::transform_adapter(camera_driver::adapter *src,
                                              CameraServer::AdapterInfo *dest) {
  dest->set_description(src->description());
  dest->set_name(src->name());
  dest->set_in_use(true);
  dest->set_version(src->version());
  camera_driver::adapter_capability *adapterCapability = src->capabilities();
  CameraServer::AdapterCapability *capability = dest->mutable_capability();
  capability->set_should_shut_down(adapterCapability->should_shutdown);
}

std::unique_ptr<grpc::Server> camera_backend_server::start_server() {
  std::string serverAddress
      (config_provider::get_instance()->read("listen_ip") + ":" + config_provider::get_instance()->read("listen_port"));
  static camera_backend_server service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(serverAddress, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  return server;
}
void camera_backend_server::filter_adapter_by_name(const std::string &name,
                                                   std::vector<camera_driver::adapter*> adapters) {
  for (auto it: mFramework->adapters()) {
    if (it->name() == name) {
      adapters.emplace_back(it);
      return;
    }
  }
  adapter_not_found_error ex;
  ex << error_info(std::string("By name: ") + name);
  BOOST_THROW_EXCEPTION(ex);
}

void camera_backend_server::update_id_index(std::vector<camera_driver::adapter *> &vector) {
  mCameraCache.clear();

  for (auto &it : vector) {
    std::vector<camera_driver::camera_descriptor> camera;
    it->camera_list(camera);
    for (auto &cam : camera) {

      camera_driver::camera_container container;
      bool found = it->get_camera_by_id(cam.id, container);
      assert(found); // camera should be found by its id.

      std::pair<std::string, camera_driver::camera_container> pair(cam.id, container);
      mCameraCache.insert(pair);
      CDINFO("Inserting camera: " << cam.id);
    }
  }
}
void camera_backend_server::transform_device_info(const camera_driver::camera_container &src,
                                                  CameraServer::DeviceInfo *dest) {
  dest->set_id(src.camera_descriptor.id);
  dest->set_version("N/A");
  dest->set_manufacture(src.camera_descriptor.manufacture);
  dest->set_model(src.camera_descriptor.model);
  dest->set_serial(src.camera_descriptor.serial);
  transform_adapter(src.adapter, dest->mutable_adapter());
  transform_device_capabilities(src.device->capabilities(), dest->mutable_capabilities());
}
void camera_backend_server::transform_device_capabilities(const camera_driver::camera_capability *src,
                                                          CameraServer::CameraCapability *dest) const {
  dest->set_can_shutdown(src->can_shutdown);
  dest->set_can_open(src->should_open);
  dest->set_can_capture_async(src->can_capture_async);
  dest->set_can_capture(src->can_capture);

  dest->set_can_adjust_exposure(src->can_adjust_exposure);

  dest->set_can_adjust_gain(src->can_adjust_gain);

  dest->set_can_adjust_gamma(src->can_adjust_gamma);
  dest->set_can_adjust_black_level(src->can_adjust_black_level);
  dest->set_can_adjust_frame_rate(src->can_adjust_frame_rate);
  dest->set_can_set_frame_number(src->can_set_frame_number);
  dest->set_can_get_temperature(src->can_get_temperature);
  dest->set_can_suspend(src->can_suspend);
  dest->set_can_reset(src->can_reset);
}

template<typename T>
void camera_backend_server::apply_parameter(camera_driver::camera_container &container,
                                            camera_driver::parameter_write<T> &dest,
                                            const CameraServer::Parameter &param,
                                            std::string fieldName,
                                            bool capability
) {
  if (param.should_update()) {
    if (!capability) {
      camera_capability_error ex(container.camera_descriptor);
      ex << error_info(std::string("No capability for setting ") + fieldName);
      BOOST_THROW_EXCEPTION(ex);
    }
    T &&value = param.value();
    dest.should_update = true;
    dest.value = value;
  }
}
void camera_backend_server::configure_camera(camera_driver::camera_container &container,
                                             const CameraServer::ConfigureRequest *configuration) {
  if (!configuration->has_config()) {
    return;
  }

  camera_driver::camera_parameter_write internalConfiguration{};
  camera_driver::camera_capability *cap = container.device->capabilities();
  // configure gain
  const CameraServer::Configuration &config = configuration->config();
  if (config.has_gain()) {
    apply_parameter(container,
                    internalConfiguration.gain,
                    config.gain(),
                    "gain",
                    cap->can_adjust_gain
    );
  }

  if (config.has_exposure()) {
    apply_parameter(container,
                    internalConfiguration.exposure,
                    config.exposure(),
                    "exposure",
                    cap->can_adjust_exposure
    );
  }

  if (config.has_black_level()) {
    apply_parameter(container,
                    internalConfiguration.black_level,
                    config.black_level(),
                    "black level",
                    cap->can_adjust_black_level
    );
  }

  if (config.has_gamma()) {
    apply_parameter(container,
                    internalConfiguration.gamma,
                    config.gamma(),
                    "gamma",
                    cap->can_adjust_gamma
    );
  }

  if (config.has_frame_rate()) {
    apply_parameter(container,
                    internalConfiguration.frame_rate,
                    config.frame_rate(),
                    "frame rate",
                    cap->can_adjust_frame_rate
    );

  }

  // driver apply the parameters to camera;
  container.device->set_configuration(internalConfiguration);
}

grpc::Status camera_backend_server::index_camera_call_wrapper(std::string id,
                                                              std::function<void(camera_driver::camera_container & )> callback) {
  // find the device from cache
  if (mCameraCache.find(id) != mCameraCache.end()) {
    // found
    camera_driver::camera_container &container = mCameraCache[id];

    try {
      callback(container);
      return grpc::Status::OK;
    } catch (boost::exception &ex) {
      return grpc::Status(grpc::INTERNAL, boost::current_exception_diagnostic_information(true));
    }
  } else {
    return grpc::Status(grpc::NOT_FOUND, "Id query failed. Try invalidate the cache first.");
  }
}
void camera_backend_server::get_configuration_from_camera(camera_driver::camera_container &container,
                                                          CameraServer::Configuration *dest) {
  camera_driver::camera_parameter_read param{};
  container.device->get_configuration(param);

  dest->mutable_black_level()->set_value(param.black_level.value);
  dest->mutable_gamma()->set_value(param.gamma.value);
  dest->mutable_exposure()->set_value(param.exposure.value);
  dest->mutable_frame_rate()->set_value(param.frame_rate.value);
  dest->mutable_gain()->set_value(param.gain.value);
//  dest->mutable_frame_number()->set_value(param.frame_number.value);
}

void camera_backend_server::get_status_from_camera(camera_driver::camera_container &container, CameraServer::Status *dest) {
  camera_driver::status status{};
  if(container.device->opened()) {
    container.device->get_status(status);
    dest->set_temperature(status.temperature);
  } else {
    dest->set_temperature(std::nan(""));
  }

  dest->set_capturing(container.device->capturing());
  dest->set_opened(container.device->opened());

}

void camera_backend_server::transform_frame(const camera_driver::frame &frame, CameraServer::Frame *dest) {
  dest->set_id(frame.id);
  dest->set_pixel_format(frame.pixel_format);
  dest->set_height(frame.height);
  dest->set_width(frame.width);
  dest->set_size(frame.size);
  dest->mutable_timestamp()->set_seconds(frame.time_stamp);
  dest->mutable_timestamp()->set_nanos(0);
  dest->set_data(frame.data, frame.size);
}

