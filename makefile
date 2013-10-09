EXE=alang
#OBJECTS=utils.o lexer.o parser.o func_manager.o reducer.o alib.o alang.o
OBJECTS=utils.o lexer.o parser.o func_manager.o reducer.o alib.o ast.o hm_inference.o hm_unification.o alang.o
#OBJECTS=utils.o lexer.o parser.o jit_engine.o func_manager.o alib.o alang.o

LLVM_CONFIG=/home/alan/llvm/build/Release+Asserts/bin/llvm-config
LLVM_MODULES=core jit native

#LLVMCPPFLAGS=$(shell $(LLVM_CONFIG) --cxxflags)

#no exceptions, no rtti
#LLVMCPPFLAGS=-isystem/home/alan/llvm/include -isystem/home/alan/llvm/build/include  -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -Wcast-qual

#no rtti
LLVMCPPFLAGS=-isystem/home/alan/llvm/include -isystem/home/alan/llvm/build/include  -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -fvisibility-inlines-hidden -fno-rtti -fPIC -Wcast-qual

#no exceptions
#LLVMCPPFLAGS=-isystem/home/alan/llvm/include -isystem/home/alan/llvm/build/include  -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -fvisibility-inlines-hidden -fno-exceptions -fPIC -Wcast-qual

LLVMLDFLAGS=$(shell $(LLVM_CONFIG) --ldflags) $(shell $(LLVM_CONFIG) --libs $(LLVM_MODULES))

SUPPRESS_WARNINGS=-Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-weak-vtables -Wno-missing-prototypes -Wno-padded -Wno-implicit-fallthrough -Wno-switch-enum -Wno-overloaded-virtual

CC=clang++
#CC=g++
CFLAGS=-c -g -Weverything -std=c++11 -fPIC $(SUPPRESS_WARNINGS) $(LLVMCPPFLAGS)
#CFLAGS=-c -g -Wall -std=c++11 $(LLVMCPPFLAGS)
LDFLAGS=$(LLVMLDFLAGS)
all: $(OBJECTS) link

clean:
	rm *.o $(EXE)

link:
	$(CC) -o $(EXE) $(OBJECTS) $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
