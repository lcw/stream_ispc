OBJDIR  := build
UNAME_S := $(shell uname -s)

# CC=icc
# CFLAGS=-std=gnu99 -g -xHOST -O3 -ffreestanding -openmp

# CXX=icpc
# CXXFLAGS=-g -xHOST -O3 -ffreestanding -openmp -DISPC_USE_OMP

 CC=gcc
 CFLAGS=-std=gnu11 -Wall -Wextra -Wpedantic -O3 -march=native -fopenmp -fno-omit-frame-pointer -ffreestanding

 CXX=g++
 CXXFLAGS=-std=c++11 -Wall -Wextra -Wpedantic -O3 -march=native -fopenmp -fno-omit-frame-pointer -DISPC_USE_OMP

ISPC = ispc
ISPCFLAGS = --target=avx2-i32x8 --pic --opt=force-aligned-memory

ifeq ($(UNAME_S),Darwin)
  CFLAGS += -Wa,-q
  CXXFLAGS += -Wa,-q
endif

ifeq ($(UNAME_S),Linux)
  LDLIBS += -lrt
endif


CHUNK ?=16384
STREAM_DEFINES ?= -DVERBOSE= -DSTREAM_TYPE=float -DSTREAM_ARRAY_SIZE=80000000 -DCHUNK=$(CHUNK)

all: $(OBJDIR)/stream $(OBJDIR)/stream_ispc

clean:
	rm -rf "$(OBJDIR)"

%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

$(OBJDIR)/%.o: %.ispc
	$(ISPC) $(ISPCFLAGS) -o $@ $^

$(OBJDIR):
	mkdir -p $@


$(OBJDIR)/stream:      CFLAGS+=-mcmodel=medium $(STREAM_DEFINES)
$(OBJDIR)/stream_ispc: CFLAGS+=$(STREAM_DEFINES)
$(OBJDIR)/stream_ispc: ISPCFLAGS+=$(STREAM_DEFINES)
$(OBJDIR)/stream_ispc: LDLIBS+=-lstdc++

# Dependencies
$(OBJDIR)/stream: $(OBJDIR)/stream.o | $(OBJDIR)
$(OBJDIR)/stream.o: stream.c | $(OBJDIR)

$(OBJDIR)/stream_ispc: $(OBJDIR)/stream_ispc.o $(OBJDIR)/stream_tasks.o $(OBJDIR)/tasksys.o | $(OBJDIR)
$(OBJDIR)/stream_ispc.o: stream_ispc.c | $(OBJDIR)
$(OBJDIR)/stream_tasks.o: stream_tasks.ispc | $(OBJDIR)
$(OBJDIR)/tasksys.o: tasksys.cpp | $(OBJDIR)
