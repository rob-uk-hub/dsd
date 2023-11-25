# - Try to find Mosquitto
# Once done this will define
#
#  Mosquitto_FOUND - System has Mosquitto
#  Mosquitto_INCLUDE_DIR - The Mosquitto include directory
#  Mosquitto_LIBRARY - The library needed to use Mosquitto
#
# In order to enable MQTT support the following must be installed:
# MacOS: brew install mosquitto

if (NOT Mosquitto_INCLUDE_DIR)
  find_path(Mosquitto_INCLUDE_DIR mosquitto.h)
endif()


if (NOT Mosquitto_LIBRARY)
  find_library(
    Mosquitto_LIBRARY
    NAMES mosquitto)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  Mosquitto DEFAULT_MSG
  Mosquitto_LIBRARY Mosquitto_INCLUDE_DIR)

message(STATUS "libmosquitto include dir: ${Mosquitto_INCLUDE_DIR}")
message(STATUS "libmosquitto: ${Mosquitto_LIBRARY}")
set(Mosquitto_LIBRARIES ${Mosquitto_LIBRARY})

mark_as_advanced(Mosquitto_INCLUDE_DIR Mosquitto_LIBRARY)
