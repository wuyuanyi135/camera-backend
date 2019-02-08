//
// Created by wuyuanyi on 19/11/18.
//
#include <iostream>
#include "server.h"
#include "config.h"

int main() {
  auto server = camera_backend_server::start_server();
  server->Wait();
  return 0;
}