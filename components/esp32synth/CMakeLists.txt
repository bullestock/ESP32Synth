idf_component_register(
  SRCS "src/ESP32Synth.cpp"
  INCLUDE_DIRS "include"
  PRIV_REQUIRES esp_driver_dac esp_driver_i2s esp_timer
)
