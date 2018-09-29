
SRC=src/ks/*.cpp
INC=include/ks/*.h

libks.a: $(SRC) $(INC)
	g++ -Wall -c -Iinclude -O3 $(SRC) && ar rvs $@ *.o

libks.dylib: $(SRC) $(INC)
	g++ -Wall -Wl,-dylib,-o,libks.dylib -Iinclude -O3 $(SRC)

static: libks.a

dynamic: libks.dylib

clean:
	rm -f *.o

distclean: clean
	rm -f *.a *.dylib

