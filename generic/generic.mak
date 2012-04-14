vpath %.c   ../generic
vpath %.cpp ../generic
vpath %.h   ../generic

OBJS += generic.o tile.o timer.o

ifneq ($(findstring qt,$(OSHW)),)
	OBJS += _in.o
else
	OBJS += in.o
endif

#
# Object files
#

generic.o : generic.c generic.h oshwbind.h
tile.o    : tile.c generic.h oshwbind.h ../gen.h ../oshw.h ../err.h \
            ../defs.h ../state.h
timer.o   : timer.c generic.h oshwbind.h ../gen.h ../oshw.h

IN_C_DEPS = generic.h oshwbind.h ../gen.h ../oshw.h ../err.h ../defs.h
in.o      : in.c $(IN_C_DEPS)
_in.o     : _in.cpp in.c $(IN_C_DEPS)
