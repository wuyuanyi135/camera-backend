//
// Created by wuyuanyi on 19/12/18.
//

#ifndef CAMERA_BACKEND_CAPABILITIES_H
#define CAMERA_BACKEND_CAPABILITIES_H
#include <string>
namespace camera_driver {
class adapter_capability {
 public:
  bool should_shutdown;
};

class camera_capability {
 public:
  bool can_shutdown;
  bool should_open;

  bool can_capture_async;
  bool can_capture;

  bool can_adjust_exposure;

  bool can_adjust_gain;

  bool can_adjust_gamma;

  bool can_adjust_black_level;

  bool can_adjust_frame_rate;

  bool can_set_frame_number;

  bool can_get_temperature;

  bool can_suspend;
  bool can_reset;
};
}
#endif //CAMERA_BACKEND_CAPABILITIES_H
