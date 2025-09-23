TARGET_LIB = libgraphics.a
OBJS = graphics.o

INCDIR = ../include
CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = ../lib
LDFLAGS =
LIBS = -lpng16 -lz

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

install: $(TARGET_LIB)
	@cp $(TARGET_LIB) $(LIBDIR)
	@echo "Installed $(TARGET_LIB) to $(LIBDIR)"

clean:
	rm -f $(OBJS) $(TARGET_LIB)