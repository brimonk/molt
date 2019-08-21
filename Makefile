# Brian Chrzanowski Tue Jan 01, 2019 13:51
#
# MOLT Specific (GNU) Makefile

CC = gcc
# LINKER = -lm -lSDL2 -lGL
LINKER = -LC:\users\brian\desktop\build\SDL\lib -LC:\MinGW\lib -LC:\users\brian\desktop\build\glew\lib -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lglew32
FLAGS = -Wall -g3 -march=native -IC:\Users\brian\Desktop\build\SDL\include -IC:\users\brian\desktop\build\glew\include
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
	rm -f $(OBJ) $(DEP)
	
clean-bin:
	rm -f $(shell find . -maxdepth 1 -executable -type f)

