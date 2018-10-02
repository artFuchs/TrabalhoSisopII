######################### MAKEFILE #########################

######################### Variables ########################

INCLUDE_DIR=include
SRC_DIR=src
BIN_DIR=bin
OUT_DIR=out

CXXFLAGS=-Wall -lpthread -std=c++11 -g
INCLUDE_FLAG=-I./include

ALL_OBJS=${wildcard ${BIN_DIR}/*.o}

SERVER_OBJS=${BIN_DIR}/mainServer.o ${BIN_DIR}/server.o
CLIENT_OBJS=${BIN_DIR}/mainClient.o ${BIN_DIR}/client.o

#############################################################

all: make_dirs server client

clean:
ifneq (${ALL_OBJS},)    # Checks if there is an *.o on ${BIN_DIR}
	@rm ${ALL_OBJS};
endif
	@if [ -f ${OUT_DIR}/server ]; then rm ${OUT_DIR}/server; fi
	@if [ -f ${OUT_DIR}/client ]; then rm ${OUT_DIR}/client; fi

make_dirs:
	@if [ ! -d "./out" ]; then mkdir ${OUT_DIR}; fi;
	@if [ ! -d "./bin" ]; then mkdir ${BIN_DIR}; fi;

#################### Linking executables ####################

server: ${SERVER_OBJS}
	g++ ${SERVER_OBJS} -o out/server ${CXXFLAGS}

client: ${CLIENT_OBJS}
	g++ ${CLIENT_OBJS} -o out/client ${CXXFLAGS}

#############################################################

##################### Compilling source #####################

${BIN_DIR}/mainServer.o: ${SRC_DIR}/mainServer.cpp
	g++ -c ${SRC_DIR}/mainServer.cpp -o ${BIN_DIR}/mainServer.o ${CXXFLAGS} ${INCLUDE_FLAG}

${BIN_DIR}/mainClient.o: ${SRC_DIR}/mainClient.cpp
	g++ -c ${SRC_DIR}/mainClient.cpp -o ${BIN_DIR}/mainClient.o ${CXXFLAGS} ${INCLUDE_FLAG}

${BIN_DIR}/server.o: ${SRC_DIR}/Server.cpp
	g++ -c ${SRC_DIR}/Server.cpp -o ${BIN_DIR}/server.o ${CXXFLAGS} ${INCLUDE_FLAG}

${BIN_DIR}/client.o: ${SRC_DIR}/Client.cpp
	g++ -c ${SRC_DIR}/Client.cpp -o ${BIN_DIR}/client.o ${CXXFLAGS} ${INCLUDE_FLAG}

#############################################################

