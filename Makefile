CC := gcc
CXX := g++
MPICXX := mpic++
CFLAGS := -O0 -g -Wall -fpic
CXXFLAGS := -O0 -g -Wall -fpic
INCLUDE := -I./
PMI_MPI_PATH := /usr/src/COBO_TEST/pmi_mpi

pmi_boot_test: pmi_boot_test.o libpmi.so
	$(CXX) $(CXXFLAGS) $^ -o $@ -Wl,-rpath=$(PMI_MPI_PATH) $(PMI_MPI_PATH)/libpmi.so

#pmi_boot_test: pmi_boot_test.o pmi.o map_wrap.o
#	$(MPICXX) $(CXXFLAGS) $^ -o $@ #-Wl,-rpath=/usr/src/COBO_TEST/pmi_mpi /usr/src/COBO_TEST/pmi_mpi/libpmi.so

libpmi.so: pmi.o map_wrap.o
	$(MPICXX) $(CXXFLAGS) -shared $^ -o $@

pmi_boot_test.o: pmi_boot_test.c
	$(CC) $(CFLAGS) $(INCLUDE) $^ -c -o $@	

pmi.o: pmi.cpp reduce.hpp
	$(MPICXX) $(CXXFLAGS) $(INCLUDE) $< -c -o $@

map_wrap.o: map_wrap.cpp
	$(MPICXX) $(CXXFLAGS) $^ -c -o $@

.PHONY: clean

clean:
	rm -f *.~ *.o pmi_boot_test libpmi.so
