//
// Created by wuyuanyi on 07/01/19.
//

#ifndef CAMERA_BACKEND_FRAME_HANDLER_H
#define CAMERA_BACKEND_FRAME_HANDLER_H
#include <functional>
#include "frame.h"

namespace camera_driver {
typedef std::function<void (frame &frame)> frame_handler;
}
#endif //CAMERA_BACKEND_FRAME_HANDLER_H
