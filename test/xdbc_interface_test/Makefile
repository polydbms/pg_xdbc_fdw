# Compiler and flags
CC := gcc
CXX := g++
CFLAGS := -Wall -Wextra -I../../include
CXXFLAGS := -Wall -Wextra -I../../include
CPPOBJS := ../../src/xdbc_interface.o
COBJS := Test_xdbc_interface.o
LDFLAGS := -lxdbc

# Rules
.PHONY: all clean objclean

all: output

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

output: $(COBJS) $(CPPOBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o output

objclean:
	rm -f $(CPPOBJS) $(COBJS)