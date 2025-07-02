# ~~~
# Summary:      Local, non-generic plugin setup
# Copyright (c) 2020-2021 Mike Rossiter
# License:      GPLv3+
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.


# -------- Options ----------

set(OCPN_TEST_REPO
    "opencpn/uktides-alpha"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_BETA_REPO
    "opencpn/uktides-beta"
    CACHE STRING
    "Default repository for tagged builds matching 'beta'"
)
set(OCPN_RELEASE_REPO
    "opencpn/uktides-prod"
    CACHE STRING
    "Default repository for tagged builds not matching 'beta'"
)

option(PLUGIN_USE_SVG "Use SVG graphics" ON)

#
#
# -------  Plugin setup --------
#
set(PKG_NAME UKTides_pi)
set(PKG_VERSION  2.5.0)
set(PKG_PRERELEASE "")  # Empty, or a tag like 'beta'

set(DISPLAY_NAME UKTides)    # Dialogs, installer artifacts, ...
set(PLUGIN_API_NAME UKTides) # As of GetCommonName() in plugin API
set(PKG_SUMMARY "Show UK tidal heights for LW/HW")
set(PKG_DESCRIPTION [=[
Show UK Tides (HW/LW) at over 600 ports.
]=])

set(PKG_AUTHOR "Mike Rossiter")
set(PKG_IS_OPEN_SOURCE "yes")
set(PKG_HOMEPAGE https://github.com/Rasbats/uktides_pi)
set(PKG_INFO_URL https://opencpn.org/OpenCPN/plugins/uktides.html)

set(SRC
    src/UKTides_pi.h
    src/UKTides_pi.cpp
    src/icons.h
    src/icons.cpp
    src/UKTidesgui.h
    src/UKTidesgui.cpp
    src/UKTidesgui_impl.cpp
    src/UKTidesgui_impl.h
    src/NavFunc.cpp
    src/NavFunc.h
    src/tidetable.cpp
    src/tidetable.h
    src/gl_private.h
    src/pidc.cpp
    src/pidc.h
    src/GL/gl.h
    src/GL/glext.h
)

set(PKG_API_LIB api-17)  #  A directory in libs/ e. g., api-17 or api-16

macro(late_init)
  # Perform initialization after the PACKAGE_NAME library, compilers
  # and ocpn::api is available.
  if (PLUGIN_USE_SVG)
    target_compile_definitions(${PACKAGE_NAME} PUBLIC PLUGIN_USE_SVG)
  endif ()

  add_definitions(-DocpnUSE_GL)

  if (QT_ANDROID)
    add_definitions(-DUSE_ANDROID_GLES2)
  endif ()

endmacro ()
macro(add_plugin_libraries)
  # Add libraries required by this plugin
  
  add_subdirectory("${CMAKE_SOURCE_DIR}/opencpn-libs/tinyxml")
  target_link_libraries(${PACKAGE_NAME} ocpn::tinyxml)

  add_subdirectory("libs/plugingl")
  target_link_libraries(${PACKAGE_NAME} ocpn::plugingl)

  add_subdirectory("${CMAKE_SOURCE_DIR}/opencpn-libs/jsoncpp")
  target_link_libraries(${PACKAGE_NAME} ocpn::jsoncpp)

  add_subdirectory("${CMAKE_SOURCE_DIR}/opencpn-libs/plugin_dc")
  target_link_libraries(${PACKAGE_NAME} ocpn::plugin-dc)
endmacro ()
