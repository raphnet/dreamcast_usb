CC=gcc
LD=$(CC)
CFLAGS=-Wall -g `libpng-config --cflags`
LDFLAGS=`libpng-config --libs`

PROG=png_to_vmu_lcd
OBJS=main.o

all: $(PROG)

clean:
	rm $(PROG) $(OBJS)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@


