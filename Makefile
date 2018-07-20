CC = cc
LINKER = -ldl -lpthread -lcurl -lncurses
FLAGS = -Wall -g3
TARGET = molt
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

%.o: %.c
	$(CC) -c $(FLAGS) -o $@ $<

clean: clean-obj clean-bin

clean-obj:
	rm -f $(OBJECTS)
	
clean-bin:
	rm -f $(TARGET)
	
$(TARGET): $(OBJECTS)
	$(CC) $(FLAGS) -o $(TARGET) $(OBJECTS) $(LINKER)
