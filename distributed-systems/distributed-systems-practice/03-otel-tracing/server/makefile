# target_server — a minimal standalone HTTP target for the Papy tracing
# demo. Reuses Papy's own vendored cpp-httplib and nlohmann::json headers
# via the include path below (no duplication, no SSL/gzip needed here),
# plus the same OpenTelemetry C++ SDK Papy links against.

PAPY_SRC = ../papy-src/Papy-main/src
OTEL_DIR = ../third_party/otel-cpp
# Reuses Papy's own vendored zlib (headers at dependencies/gzip/, the
# actual .so under dependencies/gzip/include/ — see ANALYSIS.md for why).
ZLIB_DIR = $(PAPY_SRC)/dependencies/gzip

CXX = g++
CXXFLAGS = -std=c++23 -I$(PAPY_SRC) -I$(OTEL_DIR)/include -I$(ZLIB_DIR) \
    -DOPENTELEMETRY_ABI_VERSION_NO=1 -DOPENTELEMETRY_STL_VERSION=2023 \
    -MMD -MP -O2

OTEL_LIBS = -Wl,--start-group \
    $(OTEL_DIR)/lib/libopentelemetry_trace.a \
    $(OTEL_DIR)/lib/libopentelemetry_exporter_ostream_span.a \
    $(OTEL_DIR)/lib/libopentelemetry_resources.a \
    $(OTEL_DIR)/lib/libopentelemetry_instrumentation_scope.a \
    $(OTEL_DIR)/lib/libopentelemetry_common.a \
    $(OTEL_DIR)/lib/libopentelemetry_version.a \
    -Wl,--end-group -lpthread

ZLIB_LIBS = -L$(ZLIB_DIR)/include -lz -Wl,-rpath,$(abspath $(ZLIB_DIR)/include)

TARGET = bin/target_server
OBJ = obj/target_server.o obj/otelTracing.o

all: directories $(TARGET)

directories:
	@mkdir -p obj bin

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(OTEL_LIBS) $(ZLIB_LIBS)

obj/%.o: %.cpp | directories
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include obj/target_server.d obj/otelTracing.d

clean:
	rm -rf obj bin

.PHONY: all clean directories
