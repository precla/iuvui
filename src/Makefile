TARGET = iuvui
TARGET_DEBUG = iuvui_dev
CC = gcc
CFLAGS = -Wall -std=gnu11 -pedantic -O2
LDLIBS = -lX11 -lm -lGL -lGLU
SRC = $(wildcard *.c)
SRCTOOBJ = $(patsubst %.c,%.o, $(SRC))
#SRCTOOBJDEV = $(patsubst %.c,%.o, $(SRC))
DEST = bin/

.PHONY default: $(TARGET)
.PHONY all: default
.PHONY debug: $(TARGET_DEBUG)
.PHONY clean: clean

# build for production, with optimization flag:
$(TARGET): $(SRCTOOBJ)
	mkdir -p $(DEST)
	$(CC) $(CFLAGS) -o $(DEST)$@ $(SRCTOOBJ) $(LDLIBS)
	-rm *.o

$(SRCTOOBJ): %.o: %.c
	$(CC) $(CFLAGS) $(BUILD) -c $< -o $@

# build for debug, with debug flag:
#$(TARGET_DEBUG): $(SRCTOOBJDEV)
#	$(CC) $(CFLAGS) -g -o $(DEST)$@ $(SRCTOOBJDEV) $(LDLIBS)
#	-rm *.o

#$(SRCTOOBJDEV): %.o: %.c
#	$(CC) $(CFLAGS) -g -c $< -o $@

# clean up the mess
clean:
	-rm *.o $(DEST)$(TARGET)
