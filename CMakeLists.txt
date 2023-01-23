# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{ADF_PATH}/CMakeLists.txt)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(somnus)
idf_build_set_property(COMPILE_OPTIONS "-fdiagnostics-color=always" APPEND)

set(COMPONENT_REQUIRES "Adafruit_SSD1306")
#idf_component_register(SRCS  "Adafruit_SSD1306.cpp" INCLUDE_DIRS ".")

set(COMPONENT_REQUIRES "Adafruit-GFX-Library")
#idf_component_register(SRCS "Adafruit_GFX.cpp" "Adafruit_GrayOLED.cpp" "Adafruit_SPITFT.cpp" "glcdfont.c"
#                       INCLUDE_DIRS "."
#                       REQUIRES arduino Adafruit_BusIO)

set(COMPONENT_REQUIRES "Adafruit_BusIO")

#idf_component_register(SRCS "Adafruit_I2CDevice.cpp" "Adafruit_BusIO_Register.cpp" "Adafruit_SPIDevice.cpp" 
#                       INCLUDE_DIRS "."
#                       REQUIRES arduino)
