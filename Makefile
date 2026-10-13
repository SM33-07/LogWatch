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
TEST_ARRAY = $(BIN_DIR)/test_array_store$(EXE)
TEST_SORT = $(BIN_DIR)/test_sort$(EXE)
TEST_SEARCH = $(BIN_DIR)/test_search$(EXE)
TEST_CLI = $(BIN_DIR)/test_cli$(EXE)
TEST_EXEC = $(BIN_DIR)/test_cli_executable$(EXE)

WASM_DIR = web/public/wasm
WASM_TARGET = $(WASM_DIR)/logwatch_wasm.js
EMCC ?= emcc

.PHONY: all test clean directories wasm test-wasm

all: directories $(TARGET)

directories:
	@$(MKDIR)

$(TARGET): $(wildcard $(SRC_DIR)/*.c) | directories
	$(CC) $(CFLAGS) $(SRC_DIR)/main.c $(SRC_DIR)/cli.c $(SRC_DIR)/sort.c $(SRC_DIR)/search.c $(SRC_DIR)/log_array.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c -o $(TARGET)

test: directories $(TARGET) $(TEST_DUMMY) $(TEST_PARSER) $(TEST_ARRAY) $(TEST_SORT) $(TEST_SEARCH) $(TEST_CLI) $(TEST_EXEC)
	@echo [RUN] Executing test suites...
	@$(TEST_DUMMY)
	@$(TEST_PARSER)
	@$(TEST_ARRAY)
	@$(TEST_SORT)
	@$(TEST_SEARCH)
	@$(TEST_CLI)
	@$(TEST_EXEC)

$(TEST_DUMMY): $(TEST_DIR)/test_dummy.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_dummy.c -o $(TEST_DUMMY)

$(TEST_PARSER): $(TEST_DIR)/test_parser.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_parser.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c -o $(TEST_PARSER)

$(TEST_ARRAY): $(TEST_DIR)/test_array_store.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/log_array.c $(SRC_DIR)/parser.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_array_store.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/log_array.c $(SRC_DIR)/parser.c -o $(TEST_ARRAY)

$(TEST_SORT): $(TEST_DIR)/test_sort.c $(SRC_DIR)/sort.c $(SRC_DIR)/log_array.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_sort.c $(SRC_DIR)/sort.c $(SRC_DIR)/log_array.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c -o $(TEST_SORT)

$(TEST_SEARCH): $(TEST_DIR)/test_search.c $(SRC_DIR)/search.c $(SRC_DIR)/log_array.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_search.c $(SRC_DIR)/search.c $(SRC_DIR)/log_array.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c -o $(TEST_SEARCH)

$(TEST_CLI): $(TEST_DIR)/test_cli.c $(SRC_DIR)/cli.c $(SRC_DIR)/sort.c $(SRC_DIR)/search.c $(SRC_DIR)/log_array.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_cli.c $(SRC_DIR)/cli.c $(SRC_DIR)/sort.c $(SRC_DIR)/search.c $(SRC_DIR)/log_array.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c -o $(TEST_CLI)

$(TEST_EXEC): $(TEST_DIR)/test_cli_executable.c $(TEST_DIR)/test_utils.h | directories
	$(CC) $(CFLAGS) $(TEST_DIR)/test_cli_executable.c -o $(TEST_EXEC)

wasm:
	@if not exist web\public\wasm mkdir web\public\wasm
	@if not exist web\lib\wasm mkdir web\lib\wasm
	$(EMCC) -std=c11 -Wall -Wextra -pedantic -Iinclude -O2 \
		$(SRC_DIR)/wasm_bridge.c $(SRC_DIR)/log_array.c $(SRC_DIR)/log_entry.c $(SRC_DIR)/parser.c $(SRC_DIR)/sort.c $(SRC_DIR)/search.c \
		-s MODULARIZE=1 -s EXPORT_NAME="createLogWatchModule" \
		-s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 -s NO_EXIT_RUNTIME=1 \
		-s "EXPORTED_FUNCTIONS=['_bridge_init','_bridge_load_from_vfs','_bridge_load_file','_bridge_clear','_bridge_free','_bridge_get_size','_bridge_get_capacity','_bridge_get_stats_json','_bridge_get_records_json','_bridge_sort','_bridge_compare_all_sorts','_bridge_search_response_time','_bridge_search_threshold','_bridge_search_status','_bridge_search_route','_bridge_free_string','_malloc','_free']" \
		-s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','FS','lengthBytesUTF8']" \
		-o $(WASM_TARGET)
	@copy /y "web\public\wasm\*" "web\lib\wasm\" >nul
	@echo [INFO] WebAssembly bridge compiled: $(WASM_TARGET)

test-wasm: wasm
	@echo [RUN] Executing WebAssembly integration test suite...
	node $(TEST_DIR)/test_wasm_bridge.js

clean:
	@$(RM)
	@echo [INFO] Cleaned build artifacts.
