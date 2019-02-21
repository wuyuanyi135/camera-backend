template<typename T>
void aravis_camera::set_parameter_internal(const camera_driver::parameter_write<T> &p, std::string fieldName) {
  if (p.should_update) {
    feature_guard(fieldName);

    // check range compatibility
    T min,max;
    field_bounds_internal(fieldName.c_str(), min, max);
    if (p.value < min || p.value > max) {
      camera_driver::parameter_out_of_range_error<decltype(min)> ex(this, min, max);
      BOOST_THROW_EXCEPTION(ex);
    }
    set_field_internal(fieldName.c_str(), p.value);
  }
}


template<typename T>
void aravis_camera::get_parameter_internal(camera_driver::parameter_read<T> &p, std::string fieldName) {
  feature_guard(fieldName);
  get_field_internal(fieldName.c_str(), p);
}
