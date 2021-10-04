#Makefile for daqman system
#IMPORTANT:
#To compile without threads, do 'make MULTITHREAD=false' 
#You can't compile the daq parts without threads, but others should work
#
# To get all featers, you need to have the CAENVME libraries, boost, and 
# mysql++-dev all installed, with headers and libraries in the default 
# system areas

#make sure makedepend is installed
MAKEDEPEND = $(shell which makedepend)
ifeq ("$(MAKEDEPEND)","")
$(error 'makedepend' is required for this Makefile to work properly)
endif

#find the root version
ROOTVERSION = $(shell root-config --version)
#ROOTVERSION := $(dir $(ROOTVERSION))

#find all the cc files, regardless of subdirectory except test.cc,  WarpCrateIO
CODE        := $(shell find . -name '*.cc' | grep -v 'ConfigHandler/test.cc' | \
		grep -v 'WarpCrateIO' | sort)
ifneq ("$(ROOTVERSION)","5.34/19")
CODE := $(filter-out ./modules/src/TTreeFormula.cc,$(CODE))
endif

#DATABASES: check for db libraries here and remove otherwise
ifeq ("$(shell locate mongoclient)","")
CODE := $(filter-out ./database/src/MongoDBInterface.cc,$(CODE))
else
LIBS += -lmongoclient 
endif

#all .cc files in exe/ will make executables
MAIN_CODE   := $(shell find ./exe -name '*.cc' | sort)
#all main code links against all others
COMMON_CODE := $(filter-out $(MAIN_CODE),$(CODE))

#generate names of object files
OBJS        := $(CODE:%.cc=%.o)
MAIN_OBJS   := $(MAIN_CODE:%.cc=%.o)
COMMON_OBJS := $(COMMON_CODE:%.cc=%.o) 
BIN         := $(MAIN_CODE:./exe/%.cc=bin/%)

#we might not want threads,so find the parts that absolutely need it
THREADCODE  := %BaseDaq.cc %V172X_Daq.cc %V172X_Daq_Helpers.cc
THREADOBJS  := $(THREADCODE:%.cc=%.o)

#find all headers, even in subdirectories
HEADERS     := $(shell find . -name '*.h') $(shell find . -name '*.hh') 
HEADERS     := $(filter-out ./libdaqman/include/%,$(HEADERS))
HEADERS     := $(filter-out ./doc/%,$(HEADERS))

#find all include directories, add them to the include path
INCLUDEDIRS := $(shell find $(PWD) -name 'include' -type d | \
		 grep -v 'WarpCrateIO' | grep -v 'libdaqman/include' )
INCLUDES    := $(addprefix -I,$(INCLUDEDIRS))
#add the current directory
INCLUDES    += -I$(PWD)
ROOTINCLUDEPATH := $(INCLUDES)
#add fink if we're on a mac system
INCLUDES    += -I/sw/include -I$(PWD) 
#some systems want malloc
INCLUDES    += -I/usr/include/malloc


#generic c++ and ld flags
CXXFLAGS    += $(DEBUGFLAGS) -O3 -g -Wall $(INCLUDES)  -Wno-format-y2k -fPIC
CXXFLAGS    += -DROOTINCLUDEPATH="\"$(ROOTINCLUDEPATH)\""
CXXFLAGS    += -DDAQMANBUILDDIR="\"$(PWD)\""
LDFLAGS     += -lz $(DEBUGFLAGS)

#export the whole base as a shared library
BUILDLIBS   := lib/libdaqman.so

#some more optional libraries
CAENLIBS    := -L/usr/local/lib64 -lCAENVME -lCAENDigitizer

#hardcode the path to link against daqman
LIBS     += -Wl,-rpath,$(PWD)/lib 

#find what OS we're on, determine shared library commands
UNAME = "$(shell uname -s)"
ifeq ($(UNAME),"Linux") 
CXXFLAGS += -DLINUX
LIBFLAGS += -Wl,-soname,$(notdir $@) -shared 
#don't do CAEN stuff if the CAENVME librry is not installed 
ifeq ("$(shell /sbin/ldconfig -p | grep CAENDigitizer)","")
SKIPCAEN = true
endif
else 
ifeq ($(UNAME),"Darwin")
CXXFLAGS += -DMAC_OSX -DLINUX
LDFLAGS  += -L/sw/lib
LIBFLAGS += -dynamiclib -single_module -undefined dynamic_lookup
LIBFLAGS += -install_name $(PWD)/lib/$@
#argus is retared and ignores rpath, looks in lib/lib for libraries?????
#so do a terrible recursive soft link 
DUMMY := $(shell ln -s $(PWD)/lib lib/lib 2>/dev/null)

#no CAEN library for macs
SKIPCAEN = true
endif
endif

#If CAENVME library is not installed, skip that part
ifeq ($(SKIPCAEN),true)
NEEDCAEN     := $(shell $(MAKEDEPEND) $(filter-out -pthread -I/usr% -I/sw%, $(CXXFLAGS)) -Y \
		    -f- $(CODE) 2>/dev/null | grep CAENVME)
#NEEDCAEN     := $(shell grep CAENVME .deps)
NEEDCAEN     := $(filter %.o:,$(NEEDCAEN))
NEEDCAEN     := $(sort $(NEEDCAEN))
NEEDCAEN     := $(patsubst %.o:,%.o,$(NEEDCAEN))
OBJS         := $(filter-out $(NEEDCAEN),$(OBJS))
COMMON_OBJS  := $(filter-out $(NEEDCAEN),$(COMMON_OBJS))
BIN	     := $(filter-out $(NEEDCAEN:./exe/%.o=bin/%),$(BIN))
else
LIBS         += $(CAENLIBS)
endif

#Do we use threads?
ifneq ($(MULTITHREAD),false)
LIBS        += $(THREADLIBS) 
else
CXXFLAGS    += -DSINGLETHREAD
CODE        := $(filter-out $(THREADCODE),$(CODE))
COMMON_CODE := $(filter-out $(THREADCODE),$(COMMON_CODE))
OBJS        := $(filter-out $(THREADOBJS),$(OBJS))
COMMON_OBJS := $(filter-out $(THREADOBJS),$(COMMON_OBJS))
BIN         := $(filter-out daqman,$(BIN))
endif

#Do we use root?
ifneq ($(shell which root),"")
VPATH       += $(shell root-config --libdir)
CXXFLAGS    += $(shell root-config --cflags)
LIBS        += $(shell root-config --glibs) -lThread -lSpectrum -lTreePlayer
#rules to generate the root dictionaries for custom storage classes
DICT        := Dictionaries.cc
DICTO       := $(DICT:.cc=.o)
OBJS        := $(filter-out %$(DICTO),$(OBJS))
OBJS        += $(DICT:.cc=.o)
COMMON_OBJS := $(filter-out %$(DICTO),$(COMMON_OBJS))
COMMON_OBJS += $(DICT:.cc=.o)
#and header with the phrase ClassDef in it will get added here
DICTHEADS   := $(shell grep -l ClassDef $(HEADERS))
#DICTHEADS   += LinkDef.h
DICTOBJS    := $(subst include,src,$(DICTHEADS))
DICTOBJS    := $(DICTO) $(DICTOBJS:.hh=.o)
DICTOBJS    := $(filter $(DICTOBJS),$(OBJS))
#DICTCALLS   := $(addsuffix +,$(DICTHEADS))
#build a library of our dictionary classes
BUILDLIBS   += lib/libDict.so lib/Dictionaries_rdict.pcm
endif

all: $(DICT) libs $(BIN) 

doc:
	@echo "Generating documentation..."
	@doxygen doc/doxygen.cfg > /dev/null 2>&1

lib:
	mkdir -p $@
libs: lib $(BUILDLIBS) 
lib/libdaqman.so:  $(COMMON_OBJS)  
	@echo "  [LD]  $@" 
	@$(CXX) $(LDFLAGS) $(LIBFLAGS) $(LIBS) $^ -o $@ > /dev/null

lib/libDict.so: $(DICTOBJS)
	@echo "  [LD]  $@"
	@$(CXX) $(LDFLAGS) $(LIBFLAGS) $(LIBS) $^ -o $@ > /dev/null

lib/Dictionaries_rdict.pcm: $(DICT)
	mv $(notdir $@) $@

LinkDef.h: $(DICTHEADS) scripts/generateLinkDef.sh
	@echo "  Generating $@..."
	@./scripts/generateLinkDef.sh >$@

$(DICT): $(DICTHEADS) LinkDef.h
	@echo "  [ROOTCINT] $@"
	rootcint -f $(INCLUDES) $@ $^ 

$(BIN): bin/%: exe/%.o lib/libdaqman.so  
	@echo "  [LD]  $@" 
	@mkdir -p bin
	@$(CXX) $(filter-out bindir,$^) $(LDFLAGS) $(LIBS) -o $@ > /dev/null

$(OBJS): %.o: %.cc 
	@echo "  [CXX] $<" 
	@$(CXX) $(CXXFLAGS) $< -c -o $@ 

# Dependencies
-include .deps

#use makedepend to generate the header dependencies
deps: .deps
.deps: $(HEADERS) $(CODE)
	@touch $@
	@$(MAKEDEPEND) $(filter-out -pthread -I/usr% -I/sw%, $(CXXFLAGS)) -Y \
		-f $@ $(CODE) 2>/dev/null

#remove all object files
clean:
	find . \( -name \*.o -o -name \*.ii -o -name \*.s \) -exec rm \{\} \;

#remove object files, plus binaries, libraries, dictionaries, deps
distclean: clean
	rm -rf libdaqman
	rm -f $(BIN)
	rm -rf lib bin
	rm -f LinkDef.h
	rm -f $(DICT:%.cc=%.*)
	rm -f .deps .deps.bak
	rm -rf doc/html doc/latex

.PHONY: clean distclean deps doc checkrootversion

# DO NOT DELETE
