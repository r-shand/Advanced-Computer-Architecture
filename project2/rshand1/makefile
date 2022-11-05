all:	cache-sim

cache-sim:	Main.o CacheSim.o
	g++ -g Main.o CacheSim.o -o cache-sim

CacheSim.o:	CacheSim.cpp CacheSim.h
	g++ -g -c CacheSim.cpp -o CacheSim.o

Main.o:	Main.cpp CacheSim.h
	g++ -g -c Main.cpp

clean:
	rm -f *.o cache-sim output.txt

run: $(EXECUTABLE)
	./$(EXECUTABLE)
