# Brian Chrzanowski
# Tue Jan 01, 2019 13:51
#
# MOLT Specific (GNU) Makefile

CC=gcc
LINKER=-lm -ldl -lpthread
CFLAGS=-Wall -g3 -march=native
TARGET=molt
SRC=src/lump.c src/main.c src/sys_linux.c
OBJ=$(SRC:.c=.o)
DEP=$(OBJ:.o=.d) # one dependency file for each source

all: $(TARGET) moltthreaded.so experiments/test

%.d: %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

-include $(DEP)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LINKER)

# this is where we have individual targets for our modules
moltthreaded.so: src/custom/moltthreaded.c src/custom/thpool.c
	$(CC) -fPIC -shared $(CFLAGS) -o $@ src/custom/moltthreaded.c src/custom/thpool.c -lm -lpthread

experiments/test: experiments/test.c
	$(CC) $(CFLAGS) -o $@ $^ $(LINKER)

clean: clean-obj clean-bin

clean-obj:
	rm -f src/*.o src/*.d
	
clean-bin:
	rm -f $(TARGET) moltthreaded.so experiments/test

