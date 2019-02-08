//
// Created by wuyuanyi on 28/12/18.
//

#ifndef CAMERA_BACKEND_FRAME_H
#define CAMERA_BACKEND_FRAME_H
#include <ctime>
#include <cstring>

#include <inttypes.h>
namespace camera_driver {
class frame {
 public:
  std::time_t time_stamp{};
  unsigned long long id{};
  unsigned int  size{};
  unsigned int height{};
  unsigned int width{};
  unsigned int pixel_format{};
  uint8_t *data = nullptr;

  ~frame() { delete[] data;};
  frame() = default;
  frame(const frame& another) {
    id = another.id;
    size = another.size;
    width = another.width;
    pixel_format = another.pixel_format;
    data = new uint8_t[size]();
    std::memcpy(data, another.data, size);
  }
  frame(frame&& another) noexcept {
    id = another.id;
    size = another.size;
    width = another.width;
    pixel_format = another.pixel_format;
    data = another.data;
    another.data = nullptr; // prevent delete
  }
};
}
#endif //CAMERA_BACKEND_FRAME_H
