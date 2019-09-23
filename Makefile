# Brian Chrzanowski Tue Jan 01, 2019 13:51
#
# MOLT Specific (GNU) Makefile

CC = gcc
LINKER = -lm -lSDL2 -lGL
FLAGS = -Wall -g3 -march=native
TARGET = molt
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(OBJ:.o=.d) # one dependency file for each source

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
	rm src/*.o src/*.d
	
clean-bin:
	rm $(TARGET)

