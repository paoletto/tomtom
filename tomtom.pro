TEMPLATE = lib
TARGET = qtgeoservices_tomtom
CONFIG += plugin relative_qt_rpath

QT += location-private positioning-private network

qtConfig(location-labs-plugin): DEFINES += LOCATIONLABS

HEADERS += \
    qgeocodingmanagerenginetomtom.h \
    qgeoroutingmanagerenginetomtom.h \
    qgeoserviceproviderplugintomtom.h \
    qgeotilefetchertomtom.h \
    qgeomapreplytomtom.h \
    qgeofiletilecachetomtom.h \
    qgeotiledmaptomtom.h \
    qgeoroutereplytomtom.h \
    qgeotiledmappingmanagerenginetomtom.h \
    qgeocodereplytomtom.h \
    qplacemanagerenginetomtom.h \
    qtomtomcommon.h

SOURCES += \
    qgeocodingmanagerenginetomtom.cpp \
    qgeoroutingmanagerenginetomtom.cpp \
    qgeoserviceproviderplugintomtom.cpp \
    qgeotilefetchertomtom.cpp \
    qgeomapreplytomtom.cpp \
    qgeofiletilecachetomtom.cpp \
    qgeotiledmaptomtom.cpp \
    qgeoroutereplytomtom.cpp \
    qgeotiledmappingmanagerenginetomtom.cpp \
    qgeocodereplytomtom.cpp \
    qplacemanagerenginetomtom.cpp \

OTHER_FILES += \
    tomtom_plugin.json \
    README.md

PLUGIN_TYPE = geoservices
PLUGIN_CLASS_NAME = QGeoServiceProviderFactoryTomTom

