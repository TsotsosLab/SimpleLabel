win32:TEMPLATE = vcapp
unix:TEMPLATE = app
CONFIG	+= qt thread warn_on debug_and_release build_all largefile
QT += xml testlib
#DEFINES += _USE_MATH_DEFINES
INCLUDEPATH += ./GeneratedFiles
#VERSION = 0.0.1

win32 {
	CONFIG += windows
	INCLUDEPATH += C:/OpenCV/include \
				C:/gtest-1.6.0/include
	DEFINES += APP_VERSION=\"$$VERSION\"
	DEFINES += _CRT_SECURE_NO_WARNINGS

	CONFIG(debug, debug|release) {
     		LIBS += C:/OpenCV/lib/opencv_core246d.lib \
    		C:/OpenCV/lib/opencv_highgui246d.lib \
		C:/gtest-1.6.0/lib/gtest-mdd.lib \
		C:/gtest-1.6.0/lib/gtest_main-mdd.lib

		DESTDIR = ./debug
		MOC_DIR += ./GeneratedFiles/debug
		OBJECTS_DIR += debug
    		INCLUDEPATH += ./GeneratedFiles/debug
	} else {
		LIBS += C:/OpenCV/lib/opencv_core246.lib \
    		C:/OpenCV/lib/opencv_highgui246.lib \
		C:/gtest-1.6.0/lib/gtest-md.lib \
		C:/gtest-1.6.0/lib/gtest_main-md.lib

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

TARGET = SimpleLabelTests
DEPENDPATH += .
include(SimpleLabelTests.pri)
