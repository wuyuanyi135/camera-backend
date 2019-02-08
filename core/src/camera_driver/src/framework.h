//
// Created by wuyuanyi on 08/01/19.
//

#ifndef CAMERA_BACKEND_FRAMEWORK_H
#define CAMERA_BACKEND_FRAMEWORK_H
#include <memory>
#include <unordered_map>
#include "adapter.h"
#include "aravis/aravis_adapter.h"
#include "camera_container.h"
namespace camera_driver {
class framework {
 private:
  static std::unique_ptr<adapter> INTERFACE_REGISTRATION[];
 public:
  static std::shared_ptr<framework> get_instance();

 private:
  framework();
  static std::shared_ptr<framework> mInstance;

 public:
  // invoke the list update for each adapter and save the cache.
  void update_cache(adapter *obj = nullptr);
 private:
  void update_cache_internal(adapter *obj);

 private:
  std::unordered_map<std::string, camera_container> mCameraCache;

 public:
  const camera_container& query_by_id(std::string id);

 public:
  std::vector<adapter*> adapters();
  std::vector<camera_container*> camera_list();
};
}
#endif //CAMERA_BACKEND_FRAMEWORK_H
