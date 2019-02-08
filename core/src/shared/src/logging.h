//
// Created by wuyuanyi on 04/01/19.
//
#ifndef CAMERA_BACKEND_LOGGING_H
#define CAMERA_BACKEND_LOGGING_H

#include <iostream>
#include <memory>
#include <string>
#include <sstream>

#ifndef CDLOGGER_DISABLED

#define CDLOGGER(sv, msg) _cdlogger(sv, __LINE__, __FILE__) << msg << std::endl
#define CDERROR(msg) CDLOGGER("error", msg)
#define CDWARNING(msg) CDLOGGER("warning", msg)
#define CDINFO(msg) CDLOGGER("info", msg)

#define CDOSTREAM std::cout

std::ostream& _cdlogger(std::string sv, int line, std::string file);
#endif

#endif //CAMERA_BACKEND_LOGGING_H