# Made by Marcin Piniarski

CC = gcc

INC_PATH = ./Inc/
SRC_PATH = ./Src/

#Set up flags:
CFLAGS  = -c -Wall -g
LFLAGS = -Wall -g

#Set up included header files
INCLUDES = -I$(INC_PATH) 

#Set up sources of project
CSOURCES=$(shell (find -L $(SRC_PATH) -name '*.c' | sed -e 's!^./Src!./Res!' -e 's/\.c$$/\.o/' ))


SERVER_DEP= Res/server.o Res/my_semafors.o Res/myipc.o
CLIENT_DEP= Res/client.o Res/kbhit.o Res/myipc.o


default: Res/server.out Res/client.out 

#Compile:
Res/%.o: ./Src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@
	@echo "Compiled "$<"!"

#Link: 
Res/server.out: $(SERVER_DEP)
	$(CC) $(LFLAGS) $(INCLUDES) -o $@ $(SERVER_DEP)
	@echo "Created "$@"!"

Res/client.out: $(CLIENT_DEP)
	$(CC) $(LFLAGS) $(INCLUDES) -o $@ $(CLIENT_DEP) 
	@echo "Created "$@"!"

clean:
	rm -f ./Res/* 

















