TARGET 	= sush
CC     	= gcc
FLAGS  	= -Wall
OBJ    	= builtin.o command.o shell.o
INCLUDE = ./include/
SRC		= ./src/
BIN		= ./bin/

# Ensure the bin directory exists
$(shell mkdir -p $(BIN))

all: $(TARGET) 

$(TARGET): my_shell.c $(OBJ) 
	$(CC) $(FLAGS) -o $(TARGET) $(addprefix ${BIN}, $(OBJ)) $<

%.o: ${SRC}%.c ${INCLUDE}%.h
	$(CC) $(FLAGS) -c $< -o ${BIN}$@

.PHONY: clean
clean:
	rm -f ${BIN}*.o ${TARGET} out*
clean_obj:
	rm -f ${BIN}*.o
