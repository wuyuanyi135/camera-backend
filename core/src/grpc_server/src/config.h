//
// Created by wuyuanyi on 03/01/19.
//

#ifndef CAMERA_BACKEND_CONFIG_H
#define CAMERA_BACKEND_CONFIG_H
#include <string>

#include <boost/property_tree/ptree.hpp>

#define CONFIG_FILE_NAME "config.ini"
class config_provider {
 public:
  config_provider();
 public:
  ~config_provider();

 public:
  static std::shared_ptr<config_provider> get_instance();

 private:
  static std::shared_ptr<config_provider> mInstance;

 public:
  std::string read(std::string field);

  template <typename RetType>
  RetType read(std::string field);
  template <typename RetType>
  RetType read(std::string field, RetType defaultValue);

  void write(std::string field, std::string value, bool save);

 private:
  boost::property_tree::ptree mTree;
 public:
  void create_default_config();
};

template<typename RetType>
RetType config_provider::read(std::string field) {
  return mTree.get<RetType>(field);
}

template<typename RetType>
RetType config_provider::read(std::string field, RetType defaultValue) {
  return mTree.get<RetType>(field, defaultValue);
}

#endif //CAMERA_BACKEND_CONFIG_H
