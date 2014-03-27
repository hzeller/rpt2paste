CXXFLAGS=-Wall
rpt2paste: main.o rpt-parser.o
	g++ -o $@ $^

clean:
	rm -f *.o rpt2paste