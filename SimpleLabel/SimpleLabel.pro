win32:TEMPLATE = vcapp
unix:TEMPLATE = app
CONFIG	+= qt thread warn_on debug_and_release build_all largefile
QT += xml
#DEFINES += _USE_MATH_DEFINES
INCLUDEPATH += ./GeneratedFiles
VERSION = 1.2.1

win32 {
	CONFIG += windows
	INCLUDEPATH += C:/OpenCV/include
	DEFINES += APP_VERSION=\"$$VERSION\"
	DEFINES += _CRT_SECURE_NO_WARNINGS

	CONFIG(debug, debug|release) {
     		LIBS += C:/OpenCV/lib/opencv_core246d.lib \
    		C:/OpenCV/lib/opencv_highgui246d.lib

		DESTDIR = ./debug
		MOC_DIR += ./GeneratedFiles/debug
		OBJECTS_DIR += debug
    		INCLUDEPATH += ./GeneratedFiles/debug
	} else {
		LIBS += C:/OpenCV/lib/opencv_core246.lib \
    		C:/OpenCV/lib/opencv_highgui246.lib

		DESTDIR = ./release
    		INCLUDEPATH += ./GeneratedFiles/release
		MOC_DIR += ./GeneratedFiles/release
		OBJECTS_DIR += release
	}
}

unix {
	INCLUDEPATH += /usr/include/opencv
	LIBS += -lcv -lcxcore -lhighgui
	DEFINES += APP_VERSION=\\\"$$VERSION\\\"
}

TARGET = SimpleLabel
DEPENDPATH += .
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(SimpleLabel.pri)
