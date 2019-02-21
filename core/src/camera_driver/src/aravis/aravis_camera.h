//
// Created by wuyuanyi on 08/01/19.
//

#ifndef CAMERA_BACKEND_ARAVIS_CAMERA_H
#define CAMERA_BACKEND_ARAVIS_CAMERA_H
#include <arv.h>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <critical_wait_flags/critical_wait_flags.h>
#include "camera_driver.h"
#include "exceptions.h"

#define FRAME_BUFFER_NUMBER 5

/// currently, camera disconnection can only be detected when some operation was done
namespace aravis_camera_driver {
class aravis_camera : public camera_driver::camera_device {
 private:
  camera_driver::camera_descriptor cd;
  ArvCamera *mCamera = nullptr;
  ArvDevice *mDevice = nullptr;
  ArvStream* mStream = nullptr;
  ArvBuffer** mBuffers = nullptr;
  std::atomic_bool mValidFlag;
  std::unique_ptr<camera_driver::frame> mFrame; // prevent reallocation for each frame
  camera_driver::frame_handler mFrameCallback = nullptr;
  std::mutex mCaptureMutex;
//  std::mutex mCaptureFinalizingMutex; // the finalization seems very slow and may double run if shutdown the camera just after finalizing capture.
//  std::atomic_bool mCaptureFlag;
  critical_wait_flag<bool> mCaptureFlag;

 private:
  camera_driver::capture_started_event_handler mStartedCallback = nullptr;
  camera_driver::capture_stopped_event_handler mStoppedCallback = nullptr;


  camera_driver::camera_capability mCapabilities{
      .can_shutdown = true,
      .should_open = true,
      .can_capture_async = true,
      .can_capture = true,
      .can_adjust_exposure = true,
      .can_adjust_gain = true,
      .can_adjust_gamma = true,
      .can_adjust_black_level = true,
      .can_adjust_frame_rate = true,
      .can_set_frame_number = true,
      .can_get_temperature = true,
      .can_suspend = false,
      .can_reset = true,
  };

 public:
  ~aravis_camera();
  explicit aravis_camera(camera_driver::camera_descriptor &cd);
  void open_camera() override;
  void shutdown_camera() override;
  camera_driver::camera_capability *capabilities() override;
  void set_configuration(camera_driver::camera_parameter_write &param) override;
  void get_configuration(camera_driver::camera_parameter_read &param) override;
  bool opened() override;
  bool capturing() override;
  void capture(camera_driver::frame &frame) override;
  void capture_async(camera_driver::frame_handler handler) override;
  void stop_capture_async() override;
  void get_status(camera_driver::status &status) override;
  void suspend() override;
  void resume() override;
  void reset() override;
  void register_capture_start_event_handler(camera_driver::capture_started_event_handler started,
                                            camera_driver::capture_stopped_event_handler stopped) override;
 private:
  template<typename T>
  void set_parameter_internal(const camera_driver::parameter_write<T> &p, std::string fieldName);
  template<typename T>
  void get_parameter_internal(camera_driver::parameter_read<T> &p, std::string fieldName);
 public:
  bool valid() override;
 private:
  void field_bounds_internal(const char *feature, double &min, double &max);
  void field_bounds_internal(const char *feature, long &min, long &max);

 private:
  void set_field_internal(const char *feature, double value);
  void set_field_internal(const char *feature, long value);
  void set_field_internal(const char *feature, std::string &value);
  void set_field_internal(const char *feature, const char *value);
  void set_field_internal(const char *feature, bool value);

 private:
  void get_field_internal(const char *feature, camera_driver::parameter_read<double> &p);
  void get_field_internal(const char *feature, camera_driver::parameter_read<long> &p);
  void get_field_internal(const char *feature, camera_driver::parameter_read<std::string &> &p);
  void get_field_internal(const char *feature, camera_driver::parameter_read<const char *> &p);
  void get_field_internal(const char *feature, camera_driver::parameter_read<bool> &p);

  void check_status();

  void on_camera_open();
  /// Make the camera invalid and let throw exception when accessed.
  void invalidate_camera();

  void open_guard();
  void feature_guard(std::string &fieldName) const;
  void capture_guard();
  void capture_guard_release();

  void buffer_to_frame(ArvBuffer *buffer, camera_driver::frame &frame) const;
  void capture_finalize();

 public:
  friend void stream_callback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer);

  void check_device_status_or_invalidate();
};

#include "aravis_camera_template_impl.tcc"
}
#endif //CAMERA_BACKEND_ARAVIS_CAMERA_H
