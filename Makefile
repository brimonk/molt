# Brian Chrzanowski Tue Jan 01, 2019 13:51
#
# MOLT Specific (GNU) Makefile
#
# The build system has two required portions. Everything else is fluff.
#
# Building the runnable molt binary is done by compiling all of src/*.c into the
# resulting binary. This effectively includes storage setup, simulation setup,
# and handling for storing the timeslices. When the simulation is done, the main
# binary also frees all memory, safely closes the database, etc.
#
# The actual simulation code is implemented into libraries for each "type" of
# simulation. This allows for not only method isolation, but for method
# verification. Each module is explicitly and individually compiled in the
# DYNOBJECTS (dynamic object) target.

CC = cc
LINKER = -ldl -lpthread
FLAGS = -Wall -g3 -march=native
TARGET = molt
DYNSOFT = libsoftmolt.so
DYNOCL  = liboclmolt.so
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
# Define preprocessor parameters to change defaults. Like number of default
# particles, or the default simulation gridsize.
PREPROCESSPARMS = -DDEFAULT_GRIDLEN=128 -DDEFAULT_PARTS=1024

all: DYNOBJECTS $(TARGET)

$(DYNSOFT):
	# Compile the "normal" software implementation
	$(CC) $(FLAGS) -fPIC -shared -o libsoftmolt.so src/mod/softmolt.c

$(DYNOCL):
	# Compile the OpenCL Implementation
	$(CC) $(FLAGS) -fPIC -shared -o liboclmolt.so src/mod/oclmolt.c -lOpenCL
	# With the OpenCL Kernels (load the binary at runtime)

DYNOBJECTS: $(DYNSOFT) $(DYNOCL)

%.o: %.c
	$(CC) -c $(FLAGS) $(PREPROCESSPARMS) -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJECTS) $(LINKER)

clean: clean-obj clean-bin

clean-obj:
	rm -f $(OBJECTS)
	
clean-bin:
	rm -f $(shell find . -maxdepth 1 -executable -type f)

