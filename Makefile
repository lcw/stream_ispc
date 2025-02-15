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

all: $(OBJDIR)/stream $(OBJDIR)/stream_ispc $(OBJDIR)/stream_ispc_loopy

clean:
	rm -rf "$(OBJDIR)" loopy-venv

%: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

$(OBJDIR)/%.o: %.ispc
	$(ISPC) $(ISPCFLAGS) -o $@ $^

%.o: %.ispc
	$(ISPC) $(ISPCFLAGS) -o $@ $^

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/stream_tasks_loopy.ispc: loopy-venv $(OBJDIR)
	loopy-venv/bin/python3 gen-loopy.py $(STREAM_DEFINES) > $@

$(OBJDIR)/stream:      CFLAGS+=-mcmodel=medium $(STREAM_DEFINES)
$(OBJDIR)/stream_ispc: CFLAGS+=$(STREAM_DEFINES)
$(OBJDIR)/stream_ispc: ISPCFLAGS+=$(STREAM_DEFINES)
$(OBJDIR)/stream_ispc: LDLIBS+=-lstdc++

$(OBJDIR)/stream_ispc_loopy: CFLAGS+=$(STREAM_DEFINES)
$(OBJDIR)/stream_ispc_loopy: ISPCFLAGS+=$(STREAM_DEFINES)
$(OBJDIR)/stream_ispc_loopy: LDLIBS+=-lstdc++

# Dependencies
$(OBJDIR)/stream: $(OBJDIR)/stream.o | $(OBJDIR)
$(OBJDIR)/stream.o: stream.c | $(OBJDIR)

$(OBJDIR)/stream_ispc: $(OBJDIR)/stream_ispc.o $(OBJDIR)/stream_tasks.o $(OBJDIR)/tasksys.o | $(OBJDIR)
$(OBJDIR)/stream_ispc.o: stream_ispc.c | $(OBJDIR)
# Create object file with the appropriate name so the binary will get linked
# using the rule above.
$(OBJDIR)/stream_ispc_loopy.o: $(OBJDIR)/stream_ispc.o
	cp  $(OBJDIR)/stream_ispc.o $(OBJDIR)/stream_ispc_loopy.o
$(OBJDIR)/stream_ispc_loopy: $(OBJDIR)/stream_ispc_loopy.o $(OBJDIR)/stream_tasks_loopy.o $(OBJDIR)/tasksys.o | $(OBJDIR)
$(OBJDIR)/stream_tasks.o: stream_tasks.ispc | $(OBJDIR)
$(OBJDIR)/tasksys.o: tasksys.cpp | $(OBJDIR)

loopy-venv:
	python3 -m venv loopy-venv
	loopy-venv/bin/python3 -m pip install git+https://github.com/inducer/loopy.git
