CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11
OBJ = main.o analyzer.o detector.o reporter.o ipc.o

all: stock_pipeline

stock_pipeline: $(OBJ)
	$(CC) $(CFLAGS) -o stock_pipeline $(OBJ)

main.o: main.c stock.h
	$(CC) $(CFLAGS) -c main.c

analyzer.o: analyzer.c stock.h
	$(CC) $(CFLAGS) -c analyzer.c

detector.o: detector.c stock.h
	$(CC) $(CFLAGS) -c detector.c

reporter.o: reporter.c stock.h
	$(CC) $(CFLAGS) -c reporter.c

ipc.o: ipc.c stock.h
	$(CC) $(CFLAGS) -c ipc.c

clean:
	rm -f *.o stock_pipeline