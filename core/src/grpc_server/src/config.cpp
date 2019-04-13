//
// Created by wuyuanyi on 03/01/19.
//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <fstream>
#include "config.h"

std::shared_ptr<config_provider> config_provider::mInstance;

config_provider::config_provider() {
  try {
    boost::property_tree::ini_parser::read_ini(CONFIG_FILE_NAME, mTree);
  } catch (boost::exception & ex) {
    create_default_config();
    boost::property_tree::ini_parser::read_ini(CONFIG_FILE_NAME, mTree);
  }
}
void config_provider::create_default_config() {

  mTree.put("listen_port", "5074");
  mTree.put("listen_ip", "0.0.0.0");
  mTree.put("working_state_check_interval_ms", 1000);
  // TODO: put more here

  std::ofstream f(CONFIG_FILE_NAME);
  boost::property_tree::write_ini(f, mTree);
  f.close();
}
config_provider::~config_provider() {
  // sync the configuration file
  std::ofstream f(CONFIG_FILE_NAME);
  boost::property_tree::write_ini(f, mTree);
  f.close();
}
std::shared_ptr<config_provider> config_provider::get_instance() {
  if (!mInstance) {
    mInstance = std::make_shared<config_provider>();
  }

  return mInstance;
}

std::string config_provider::read(std::string field) {
  return mTree.get<std::string>(field);
}


void config_provider::write(std::string field, std::string value, bool save) {
  mTree.put(field, value);

  if (save) {
    std::ofstream f(CONFIG_FILE_NAME);
    boost::property_tree::write_ini(f, mTree);
    f.close();
  }
}
