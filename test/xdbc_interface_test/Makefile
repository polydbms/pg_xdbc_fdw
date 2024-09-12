# Compiler and flags
CC := gcc
CXX := g++
CFLAGS := -Wall -Wextra -I../../include -pthread -I../../submodules/xdbc-client/xdbc
CXXFLAGS := -Wall -Wextra -I../../include -std=c++17 -pthread -I../../submodules/xdbc-client/xdbc -g -O0
CPPOBJS := ../../src/xdbc_interface.o
CPPOBJSOTHER := Test_xdbc_interface_from_cpp.o
COBJS := Test_xdbc_interface.o
LDFLAGS := -lxdbc -pthread -lfmt -lspdlog -lboost_program_options

# Rules
.PHONY: all clean objclean cpp

all: output

cpp: cppout

$(CPPOBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CPPOBJSOTHER): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(COBJS): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

output: $(COBJS) $(CPPOBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

cppout: $(CPPOBJS) $(CPPOBJSOTHER)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean: objclean
	rm -f *.o output cppout

objclean:
	rm -f $(CPPOBJS) $(COBJS)