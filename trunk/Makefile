
TARGET  := loki_demos
OBJS	:= loki_demos.o play_movie.o loki_launchurl.o
CFLAGS  := -g -Wall
CFLAGS  += $(shell sdl-config --cflags)
LFLAGS  := -lSDL_image -lpng -lz -ljpeg -lSDL_mixer -lsmpeg
LFLAGS  += $(shell sdl-config --static-libs)
ARCH    := $(shell sh print_arch)
ifeq ($(ARCH), alpha)
CFLAGS  += -mcpu=ev4 -Wa,-mall
endif
INSTALL := ../../bin/$(ARCH)/$(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LFLAGS) -static

install: $(TARGET)
	@echo "$(TARGET) -> $(INSTALL)"
	@cp -p $(TARGET) $(INSTALL)
	@strip $(INSTALL)
	-brandelf -t $(shell uname -s) $(INSTALL)

clean:
	rm -f $(TARGET) *.o
