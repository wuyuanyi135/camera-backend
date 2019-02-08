//
// Created by wuyuanyi on 10/01/19.
//

#ifndef CAMERA_BACKEND_EVENT_HANDLERS_H
#define CAMERA_BACKEND_EVENT_HANDLERS_H
namespace camera_driver {
class camera_device;
typedef void (*capture_started_event_handler)(camera_driver::camera_device &camera);
typedef void (*capture_stopped_event_handler)(camera_driver::camera_device &camera);

}
#endif //CAMERA_BACKEND_EVENT_HANDLERS_H
