CC = gcc
CFLAGS = -ggdb3

all: wolfvoitool

wolfvoitool: wolfvoitool.c wolfvoitool.h voi.c voi.h vbios-tables.h
	$(CC) $(CFLAGS) wolfvoitool.c voi.c -o wolfvoitool

clean:
	rm -f wolfvoitool
