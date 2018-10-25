# Makefile
# CS375 Project 5 
# Due:

LDLIBS=-pthread -lrt

all: master reverse upper

master: master.cpp reverse.cpp upper.cpp
	g++ -o master master.cpp sh_com.h $(LDLIBS)

reverse: reverse.cpp
	g++ -o reverse reverse.cpp sh_com.h $(LDLIBS)

upper: upper.cpp
	g++ -o upper upper.cpp sh_com.h $(LDLIBS)