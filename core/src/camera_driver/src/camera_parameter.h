//
// Created by wuyuanyi on 28/12/18.
//

#ifndef CAMERA_BACKEND_CAMERAPARAMETER_H
#define CAMERA_BACKEND_CAMERAPARAMETER_H

#include <type_traits>

namespace camera_driver {
template<typename ParameterType>
class parameter_read {
 public:
  ParameterType value;
  bool should_update;
  ParameterType min;
  ParameterType max;
};

template<typename ParameterType>
class parameter_write {
 public:
  ParameterType value;
  bool should_update;
};

template<typename T, bool Read>
using parameter = std::conditional_t <Read, parameter_read<T>, parameter_write<T>>;


template<bool Read>
class camera_parameter {
 public:
  parameter<double, Read> black_level;
  parameter<double, Read> exposure;
  parameter<long, Read>  frame_number; // 0 for continuous
  parameter<double, Read> frame_rate;
  parameter<double, Read> gain;
  parameter<double, Read> gamma;
};

using camera_parameter_write = camera_parameter<false> ;

using camera_parameter_read = camera_parameter<true> ;
}
#endif //CAMERA_BACKEND_CAMERAPARAMETER_H
