CC = gcc
CFLAGS = -g -std=c99 -I./
SRCCLIENT = .
SRCSERVER = .
DIRCLIENT = client
DIRSERVER = server
OBJCLIENT = obj
OBJSERVER = obj
OBJECTSCLIENT = ${OBJCLIENT}/commons.o ${OBJCLIENT}/client.o
OBJECTSSERVER = ${OBJSERVER}/commons.o ${OBJSERVER}/server.o
EXECUTABLECLIENT = ${DIRCLIENT}/client
EXECUTABLESERVER = ${DIRSERVER}/server

all:	clean client server

client:	${OBJECTSCLIENT}
	${CC} ${CFLAGS} $^ -o ${EXECUTABLECLIENT}

server:	${OBJECTSSERVER}
	${CC} ${CFLAGS} $^ -o ${EXECUTABLESERVER}

${OBJCLIENT}/%.o:	${SRCCLIENT}/%.c
	${CC} ${CFLAGS} -c $< -o $@

${OBJSERVER}/%.o:	${SRCSERVER}/%.c
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm -f ${OBJCLIENT}/*.o
	rm -f ${OBJSERVER}/*.o
	rm -f ${EXECUTABLECLIENT}
	rm -f ${EXECUTABLESERVER}
