# Brian Chrzanowski Tue Jan 01, 2019 13:51
#
# MOLT Specific (GNU) Makefile

CC = gcc
# LINKER = -lm -lSDL2 -lGL
LINKER = -lmingw32 -lSDL2main -lSDL2 -lopengl32
FLAGS = -Wall -O3 -g3 -march=native
TARGET = molt
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d) # one dependency file for each source

ifeq ($(OS),Windows_NT)
	# windows is special :)
	RMCMD := del
	PATHD := \\
    CCFLAGS += -D WIN32
	TARGET := $(TARGET).exe
	LINKER = -lm -lmingw32 -lSDL2main -lSDL2 -lopengl32
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        CCFLAGS += -D AMD64
    else
        ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
            CCFLAGS += -D AMD64
        endif
        ifeq ($(PROCESSOR_ARCHITECTURE),x86)
            CCFLAGS += -D IA32
        endif
    endif
else
	RMCMD := rm
	PATHD := /
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CCFLAGS += -D LINUX
    endif
    ifeq ($(UNAME_S),Darwin)
        CCFLAGS += -D OSX
    endif
    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)
        CCFLAGS += -D AMD64
    endif
    ifneq ($(filter %86,$(UNAME_P)),)
        CCFLAGS += -D IA32
    endif
    ifneq ($(filter arm%,$(UNAME_P)),)
        CCFLAGS += -D ARM
    endif
endif

all: $(TARGET)

%.d: %.c
	@$(CC) $(FLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(FLAGS) $(PREPROCESSPARMS) -o $@ $<

-include $(DEP)

$(TARGET): $(OBJ)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJ) $(LINKER)

clean: clean-obj clean-bin

clean-obj:
	$(RMCMD) src$(PATHD)*.o src$(PATHD)*.d $(RESO)
	
clean-bin:
	$(RMCMD) $(TARGET)

