
TARGET  := demo_prefs

OBJS    := demo_prefs.o

CFLAGS = -g -O2 -Wall
CFLAGS += $(shell gtk-config --cflags) $(shell libglade-config --cflags)
CFLAGS += $(shell xml-config --cflags)
LFLAGS += -Wl,-Bstatic
LFLAGS += -L$(shell libglade-config --prefix)
LFLAGS +=  -lglade
LFLAGS += -L$(shell gtk-config --prefix)
LFLAGS +=  -lgtk -lgdk -rdynamic -lgmodule -lglib 
LFLAGS += -L$(shell xml-config --prefix)
LFLAGS += -lxml -lz
LFLAGS += -Wl,-Bdynamic
LFLAGS += -L/usr/X11R6/lib -lXi -lXext -lX11 -lm -ldl

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	rm -f *.o *.bak core $(TARGET)
