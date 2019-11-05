# Brian Chrzanowski Tue Jan 01, 2019 13:51
#
# MOLT Specific (GNU) Makefile

CC = gcc
LINKER = -lm -ldl -lSDL2 -lGL
FLAGS = -Wall -g3 -march=native
TARGET = molt
SRC = src/calcs.c src/common.c src/main.c src/sys_linux.c src/test.c src/viewer.c
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d) # one dependency file for each source

all: $(TARGET) moltthreaded.so

%.d: %.c
	@$(CC) $(FLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(FLAGS) $(PREPROCESSPARMS) -o $@ $<

-include $(DEP)

$(TARGET): $(OBJ)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJ) $(LINKER)

# this is where we have individual targets for our modules
moltthreaded.so: src/custom/moltthreaded.c
	$(CC) -fPIC -shared $(FLAGS) -o $@ $< -lm -lpthread

clean: clean-obj clean-bin

clean-obj:
	rm src/*.o src/*.d
	
clean-bin:
	rm $(TARGET) moltthreaded.so

