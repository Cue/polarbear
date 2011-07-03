# Source lists
LIBNAME=outOfMemory
SOURCES=outOfMemory.cc

LINK.cxx    = $(CXX) $(MY_CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS)

# Solaris Sun C Compiler Version 5.5
ifeq ($(OSNAME), solaris)
    # Sun Solaris Compiler options needed
    COMMON_FLAGS=-mt -xregs=no%appl -KPIC
    # Options that help find errors
    COMMON_FLAGS+= -Xa -v -xstrconst -xc99=%none
    # Check LIBARCH for any special compiler options
    LIBARCH=$(shell uname -p)
    ifeq ($(LIBARCH), sparc)
        COMMON_FLAGS+=-xarch=v8
    endif
    ifeq ($(LIBARCH), sparcv9)
        COMMON_FLAGS+=-xarch=v9
    endif
    ifeq ($(OPT), true)
        CXXFLAGS=-xO2 $(COMMON_FLAGS)
    else
        CXXFLAGS=-g $(COMMON_FLAGS)
    endif
    # Object files needed to create library
    OBJECTS=$(SOURCES:%.cc=%.o)
    # Library name and options needed to build it
    LIBRARY=lib$(LIBNAME).so
    LDFLAGS=-z defs -ztext
    # Libraries we are dependent on
    LIBRARIES= -lc
    # Building a shared library
    LINK_SHARED=$(LINK.cxx) -G -o $@
endif

# Linux GNU C Compiler
ifeq ($(OSNAME), linux)
    # GNU Compiler options needed to build it
    COMMON_FLAGS=-fno-strict-aliasing -fPIC -fno-omit-frame-pointer
    # Options that help find errors
    COMMON_FLAGS+= -W -Wall  -Wno-unused -Wno-parentheses
    ifeq ($(OPT), true)
        CXXFLAGS=-O2 $(COMMON_FLAGS)
    else
        CXXFLAGS=-g $(COMMON_FLAGS)
    endif
    # Object files needed to create library
    OBJECTS=$(SOURCES:%.cc=%.o)
    # Library name and options needed to build it
    LIBRARY=lib$(LIBNAME).so
    LDFLAGS=-Wl,-soname=$(LIBRARY) -static-libgcc -mimpure-text
    # Libraries we are dependent on
    LIBRARIES=-lc
    # Building a shared library
    LINK_SHARED=$(LINK.cxx) -shared -o $@
endif

# Mac OSX
ifeq ($(OSNAME), darwin)
    # GNU Compiler options needed to build it
    COMMON_FLAGS=-fno-strict-aliasing -fPIC -fno-omit-frame-pointer
    # Options that help find errors
    COMMON_FLAGS+= -W -Wall  -Wno-unused -Wno-parentheses
    ifeq ($(OPT), true)
        CXXFLAGS=-O2 $(COMMON_FLAGS)
    else
        CXXFLAGS=-g $(COMMON_FLAGS)
    endif
    # Object files needed to create library
    OBJECTS=$(SOURCES:%.cc=%.o)
    # Library name and options needed to build it
    LIBRARY=lib$(LIBNAME).jnilib
    LDFLAGS=-Wl-static-libgcc -mimpure-text
    # Libraries we are dependent on
    LIBRARIES=-lc
    # Building a shared library
    LINK_SHARED=$(LINK.cxx) -dynamiclib -single_module -undefined suppress -flat_namespace -o $(LIBRARY)
endif

# Windows Microsoft C/C++ Optimizing Compiler Version 12
ifeq ($(OSNAME), win32)
    CC=cl
    # Compiler options needed to build it
    COMMON_FLAGS=-Gy
    # Options that help find errors
    COMMON_FLAGS+=-W0 -WX
    ifeq ($(OPT), true)
        CXXFLAGS= -Ox -Op -Zi $(COMMON_FLAGS)
    else
        CXXFLAGS= -Od -Zi $(COMMON_FLAGS)
    endif
    # Object files needed to create library
    OBJECTS=$(SOURCES:%.cc=%.obj)
    # Library name and options needed to build it
    LIBRARY=$(LIBNAME).dll
    LDFLAGS=
    # Libraries we are dependent on
    LIBRARIES=
    # Building a shared library
    LINK_SHARED=link -dll -out:$@
endif

# Common -I options
CXXFLAGS += -I.
CXXFLAGS += -I$(J2SDK)/include -I$(J2SDK)/include/$(OSNAME)

%.class: %.java
	javac $<

# Default rule
all: $(LIBRARY)

# Build native library
$(LIBRARY): $(OBJECTS)
	$(LINK_SHARED) $(OBJECTS) $(LIBRARIES)

# Cleanup the built bits
clean:
	rm -f $(LIBRARY) $(OBJECTS)

# Simple tester
test: all Test.class
	rm -f /tmp/oom.log
	LD_LIBRARY_PATH=`pwd` $(J2SDK)/bin/java -Xms50m -Xmx50m -agentlib:$(LIBNAME)=HashMap,OOMList Test || cat '/tmp/oom.log'

# Compilation rule only needed on Windows
ifeq ($(OSNAME), win32)
%.obj: %.cc
	$(COMPILE.c) $<
endif

