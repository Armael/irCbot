CC = gcc
CFLAGS = -Wall -O0 -g -D_REENTRANT
LIBS = /usr/lib/libircclient.a -lnsl -lpthread
INCLUDES=-I/usr/share/libircclient/include

TARGET=bot

all: $(TARGET)

bot:	bot.o
	$(CC) -o bot bot.o $(LIBS)

clean:
	rm *.o

mproper:	clean
	rm $(TARGET)

distclean:  mproper
	rm *.log

.c.o:
	@$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<
