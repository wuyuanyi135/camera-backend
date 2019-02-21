#include <memory>

//
// Created by wuyuanyi on 09/01/19.
//

#include "aravis_camera.h"
#include "logging.h"

namespace aravis_camera_driver {

void aravis_camera::feature_guard(std::string &fieldName) const {
  if (!arv_device_get_feature(mDevice, fieldName.c_str())) {
    camera_driver::parameter_not_supported_error ex(this, fieldName);
    BOOST_THROW_EXCEPTION(ex);
  }
}

void aravis_camera::field_bounds_internal(const char *feature, double &min, double &max) {
  arv_device_get_float_feature_bounds(mDevice, feature, &min, &max);
}
void aravis_camera::field_bounds_internal(const char *feature, long &min, long &max) {
  arv_device_get_integer_feature_bounds(mDevice, feature, &min, &max);
}

void aravis_camera::check_status() {
  ArvDeviceStatus status = arv_device_get_status(mDevice);
  if (status == ARV_DEVICE_STATUS_WRITE_ERROR || status == ARV_DEVICE_STATUS_TRANSFER_ERROR || status == ARV_DEVICE_STATUS_TIMEOUT) {
    camera_driver::write_parameter_error ex(this, status);
    BOOST_THROW_EXCEPTION(ex);
  }
  if (status == ARV_DEVICE_STATUS_UNKNOWN || status == ARV_DEVICE_STATUS_NOT_CONNECTED) {
    camera_driver::invalid_camera_error ex(this);
    BOOST_THROW_EXCEPTION(ex);
  }
}

void aravis_camera::set_field_internal(const char *feature, double value) {
  arv_device_set_float_feature_value(mDevice, feature, value);
  check_status();
}

void aravis_camera::set_field_internal(const char *feature, long value) {
  arv_device_set_integer_feature_value(mDevice, feature, value);
  check_status();
}
void aravis_camera::set_field_internal(const char *feature, std::string &value) {
  arv_device_set_string_feature_value(mDevice, feature, value.c_str());
  check_status();
}
void aravis_camera::set_field_internal(const char *feature, const char *value) {
  arv_device_set_string_feature_value(mDevice, feature, value);
  check_status();
}
void aravis_camera::set_field_internal(const char *feature, bool value) {
  arv_device_set_boolean_feature_value(mDevice, feature, value);
  check_status();
}

void aravis_camera::open_guard() {
  if (!opened()) {
    camera_driver::camera_not_open_error ex(this);
    BOOST_THROW_EXCEPTION(ex);
  }
}

void aravis_camera::get_field_internal(const char *feature, camera_driver::parameter_read<double> &p) {
  p.value = arv_device_get_float_feature_value(mDevice, feature);
  arv_device_get_float_feature_bounds(mDevice, feature, &p.min, &p.max);
}
void aravis_camera::get_field_internal(const char *feature, camera_driver::parameter_read<long> &p) {
  p.value = arv_device_get_integer_feature_value(mDevice, feature);
  arv_device_get_integer_feature_bounds(mDevice, feature, &p.min, &p.max);
}
void aravis_camera::get_field_internal(const char *feature, camera_driver::parameter_read<std::string &> &p) {
  p.value = std::string(arv_device_get_string_feature_value(mDevice, feature));
}
void aravis_camera::get_field_internal(const char *feature, camera_driver::parameter_read<const char *> &p) {
  p.value = arv_device_get_string_feature_value(mDevice, feature);
}
void aravis_camera::get_field_internal(const char *feature, camera_driver::parameter_read<bool> &p) {
  p.value = static_cast<bool>(arv_device_get_boolean_feature_value(mDevice, feature));
}

void aravis_camera::capture_guard() {
  if (!mCaptureMutex.try_lock()) {
    camera_driver::camera_capture_started_error ex(this);
    BOOST_THROW_EXCEPTION(ex);
  }
  mCaptureFlag.set_flag_blocking(true);
}
void aravis_camera::capture_guard_release() {
  mCaptureMutex.unlock();
  mCaptureFlag.set_flag_blocking(false);
}
void aravis_camera::capture_finalize() {
//  if(!mCaptureFinalizingMutex.try_lock()) {
//    CDWARNING("Double release prevented. Possibly due to shutdown just after stop capturing");
//    return;
//  }
  mCaptureFlag.enter_critical();
  mFrame.reset();

  g_object_unref(mStream);

  // check whether buffer is not released
  for (int i = 0; i < FRAME_BUFFER_NUMBER; ++i) {
    if (mBuffers[i] != nullptr && G_IS_OBJECT(mBuffers[i])) {
      CDWARNING("Releasing a leaking buffer...");
      g_object_unref(mBuffers[i]);
    }
  }
  delete[] mBuffers;

  mFrameCallback = nullptr;
  mStream = nullptr;
  capture_guard_release();
  CDINFO("Capture finalize");
  mCaptureFlag.exit_critical();
}

// TODO: when reset this is called. how to ensure the removed camera calling this function?
void aravis_camera::invalidate_camera() {
  CDINFO("Camera " << cd.model << " was flagged invalid");
  mValidFlag = false;
}
void aravis_camera::check_device_status_or_invalidate() {
  try {
    check_status();
  } catch (camera_driver::invalid_camera_error &ex) {
    invalidate_camera();
  }
}

} //namespace