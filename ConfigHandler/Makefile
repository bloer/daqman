#Makefile for ConfigHandler utility library

#Copyright 2013 Ben Loer
#This file is part of the ConfigHandler library
#It is released under the terms of the GNU General Public License v3

#This library requires boost v1.35 or greater for shared pointers
#Additionally, by default the MessageHandler library requires the boost_thread
#library.  You can call 'make MULTITHREAD=false' in order to bypass this 
#requirement. Message functions will no longer be thread-safe in this case

SOURCE    := test.cc $(shell find src -name '*.cc' | sort)
OBJECTS   := $(SOURCE:%.cc=%.o)
HEADERS   := $(shell find . -name '*.hh')

CXXFLAGS  += $(BUILDFLAGS) -g -Wall -I$(PWD)/include -fPIC -pthread 
ifneq  ($(MULTITHREAD),false)
CXXFLAGS  += -DMULTITHREAD
LIBFLAGS  += -lboost_thread
endif
LDFLAGS   += $(BUILDFLAGS) -g


ifeq ($(shell uname),Darwin)
	LIBFLAGS += -dynamiclib -single_module -undefined dynamic_lookup
	LIBFLAGS += -install_name $(PWD)/lib/$@
else
	LIBFLAGS += -Wl,-soname,$@ -shared 
endif


all: libMessageHandler.so libParameterList.so libConfigHandler.so 

test: test.o libConfigHandler.so
	@echo " [LD] $@"
	@$(CXX) $(LDFLAGS) test.o -Llib -lConfigHandler -Wl,-rpath,$(PWD)/lib \
	-o $@ 
	@echo "Make complete! Try running ./test binary (./test -h for help)."

libMessageHandler.so: src/MessageHandler.o
	@echo " Generating shared library $@"
	@$(CXX) $(LDFLAGS) $(LIBFLAGS) $^ -o $@
	@mv $@ lib/$@

libParameterList.so: src/MessageHandler.o src/VParameterNode.o src/ParameterList.o src/ParameterIOimpl.o
	@echo " Generating shared library $@"
	@$(CXX) $(LDFLAGS) $(LIBFLAGS) $^ -o $@
	@mv $@ lib/$@

libConfigHandler.so: src/MessageHandler.o src/VParameterNode.o src/ParameterList.o src/ParameterIOimpl.o src/ConfigHandler.o
	@echo " Generating shared library $@"
	@$(CXX) $(LDFLAGS) $(LIBFLAGS) $^ -o $@
	@mv $@ lib/$@

$(OBJECTS): %.o: %.cc
	@echo " [CXX] $<" && $(CXX) $(CXXFLAGS) $< -c -o $@ > /dev/null
#Dependencies 
-include .deps
deps: .deps
.deps: $(HEADERS) $(SOURCE)
	@touch $@
	@makedepend $(filter-out -pthread -I/usr%, $(CXXFLAGS)) -Y \
		-f $@ $(SOURCE) 2>/dev/null

clean:
	find . \( -name \*.o -o -name \*.ii -o -name \*.s \) -exec rm \{\} \;

distclean: clean
	rm -f lib/lib* test
	rm -f .deps .deps.bak

.PHONY: clean distclean deps doc
