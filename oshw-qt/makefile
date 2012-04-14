default_goal: ../lib$(OSHW).a

include ../common.mak
include ../generic/generic.mak


# SDL setup

ifeq ($(OSTYPE),windows)
	CFLAGS += -I$(SDLDIR)/include/SDL
else
	CFLAGS += $(shell sdl-config --cflags)
endif


#
# Qt setup
#

ifdef QTDIR
	QTINCDIR = $(QTDIR)/include
	UIC = $(QTDIR)/bin/uic
	MOC = $(QTDIR)/bin/moc
else
	QTINCDIR = $(shell qmake -query QT_INSTALL_HEADERS)
	UIC = uic
	MOC = moc
endif

CXXFLAGS += -I$(QTINCDIR) \
            -I$(QTINCDIR)/QtCore \
            -I$(QTINCDIR)/QtGui \
            -I$(QTINCDIR)/QtXml

ui_%.h: %.ui
	@echo Compiling UI $<...
	$(UIC) -o $@ $<
	
moc_%.cpp: %.h
	@echo MOC-ing $<...
	$(MOC) -o $@ $<


OBJS += _sdlsfx.o oshwbind.o CCMetaData.o TWDisplayWidget.o TWProgressBar.o \
	    TWMainWnd.o moc_TWMainWnd.o TWApp.o

#
# The main target
#

../lib$(OSHW).a: $(OBJS)
	@echo Creating library $@...
	ar crs $@ $^

#
# Dependencies
#

_sdlsfx.o: _sdlsfx.c ../oshw-sdl/sdlsfx.c ../oshw-sdl/sdlsfx.h \
           ../gen.h ../oshw.h ../err.h ../defs.h ../state.h

oshwbind.o: generic.h oshwbind.h ../gen.h

CCMetaData.o: CCMetaData.h

TWDisplayWidget.o: TWDisplayWidget.h

TWProgressBar.o: TWProgressBar.h

DEPS = TWApp.h TWMainWnd.h ui_TWMainWnd.h TWProgressBar.h TWDisplayWidget.h CCMetaData.h \
	   generic.h oshwbind.h ../gen.h ../defs.h ../state.h ../play.h ../oshw.h

TWMainWnd.o: TWMainWnd.cpp $(DEPS)

moc_TWMainWnd.cpp: $(DEPS)
moc_TWMainWnd.o: moc_TWMainWnd.cpp $(DEPS)

TWApp.o: TWApp.cpp $(DEPS) ../oshw-sdl/sdlsfx.h

#
# Other
#

all: ../lib$(OSHW).a

clean:
	@echo Cleaning $(OSHW)...
	$(RM_F) $(OBJS) ui_*.h moc_*.cpp
