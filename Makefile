CC ?= gcc
CFLAGS = -Wall -Wextra -O2 -I firmware/include
LDFLAGS = -lm

SRC = firmware/src/motion_gating_engine.c \
      firmware/src/neuromodulation_engine.c \
      firmware/src/erm_hardware_scheduler.c \
      firmware/src/am_crn_system.c \
      firmware/src/serial_protocol.c

OBJ = $(SRC:.c=.o)
LIB = firmware/libam_crn.a
TEST_BIN = firmware/tests/test_am_crn

.PHONY: all build test clean

all: build test

build: $(LIB)

$(LIB): $(OBJ)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(OBJ) firmware/tests/test_am_crn.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f firmware/src/*.o firmware/tests/*.o $(LIB) $(TEST_BIN)
