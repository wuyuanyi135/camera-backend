//
// Created by wuyuanyi on 28/12/18.
//

#ifndef CAMERA_BACKEND_CAMERA_H
#define CAMERA_BACKEND_CAMERA_H

#include "camera_descriptor.h"
#include "camera_parameter.h"
#include "frame.h"
#include "status.h"
#include "frame_handler.h"
#include "event_handlers.h"
#include "adapter.h"

namespace camera_driver{
class camera_device {
 public:
  adapter* adapter_ref;
  camera_descriptor camera_descriptor_ref;
 public:
  virtual void open_camera() = 0;
  virtual void shutdown_camera() = 0;

 public:
  /// Configure the camera parameter. Do not check the capabilities in this function. The invoker will observe the capability limitation.
  /// \param param
  virtual void set_configuration(camera_parameter_write& param) = 0;
  virtual void get_configuration(camera_parameter_read& param) = 0;

  /// Check whether the camera is opened and ready to use.
  /// This function may not guard the validity of the camera. Instead, check valid() to see if camera is still alive. If valid() is false,
  /// all operation on the camera should throw invalid_camera_error.
  /// \return
  virtual bool opened() = 0;
  virtual bool valid() = 0;
  virtual bool capturing() = 0;

  virtual void capture(frame &frame) = 0;
  /// Whatever capture mode is set, the handler will hand over the frame one by one.
  /// \param handler
  virtual void capture_async(frame_handler handler) = 0;
  virtual void stop_capture_async() = 0;

 public:
  virtual void get_status(status &status)=0;

 public:
  // suspend the power or communication to the device for power saving purpose
  virtual void suspend() = 0;

  // resume from the suspended state.
  virtual void resume() = 0;

  // reset camera to power-up state.
  virtual void reset() = 0;

  // register with nullptr to disable. re-register will overwrite.
  virtual void register_capture_start_event_handler(capture_started_event_handler started,
                                                    capture_stopped_event_handler stopped) = 0;

};
}
#endif //CAMERA_BACKEND_CAMERA_H
