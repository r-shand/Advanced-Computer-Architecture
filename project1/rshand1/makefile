all:	predictors

predictors:	Main.o
	g++ Main.o -o predictors

Main.o:	Main.cpp
	g++ -c Main.cpp

clean:
	rm -f *.o predictors output.txt

run: $(EXECUTABLE)
	./$(EXECUTABLE)
