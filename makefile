include common.mak


#
# Library / Executable setup
#

TWORLD = tworld

ifneq ($(findstring qt,$(OSHW)),)

TWORLD = tworld2
LINK = $(CXX)

ifdef QTDIR
	LOADLIBES += -L$(QTDIR)/lib
else
	LOADLIBES += -L$(shell qmake -query QT_INSTALL_LIBS)
endif

QTMODULES := Core Gui Xml
QTMODULES := $(addprefix -lQt,$(QTMODULES))
ifeq ($(OSTYPE),windows)
	QTMODULES := $(addsuffix 4,$(QTMODULES))
endif
LOADLIBES += $(QTMODULES)

# else
endif
# SDL still used partially by Qt OSHW layer

ifeq ($(OSTYPE),windows)
	LOADLIBES += -L$(SDLDIR)/lib -lmingw32 -lSDLmain -lSDL
else
	LOADLIBES += $(shell sdl-config --libs)
endif

# endif


#
# Object file listing
#

OBJS = \
tworld.o series.o play.o encoding.o solution.o res.o lxlogic.o mslogic.o \
unslist.o help.o score.o random.o cmdline.o fileio.o err.o lib$(OSHW).a

ifeq ($(OSTYPE),windows)
	RESOURCES = tworldres.o
else
	RESOURCES =
endif

#
# Binaries
#

$(TWORLD)$(EXE): $(OBJS) $(RESOURCES)
	@echo Linking $@...
	$(LINK) $(LDFLAGS) -o $@ $^ $(LOADLIBES)

mklynxcc$(EXE): mklynxcc.c
	@echo Building $@...
	$(CC) -Wall -W -O -o $@ $^

#
# Object files
#

tworld.o   : tworld.c defs.h gen.h err.h series.h res.h play.h score.h \
             solution.h fileio.h help.h oshw.h cmdline.h ver.h
series.o   : series.c series.h defs.h gen.h err.h fileio.h solution.h
play.o     : play.c play.h defs.h gen.h err.h state.h random.h oshw.h res.h \
             logic.h solution.h fileio.h
encoding.o : encoding.c encoding.h defs.h gen.h err.h state.h
solution.o : solution.c solution.h defs.h gen.h err.h fileio.h
res.o      : res.c res.h defs.h gen.h fileio.h err.h oshw.h
lxlogic.o  : lxlogic.c logic.h defs.h gen.h err.h state.h random.h
mslogic.o  : mslogic.c logic.h defs.h gen.h err.h state.h random.h
unslist.o  : unslist.c unslist.h gen.h err.h fileio.h
help.o     : help.c help.h defs.h gen.h state.h oshw.h ver.h comptime.h
score.o    : score.c score.h defs.h gen.h err.h play.h
random.o   : random.c random.h defs.h gen.h
cmdline.o  : cmdline.c cmdline.h gen.h
fileio.o   : fileio.c fileio.h defs.h gen.h err.h
err.o      : err.c oshw.h err.h

#
# Generated files
#

comptime.h:
	@echo Generating $@...
ifeq ($(OSTYPE),windows)
	echo #define COMPILE_TIME "%DATE% %TIME%" > comptime.h
else
	echo \#define COMPILE_TIME \"`date '+%Y %b %e %T %Z'`\" > comptime.h
endif

#
# Libraries
#

lib$(OSHW).a: oshw.h defs.h gen.h state.h play.h err.h \
              $(wildcard $(OSHW)/*.c $(OSHW)/*.cpp $(OSHW)/*.h $(OSHW)/*.ui) \
              $(wildcard generic/*.c generic/*.h)
	@echo --- Building $(OSHW)...
	$(MAKE) -C $(OSHW) OSHW=$(OSHW)
	@echo ---

#
# Resources
#

tworldres.o: $(TWORLD).ico
	@echo Compiling resource $<...
	echo SDL_app ICON $^ | windres -o $@

#
# Other
#

all: $(TWORLD)$(EXE) mklynxcc$(EXE)

clean:
	@echo Cleaning...
	$(RM_F) $(OBJS) $(RESOURCES) $(TWORLD)$(EXE) mklynxcc$(EXE) comptime.h
	$(MAKE) -C $(OSHW) clean


ifneq ($(OSTYPE),windows)

install: $(TWORLD)$(EXE)
	mkdir -p $(bindir)
	mkdir -p $(sharedir)/sets
	mkdir -p $(sharedir)/data
	mkdir -p $(sharedir)/res
	mkdir -p $(mandir)/man6
	cp -i ./$(TWORLD)$(EXE) $(bindir)/.
	cp -i sets/*.dac $(sharedir)/sets/.
	cp -i data/*.dat $(sharedir)/data/.
	cp -i res/rc $(sharedir)/res/.
	cp -i res/*.bmp $(sharedir)/res/.
	cp -i res/*.txt $(sharedir)/res/.
	cp -i res/*.wav $(sharedir)/res/.
	cp -i docs/$(TWORLD).6 $(mandir)/man6/.

install2: $(TWORLD)$(EXE)
	mkdir -p $(bindir)
	mkdir -p $(mandir)/man6
	cp -i ./$(TWORLD)$(EXE) $(bindir)/.
	cp -i docs/$(TWORLD).6 $(mandir)/man6/.

endif
