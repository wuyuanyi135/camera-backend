//
// Created by wuyuanyi on 06/01/19.
//

#include <boost/make_shared.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

#include <cmath>

#include "server.h"
#include "error.h"
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
                                              mvcam::AdapterInfo *dest) {
  dest->set_description(src->description());
  dest->set_name(src->name());
  dest->set_in_use(true);
  dest->set_version(src->version());
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
                                                   std::vector<camera_driver::adapter *> adapters) {
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

void camera_backend_server::transform_device_info(camera_driver::camera_device &src,
                                                  mvcam::DeviceInfo *dest) {
  dest->set_id(src.camera_descriptor_ref.id);
  dest->set_version("N/A");
  dest->set_manufacture(src.camera_descriptor_ref.manufacture);
  dest->set_model(src.camera_descriptor_ref.model);
  dest->set_serial(src.camera_descriptor_ref.serial);
  dest->set_connected(src.opened());
  transform_adapter(src.adapter_ref, dest->mutable_adapter());
}


template<typename T>
void camera_backend_server::apply_parameter(camera_driver::camera_device &camera,
                                            camera_driver::parameter_write<T> &dest,
                                            const mvcam::Parameter &param,
                                            std::string fieldName
) {
  T &&value = param.value();
  dest.should_update = true;
  dest.value = value;
}
void camera_backend_server::configure_camera(camera_driver::camera_device &camera,
                                             const mvcam::ConfigureRequest *configuration) {
  if (!configuration->has_config()) {
    return;
  }

  camera_driver::camera_parameter_write internalConfiguration{};
  // configure gain
  const mvcam::Configuration &config = configuration->config();
  if (config.has_gain()) {
    apply_parameter(camera,
                    internalConfiguration.gain,
                    config.gain(),
                    "gain"
    );
  }

  if (config.has_exposure()) {
    apply_parameter(camera,
                    internalConfiguration.exposure,
                    config.exposure(),
                    "exposure"
    );
  }

  if (config.has_black_level()) {
    apply_parameter(camera,
                    internalConfiguration.black_level,
                    config.black_level(),
                    "black level"
    );
  }

  if (config.has_gamma()) {
    apply_parameter(camera,
                    internalConfiguration.gamma,
                    config.gamma(),
                    "gamma"
    );
  }

  if (config.has_frame_rate()) {
    apply_parameter(camera,
                    internalConfiguration.frame_rate,
                    config.frame_rate(),
                    "frame rate"
    );

  }

  // driver apply the parameters to camera;
  camera.set_configuration(internalConfiguration);
}

grpc::Status camera_backend_server::index_camera_call_wrapper(std::string id,
                                                              std::function<void(camera_driver::camera_device&)> callback) {
  // find the device from cache
  std::shared_ptr<camera_driver::camera_device> camera;
  try {
    camera = this->mFramework->query_by_id(id);
  } catch (boost::exception &ex) {
    return grpc::Status(grpc::NOT_FOUND, "Id query failed. Try invalidate the cache first.");
  }
  try {
    callback(*camera);
    return grpc::Status::OK;
  } catch (boost::exception &ex) {
    return grpc::Status(grpc::INTERNAL, boost::current_exception_diagnostic_information(true));
  }
}

void camera_backend_server::get_configuration_from_camera(camera_driver::camera_device &camera,
                                                          mvcam::Configuration *dest) {
  camera_driver::camera_parameter_read param{};
  camera.get_configuration(param);

  dest->mutable_black_level()->set_value(param.black_level.value);
  dest->mutable_black_level()->set_max(param.black_level.max);
  dest->mutable_black_level()->set_min(param.black_level.min);

  dest->mutable_gamma()->set_value(param.gamma.value);
  dest->mutable_gamma()->set_max(param.gamma.max);
  dest->mutable_gamma()->set_min(param.gamma.min);

  dest->mutable_exposure()->set_value(param.exposure.value);
  dest->mutable_exposure()->set_min(param.exposure.min);
  dest->mutable_exposure()->set_max(param.exposure.max);

  dest->mutable_frame_rate()->set_value(param.frame_rate.value);
  dest->mutable_frame_rate()->set_min(param.frame_rate.min);
  dest->mutable_frame_rate()->set_max(param.frame_rate.max);

  dest->mutable_gain()->set_value(param.gain.value);
  dest->mutable_gain()->set_max(param.gain.max);
  dest->mutable_gain()->set_min(param.gain.min);
//  dest->mutable_frame_number()->set_value(param.frame_number.value);
}

void camera_backend_server::get_status_from_camera(camera_driver::camera_device &camera, mvcam::Status *dest) {
  camera_driver::status status{};
  if (camera.opened()) {
    camera.get_status(status);
    dest->set_temperature(status.temperature);
  } else {
    dest->set_temperature(std::nan(""));
  }

  dest->set_capturing(camera.capturing());
  dest->set_opened(camera.opened());

}

void camera_backend_server::transform_frame(const camera_driver::frame &frame, mvcam::Frame *dest) {
  dest->set_id(frame.id);
  dest->set_pixel_format(frame.pixel_format);
  dest->set_height(frame.height);
  dest->set_width(frame.width);
  dest->set_size(frame.size);
  dest->mutable_timestamp()->set_seconds(frame.time_stamp);
  dest->mutable_timestamp()->set_nanos(0);
  dest->set_data(frame.data, frame.size);
}

