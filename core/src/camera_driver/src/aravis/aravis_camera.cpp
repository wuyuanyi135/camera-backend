//
// Created by wuyuanyi on 08/01/19.
//
#include <thread>
#include <chrono>

#include <finally.h>
#include <future>
#include "aravis_camera.h"
#include "exceptions.h"
#include "logging.h"

#define ABORT_WAIT_TIME_MS 1000
namespace aravis_camera_driver {

void stream_callback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer) {
  aravis_camera *camera = static_cast<aravis_camera *>(user_data);
  switch (type) {
    case ARV_STREAM_CALLBACK_TYPE_START_BUFFER:break;
    case ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE:
      if (camera->mFrame && camera->mFrameCallback && camera->mStream) {
        arv_stream_push_buffer(camera->mStream, arv_stream_try_pop_buffer(camera->mStream));

        camera->buffer_to_frame(buffer, *camera->mFrame);
        camera->mFrameCallback(*camera->mFrame);

        // In order to reuse the buffer, the stream must be dequeued and requeued.
      } else {
        CDWARNING("Buffer arrives after clean up");
      }

      break;
    case ARV_STREAM_CALLBACK_TYPE_INIT:
      if (camera->mStartedCallback) {
        camera->mStartedCallback(*camera);
      }
      break;
    case ARV_STREAM_CALLBACK_TYPE_EXIT:CDINFO("Stream exits");
      break;
  }
}

aravis_camera::~aravis_camera() {
  try {
    if (opened()) {
      shutdown_camera();
    }
  } catch (boost::exception & ex) {
  }
  invalidate_camera();

}

aravis_camera::aravis_camera(camera_driver::camera_descriptor &cd) : cd(cd) {
  mCaptureFlag.set_flag_blocking(false);
  mValidFlag = true;
}

void aravis_camera::open_camera() {
  if (opened()) {
    camera_driver::camera_already_open ex(this);
    BOOST_THROW_EXCEPTION(ex);
  }
  mCamera = arv_camera_new(cd.id.c_str());
  mDevice = arv_camera_get_device(mCamera);
  if (mCamera == nullptr || mDevice == nullptr) {
    camera_driver::camera_start_failed_error ex(this);
    invalidate_camera();
    BOOST_THROW_EXCEPTION(ex);
  }
  on_camera_open();
}
void aravis_camera::shutdown_camera() {
  if (!opened()) {
    CDWARNING("Trying to shutdown an unopend camera: " << cd.id);
    return;
  }

  if (capturing()) {
    arv_camera_abort_acquisition(mCamera);
    capture_finalize();
  }

  g_object_unref(mCamera);

  mCamera = nullptr;
  mDevice = nullptr;
}

void aravis_camera::set_configuration(camera_driver::camera_parameter_write &param) {
  open_guard();
  try {
    set_parameter_internal(param.exposure, "ExposureTime");
    set_parameter_internal(param.frame_rate, "AcquisitionFrameRate");
    set_parameter_internal(param.gain, "Gain");
    set_parameter_internal(param.black_level, "BlackLevel");
    set_parameter_internal(param.gamma, "Gamma");


    // capturing mode cannot be modified during acquisition
    if (!capturing()) {
      if (param.frame_number.value == 0) {
        arv_camera_set_acquisition_mode(mCamera, ARV_ACQUISITION_MODE_CONTINUOUS);
      } else if (param.frame_number.value == 1) {
        arv_camera_set_acquisition_mode(mCamera, ARV_ACQUISITION_MODE_SINGLE_FRAME);
      } else {
        arv_camera_set_acquisition_mode(mCamera, ARV_ACQUISITION_MODE_MULTI_FRAME);
        set_parameter_internal(param.frame_number, "AcquisitionFrameCount");
      }
    }
  } catch (camera_driver::invalid_camera_error &ex) {
    invalidate_camera();
  }
}

void aravis_camera::on_camera_open() {

}
bool aravis_camera::opened() {
  if (!mValidFlag) {
    camera_driver::invalid_camera_error ex(this);
    BOOST_THROW_EXCEPTION(ex);
  }
  return mDevice != nullptr;
}

bool aravis_camera::valid() {
  return mValidFlag;
}

void aravis_camera::get_configuration(camera_driver::camera_parameter_read &param) {
  open_guard();

  get_parameter_internal(param.exposure, "ExposureTime");
  get_parameter_internal(param.frame_rate, "AcquisitionFrameRate");
  get_parameter_internal(param.gain, "Gain");
  get_parameter_internal(param.black_level, "BlackLevel");
  get_parameter_internal(param.gamma, "Gamma");
  get_parameter_internal(param.frame_number, "AcquisitionFrameCount");
  check_device_status_or_invalidate();
}

bool aravis_camera::capturing() {
  return mCaptureFlag.get_flag();
}
void aravis_camera::capture(camera_driver::frame &frame) {
  open_guard();
  capture_guard();
  FINALLY(capture_guard_release(););
  try {
    ArvBuffer *buffer = arv_camera_acquisition(mCamera, 0);
    assert(buffer);
    FINALLY(g_clear_object(&buffer););
    buffer_to_frame(buffer, frame);

  } catch (boost::exception &ex) {
    check_device_status_or_invalidate();
    throw;
  }
}
void aravis_camera::buffer_to_frame(ArvBuffer *buffer, camera_driver::frame &frame) const {
  frame.id = arv_buffer_get_frame_id(buffer);
  frame.height = static_cast<unsigned int>(arv_buffer_get_image_height(buffer));
  frame.width = static_cast<unsigned int>(arv_buffer_get_image_width(buffer));
  frame.time_stamp = arv_buffer_get_timestamp(buffer);
  frame.pixel_format = arv_buffer_get_image_pixel_format(buffer);
  size_t size;
  const void *const imageBuffer = arv_buffer_get_data(buffer, &size);
  frame.size = static_cast<unsigned int>(size);
  if (!frame.data) {
    frame.data = new uint8_t[frame.size];
  }
  memcpy(frame.data, imageBuffer, frame.size);
}
void aravis_camera::capture_async(camera_driver::frame_handler handler) {
  open_guard();
  capture_guard();
  mFrame = std::make_unique<camera_driver::frame>();
  mFrameCallback = handler;
  mStream = arv_camera_create_stream(mCamera, stream_callback, this);
  mBuffers = new ArvBuffer *[FRAME_BUFFER_NUMBER];
  guint payload = arv_camera_get_payload(mCamera);
  for (int i = 0; i < FRAME_BUFFER_NUMBER; ++i) {
    mBuffers[i] = arv_buffer_new(payload, nullptr);
    arv_stream_push_buffer(mStream, mBuffers[i]);
  }
  arv_camera_start_acquisition(mCamera);
  check_device_status_or_invalidate();
}
void aravis_camera::stop_capture_async() {
  if (capturing()) {
    arv_camera_stop_acquisition(mCamera);
    capture_finalize();
  }
  check_device_status_or_invalidate();
}
void aravis_camera::get_status(camera_driver::status &status) {
  status.temperature = arv_device_get_float_feature_value(mDevice, "DeviceTemperature");
  check_device_status_or_invalidate();
}
void aravis_camera::suspend() {

}
void aravis_camera::resume() {

}
void aravis_camera::reset() {
  open_guard();
  arv_device_execute_command(mDevice, "DeviceReset");
  shutdown_camera();

  // the camera will need to be re-enumerated. It should be removed from the instance cache!
  invalidate_camera();
}
void aravis_camera::register_capture_start_event_handler(camera_driver::capture_started_event_handler started,
                                                         camera_driver::capture_stopped_event_handler stopped) {
  mStartedCallback = started;
  mStoppedCallback = stopped;
}

}