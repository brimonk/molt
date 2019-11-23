# Brian Chrzanowski
# Tue Jan 01, 2019 13:51
#
# MOLT Specific (GNU) Makefile

CC = gcc
LINKER = -lm -ldl -lpthread
FLAGS = -Wall -march=native
TARGET = molt
SRC = src/calcs.c src/common.c src/lump.c src/main.c src/sys_linux.c
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d) # one dependency file for each source

ifeq ($(MOLTDEBUG),1)
	FLAGS += -g3
endif

ifeq ($(MOLTVIEWER),1)
	PPARMS := -DMOLT_VIEWER
	SRC += src/viewer.c
	LINKER += -lSDL2 -lGL
endif

all: $(TARGET) moltthreaded.so

%.d: %.c
	@$(CC) $(FLAGS) $(PPARMS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(FLAGS) $(PPARMS) -o $@ $<

-include $(DEP)

$(TARGET): $(OBJ)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJ) $(LINKER)

# this is where we have individual targets for our modules
moltthreaded.so: src/custom/moltthreaded.c src/custom/thpool.c
	$(CC) -fPIC -shared $(FLAGS) -o $@ src/custom/moltthreaded.c src/custom/thpool.c -lm -lpthread

clean: clean-obj clean-bin

clean-obj:
	rm src/*.o src/*.d
	
clean-bin:
	rm $(TARGET) moltthreaded.so

