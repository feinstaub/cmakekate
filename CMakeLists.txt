
cmake_minimum_required (VERSION 3.3)

project(cmakekate)

set (QT_MIN_VERSION "5.2.0")
set(KF5_DEP_VERSION "5.6.0")

find_package(ECM 1.1.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core)

find_package(KF5 "${KF5_DEP_VERSION}" REQUIRED COMPONENTS
  GuiAddons
  I18n
  KIO
  TextEditor
)

add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
  -DQT_USE_FAST_OPERATOR_PLUS
)

add_library(cmakekateplugin MODULE
  lib/cmakeclient.cpp
  lib/projectmodel.cpp
  lib/debugwidget.cpp
  plugin/cmakekateplugin.cpp
  plugin/cmakekatewindowintegration.cpp
  plugin/plugin.qrc
)
target_include_directories(cmakekateplugin PRIVATE
  lib
  plugin
)
target_compile_definitions(cmakekateplugin PRIVATE cxx_override)


target_link_libraries(cmakekateplugin
  KF5::KIOFileWidgets
  KF5::TextEditor
)

install(TARGETS cmakekateplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

kcoreaddons_desktop_to_json(cmakekateplugin plugin/cmakekateplugin.desktop)
