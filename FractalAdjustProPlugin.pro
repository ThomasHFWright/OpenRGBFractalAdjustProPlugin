TEMPLATE = lib
CONFIG += plugin c++17 link_pkgconfig
QT += widgets
!equals(QT_MAJOR_VERSION, 5): error("Use Qt 5 qmake for the official 1.0rc3.1 AppImage")
TARGET = FractalAdjustProPlugin
isEmpty(OPENRGB_SOURCE): error("Pass OPENRGB_SOURCE=/path/to/official/OpenRGB")
INCLUDEPATH += $$OPENRGB_SOURCE $$OPENRGB_SOURCE/RGBController $$OPENRGB_SOURCE/i2c_smbus src
PKGCONFIG += hidapi-hidraw
SOURCES += src/FractalAdjustProWidget.cpp src/FractalAdjustProPlugin.cpp src/FractalAdjustProController.cpp \
           $$OPENRGB_SOURCE/RGBController/RGBController.cpp
HEADERS += src/FractalAdjustProPlugin.h src/FractalAdjustProController.h src/FractalAdjustProProtocol.h src/FractalAdjustProThemes.h src/FractalAdjustProStartup.h
DISTFILES += src/plugin.json
QMAKE_CXXFLAGS += -Wall -Wextra

# Keep our linked API 4 implementation local to the plugin.
QMAKE_CXXFLAGS += -fvisibility=hidden
