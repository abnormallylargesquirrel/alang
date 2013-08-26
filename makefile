EXE=alang
OBJECTS=utils.o lexer.o parser.o jit_engine.o alib.o alang.o

LLVM_CONFIG=/home/alan/llvm/build/Release+Asserts/bin/llvm-config
LLVM_MODULES=core jit native

#LLVMCPPFLAGS=$(shell $(LLVM_CONFIG) --cxxflags)
LLVMCPPFLAGS=-isystem/home/alan/llvm/include -isystem/home/alan/llvm/build/include  -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual
LLVMLDFLAGS=$(shell $(LLVM_CONFIG) --ldflags) $(shell $(LLVM_CONFIG) --libs)

SUPPRESS_WARNINGS=-Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-weak-vtables -Wno-missing-prototypes -Wno-padded -Wno-implicit-fallthrough

CC=clang++
CFLAGS=-c -g -Weverything -std=c++11 -fPIC $(SUPPRESS_WARNINGS) $(LLVMCPPFLAGS)
LDFLAGS=$(LLVMLDFLAGS)
all: $(OBJECTS) link

clean:
	rm *.o $(EXE)

link:
	$(CC) -o $(EXE) $(OBJECTS) $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
