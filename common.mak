#
# Standard directory locations for UNIX (no configure script as yet)
#

ifneq ($(OSTYPE),windows)
	bindir = /usr/local/games
	sharedir = /usr/local/share/tworld
	mandir = /usr/local/man
endif


OSHW ?= oshw-qt


ifeq ($(OSTYPE),windows)
	EXE  = .exe
	RM_F = del /f
	CP   = copy
	CC   = gcc
	CXX  = g++
else
	EXE  =
	RM_F = rm -f
	CP   = cp
endif

LINK = $(CXX)


COMMONFLAGS = -Wall -O2 -I. -DNDEBUG
ifneq ($(OSTYPE),windows)
	COMMONFLAGS += '-DROOTDIR="$(sharedir)"' -Dstricmp=strcasecmp
endif
ifneq ($(findstring qt,$(OSHW)),)
        COMMONFLAGS += -DTWPLUSPLUS
endif

CFLAGS += $(COMMONFLAGS)
CXXFLAGS += $(COMMONFLAGS)

%.o: %.c
	@echo Compiling $<...
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<

%.o: %.cpp
	@echo Compiling $<...
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<
