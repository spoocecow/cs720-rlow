# Mark Niemeyer
# CS 720 Assignment 4 11.28.09

CC = gcc
LDFLAGS = -lrt

CFLAGS = -Wall -pedantic -ansi -O -g

EXECUTABLES = rlowc rlows

all:	$(EXECUTABLES)

#rlowc:	rlowc.o low-netbase.o low-base.o low-file.o low-stats.o print_time.o tcpblockio.o no_sigpipe.o


#rlows:	rlows.o low-netbase.o low-base.o low-file.o low-stats.o print_time.o tcpblockio.o no_sigpipe.o

rlowc:	rlowc.o low-netbase.o low-base.o low-file.o low-stats.o print_time.o tcpblockio.o no_sigpipe.o
rlows:	rlows.o low-netbase.o low-base.o low-file.o low-stats.o print_time.o tcpblockio.o no_sigpipe.o

rlowc.o:	rlowc.h

rlows.o:	rlows.h

low-base.o:	low-base.h 

low-file.o:	low-file.h

low-stats.o:	low-stats.h

low-netbase.o:	low-netbase.h

print_time.o:	print_time.h

tcpblockio.o:	tcpblockio.h

no_sigpipe.o:	no_sigpipe.h

.PHONY:	clean

clean:
	$(RM) $(EXECUTABLES) *.o *.gch
