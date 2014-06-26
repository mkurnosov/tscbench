#
# makefile
#

tscbench := tscbench
tscbench_objs := tscbench.o tsc_x86.o mathstat.o measured_code.o

tests := tests
tests_objs := tsc_x86.o mathstat.o tests.o 

CC := gcc
LD := gcc
CFLAGS := -Wall -std=c99 -O2
MEASURED_CODE_CFLAGS := -Wall -std=c99 -O2
LDFLAGS := -std=c99 -lm

.PHONY: all clean

all: $(tscbench) $(tests)

$(tscbench): $(tscbench_objs)
	$(LD) -o $@ $^ $(LDFLAGS)

$(tests): $(tests_objs)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

measured_code.o: measured_code.c
	$(CC) $(MEASURED_CODE_CFLAGS) -c $< -o $@

tsc_x86.o: tsc_x86.c tsc_x86.h
tscbench.o: tscbench.c 
mathstat.o: mathstat.c mathstat.h
measured_code.o: measured_code.c measured_code.h
tests.o: tests.c

clean:
	@rm -rf *.o $(tscbench) $(tests)
