OBJS = TrafficSignalManager.o
CC = g++ -pthread
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

main:	$(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o TrafficSignalManager

TrafficSignalManager.o:	TrafficSignalManager.cpp
	$(CC) $(CFLAGS) TrafficSignalManager.cpp

clean:
	\rm *o  TrafficSignalManager
