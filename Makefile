# LogWatch - GNU Makefile
CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Iinclude -Itests
BIN_DIR = bin
SRC_DIR = src
TEST_DIR = tests

# Detect OS for binary naming and directory operations
ifeq ($(OS),Windows_NT)
    EXE = .exe
    MKDIR = if not exist $(BIN_DIR) mkdir $(BIN_DIR)
    RM = if exist $(BIN_DIR) rmdir /s /q $(BIN_DIR)
else
    EXE =
    MKDIR = mkdir -p $(BIN_DIR)
    RM = rm -rf $(BIN_DIR)
endif

TARGET = $(BIN_DIR)/logwatch$(EXE)
TEST_DUMMY = $(BIN_DIR)/test_dummy$(EXE)
TEST_PARSER = $(BIN_DIR)/test_parser$(EXE)

.PHONY: all test clean directories

all: directories $(TARGET)

directories:
	@$(MKDIR)

$(TARGET): $(wildcard $(SRC_DIR)/*.c) | directories
	@if exist $(SRC_DIR)\main.c ( $(CC) $(CFLAGS) $(SRC_DIR)/*.c -o $(TARGET) ) else ( echo [INFO] Core CLI application deferred to Milestone 6. )

test: directories $(TEST_DUMMY) $(TEST_PARSER)
	@echo [RUN] Executing test suites...
	@$(TEST_DUMMY)
	@$(TEST_PARSER)

$(TEST_DUMMY): $(TEST_DIR)/test_dummy.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_dummy.c -o $(TEST_DUMMY)

$(TEST_PARSER): $(TEST_DIR)/test_parser.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_parser.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c -o $(TEST_PARSER)

clean:
	@$(RM)
	@echo [INFO] Cleaned build artifacts.
