### Module for finding libudev

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(LIBUDEV libudev)
endif ()

find_path(LIBUDEV_INCLUDE_DIR
  NAMES libudev.h
  PATHS ${LIBUDEV_INCLUDE_DIRS} ${LIBUDEV_INCLUDEDIR}
  PATH_SUFFIXES include
)

find_library(LIBUDEV_LIBRARY
  NAMES udev
  PATHS ${LIBUDEV_LIBRARY_DIRS} ${LIBUDEV_LIBDIR}
  PATH_SUFFIXES lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBUDEV
  DEFAULT_MSG
  LIBUDEV_LIBRARY
  LIBUDEV_INCLUDE_DIR
)

if (LIBUDEV_FOUND)
  list(APPEND LIBUDEV_INCLUDE_DIRS ${LIBUDEV_INCLUDE_DIR})
  list(APPEND LIBUDEV_LIBRARIES ${LIBUDEV_LIBRARY})
endif ()

mark_as_advanced(
  LIBUDEV_INCLUDE_DIR
  LIBUDEV_LIBRARY
)
