//
// Created by wuyuanyi on 08/01/19.
//

#include <assert.h>
#include <iostream>

#include "framework.h"
#include "exceptions.h"
namespace camera_driver {
std::unique_ptr<adapter> framework::INTERFACE_REGISTRATION[] = {
    std::unique_ptr<adapter>(new aravis_camera_driver::aravis_adapter()),
};

std::shared_ptr<framework> framework::mInstance;

std::shared_ptr<framework> framework::get_instance() {
  if (!mInstance) {
    mInstance = std::shared_ptr<framework>(new framework());
  }
  return mInstance;
}
framework::framework() {
}
void framework::update_cache(adapter *obj) {
  if (obj == nullptr) {
    for (auto &it: INTERFACE_REGISTRATION) {
      update_cache_internal(&*it);
    }
  } else {
    update_cache_internal(obj);
  }
}

void framework::update_cache_internal(adapter *obj) {
  std::vector<camera_descriptor> camera;
  obj->camera_list(camera);

  std::unordered_map<std::string, std::shared_ptr<camera_device>> tmp;
  for (auto &cd : camera) {
    if (mCameraCache.find(cd.id) == mCameraCache.end()) {
      // not found
      const std::shared_ptr<camera_device> ptr = obj->create_camera(cd);
      tmp[cd.id] = ptr;
    } else {
      tmp[cd.id] = mCameraCache[cd.id];
    }
  }
  mCameraCache = tmp;
}
const std::shared_ptr<camera_device> framework::query_by_id(std::string id) {
  if (mCameraCache.find(id) == mCameraCache.end()) {
    id_not_found_error ex;
    BOOST_THROW_EXCEPTION(ex);
  }

  return mCameraCache[id];
}
std::vector<adapter *> framework::adapters() {
  std::vector<adapter *> v;
  for (std::unique_ptr<adapter> &it: INTERFACE_REGISTRATION) {
    v.emplace_back(it.get());
  }
  return v;
}
std::vector<std::shared_ptr<camera_device>> framework::camera_list() {
  std::vector<std::shared_ptr<camera_device>> containerList;
  for (auto &cam : mCameraCache) {
    containerList.emplace_back(cam.second);
  }
  return containerList;
}

}