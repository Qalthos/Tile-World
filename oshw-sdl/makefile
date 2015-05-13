default_goal: ../lib$(OSHW).a

include ../common.mak
include ../generic/generic.mak

ifeq ($(OSTYPE),windows)
	CFLAGS += -I$(SDLDIR)/include/SDL
else
	CFLAGS += $(shell sdl-config --cflags)
endif


OBJS += sdloshw.o sdlerr.o sdlin.o sdlout.o sdltext.o sdlsfx.o \
        oshwbind.o

#
# The main target
#

../lib$(OSHW).a: $(OBJS)
	@echo Creating library $@...
	ar crs $@ $^

#
# Object files
#

sdlerr.o  : sdlerr.c sdlgen.h generic.h oshwbind.h ../gen.h ../oshw.h
sdloshw.o : sdloshw.c sdlgen.h sdlsfx.h generic.h oshwbind.h ccicon.c ../gen.h \
            ../oshw.h ../err.h
sdltext.o : sdltext.c sdlgen.h generic.h oshwbind.h ../gen.h ../oshw.h ../err.h
sdlsfx.o  : sdlsfx.c sdlsfx.h ../gen.h ../oshw.h ../err.h ../defs.h ../state.h ../settings.h
sdlin.o   : sdlin.c sdlgen.h generic.h oshwbind.h ../gen.h ../oshw.h ../defs.h
sdlout.o  : sdlout.c sdlgen.h generic.h oshwbind.h ../gen.h ../oshw.h ../err.h \
            ../defs.h ../state.h
oshwbind.o: generic.h oshwbind.h ../gen.h ../err.h

#
# Other
#

all: ../lib$(OSHW).a

clean:
	@echo Cleaning $(OSHW)...
	$(RM_F) $(OBJS)
