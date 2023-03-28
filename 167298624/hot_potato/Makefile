CCFLAGS=-Wall  -Werror -pedantic -ggdb3 -std=gnu++98
PROGS=ringmaster player
all: $(PROGS)

ringmaster: ringmaster.o socketutils.o potato.o
	g++ -std=c++98 -o ringmaster $(CCFLAGS) socketutils.o potato.o ringmaster.o

player: player.o socketutils.o potato.o
	g++ -std=c++98 -o player $(CCFLAGS) socketutils.o potato.o player.o

%.o: %.cpp socketutils.h potato.hpp
	g++ -std=c++98 $(CCFLAGS) -c $<

clean:
	rm -f *.o *- ringmaster player
