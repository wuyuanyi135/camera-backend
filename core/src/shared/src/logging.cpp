//
// Created by wuyuanyi on 04/01/19.
//
#include <iostream>
#include <ctime>
#include <iomanip>
#include "logging.h"
std::ostream& _cdlogger(std::string sv, int line, std::string file) {

  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  CDOSTREAM << "\033[32m";
  CDOSTREAM << std::put_time(&tm, "%Y/%m/%d %H:%M:%S")<< " ";

  if (sv == "info") CDOSTREAM << "\033[32m";
  if (sv == "warning") CDOSTREAM << "\033[33;1m";
  if (sv == "error") CDOSTREAM << "\033[31;1m";

  CDOSTREAM << "[" << std::uppercase << sv <<"] ";

  CDOSTREAM << "\033[34m";

  CDOSTREAM << file << ":" << line << ": ";
  CDOSTREAM << "\033[37m";
  return CDOSTREAM;
}

