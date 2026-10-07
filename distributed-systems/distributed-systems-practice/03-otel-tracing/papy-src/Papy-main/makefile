# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
ifeq ($(shell uname), Darwin)
    OPENSSL_DIR = /opt/homebrew/opt/openssl@3
    ZLIB_DIR = /opt/homebrew/opt/zlib
    OPENSSL_LIBDIR = $(OPENSSL_DIR)/lib
    ZLIB_LIBDIR = $(ZLIB_DIR)/lib
else
    OPENSSL_DIR = $(SRC_DIR)/dependencies/openssl
    ZLIB_DIR = $(SRC_DIR)/dependencies/gzip
    # NOTE: the vendored .so files ship inside include/, not lib/ — this
    # repo was previously only ever built on macOS (Homebrew branch
    # above), so this Linux path was never exercised. See ANALYSIS.md,
    # "A pre-existing Papy build defect found along the way".
    OPENSSL_LIBDIR = $(OPENSSL_DIR)/include
    ZLIB_LIBDIR = $(ZLIB_DIR)/include
endif

# OpenTelemetry C++ SDK (built by ../../scripts/setup_otel.sh into
# ../../third_party/otel-cpp — a flat include/ + lib/ pair regardless of
# the upstream repo's own directory layout). See ANALYSIS.md for what's
# real (the actual SDK, real spans, real W3C trace-context propagation)
# vs. simplified (OStreamSpanExporter instead of a network collector).
OTEL_DIR = ../../third_party/otel-cpp
OTEL_INCLUDES = -I$(OTEL_DIR)/include
# Must match exactly what third_party/otel-cpp was compiled with, or
# nostd::shared_ptr's ABI layout mismatches between Papy's object code
# and the SDK's, causing silent segfaults (see ANALYSIS.md).
OTEL_DEFINES = -DOPENTELEMETRY_ABI_VERSION_NO=1 -DOPENTELEMETRY_STL_VERSION=2023
OTEL_LIBS = -Wl,--start-group \
    $(OTEL_DIR)/lib/libopentelemetry_trace.a \
    $(OTEL_DIR)/lib/libopentelemetry_exporter_ostream_span.a \
    $(OTEL_DIR)/lib/libopentelemetry_resources.a \
    $(OTEL_DIR)/lib/libopentelemetry_instrumentation_scope.a \
    $(OTEL_DIR)/lib/libopentelemetry_common.a \
    $(OTEL_DIR)/lib/libopentelemetry_version.a \
    -Wl,--end-group -lpthread

# Compiler and flags
CXX = g++
#CXXFLAGS_COMMON = -I$(OPENSSL_DIR) -I$(ZLIB_DIR) -MMD -MP -O3   # Include dependency tracking
CXXFLAGS_COMMON = -I$(OPENSSL_DIR)/include -I$(ZLIB_DIR)/include $(OTEL_INCLUDES) $(OTEL_DEFINES) -MMD -MP -O3

ifeq ($(shell uname), Darwin)
	CXXFLAGS = -std=c++2b $(CXXFLAGS_COMMON)  # macOS-specific flags
else
	CXXFLAGS = -std=c++23 $(CXXFLAGS_COMMON)  # Default flags
endif

# Linker flags
#LDFLAGS = -L$(OPENSSL_DIR) -L$(ZLIB_DIR) -lssl -lcrypto -lz # Link local OpenSSL libraries
LDFLAGS = -L$(OPENSSL_LIBDIR) -L$(ZLIB_LIBDIR) -lssl -lcrypto -lz \
    -Wl,-rpath,$(abspath $(OPENSSL_LIBDIR)) -Wl,-rpath,$(abspath $(ZLIB_LIBDIR)) \
    $(OTEL_LIBS)


# Source files and object files
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# Track build dependencies
DEP_FILES = $(SRC_FILES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.d)

# Output executable
TARGET = $(BIN_DIR)/papy

# Default target to build
all: directories $(TARGET)

# Create required directories if they don't exist
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Link object files to create the executable
$(TARGET): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $@ $(LDFLAGS)

# Rule to compile .cpp to .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEP_FILES)

# Clean up object files and executable
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

# Rebuild the project
rebuild: clean all

# Install (optional, you can customize)
install:
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean rebuild install directories
