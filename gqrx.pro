#-------------------------------------------------
#
# Qmake project file for gqrx
#
#-------------------------------------------------

QT       += core gui

TARGET = gqrx
TEMPLATE = app

# disable debug messages in release
CONFIG(debug, debug|release) {
    # Define version string (see below for releases)
    VER = $$system(git describe --abbrev=8)
} else {
    DEFINES += QT_NO_DEBUG
    DEFINES += QT_NO_DEBUG_OUTPUT
    VER = 2.0
}

# Tip from: http://www.qtcentre.org/wiki/index.php?title=Version_numbering_using_QMake
VERSTR = '\\"$${VER}\\"'          # place quotes around the version string
DEFINES += VERSION=\"$${VERSTR}\" # create a VERSION macro containing the version string

# QMAKE_RPATH & co won't work with origin
QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/lib\''

SOURCES +=\
    receiver.cc \
    main.cc \
    mainwindow.cc \
    qtgui/freqctrl.cpp \
    qtgui/meter.cpp \
    qtgui/plotter.cpp \
    dsp/rx_fft.cc \
    dsp/rx_filter.cc \
    dsp/rx_demod_fm.cc \
    dsp/rx_meter.cc \
    qtgui/dockrxopt.cc \
    dsp/rx_demod_am.cc \
    qtgui/ioconfig.cc \
    qtgui/dockfcdctl.cpp \
    qtgui/dockaudio.cpp \
    dsp/resampler_ff.cc \
    qtgui/dockiqrecorder.cpp \
    qtgui/dockfft.cpp


HEADERS  += mainwindow.h \
    receiver.h \
    qtgui/freqctrl.h \
    qtgui/meter.h \
    qtgui/plotter.h \
    dsp/rx_fft.h \
    dsp/rx_filter.h \
    dsp/rx_demod_fm.h \
    dsp/rx_meter.h \
    qtgui/dockrxopt.h \
    dsp/rx_demod_am.h \
    qtgui/ioconfig.h \
    qtgui/dockfcdctl.h \
    qtgui/dockaudio.h \
    dsp/resampler_ff.h \
    qtgui/dockiqrecorder.h \
    qtgui/dockfft.h

FORMS    += \
    qtgui/dockrxopt.ui \
    mainwindow.ui \
    qtgui/ioconfig.ui \
    qtgui/dockfcdctl.ui \
    qtgui/dockaudio.ui \
    qtgui/dockiqrecorder.ui \
    qtgui/dockfft.ui


# dependencies via pkg-config
# FIXME: check for version?
linux-g++ {
    CONFIG += link_pkgconfig
    PKGCONFIG += gnuradio-core gnuradio-audio gnuradio-fcd
}

OTHER_FILES += \
    README

RESOURCES += \
    icons.qrc
