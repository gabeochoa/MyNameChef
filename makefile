# Detect OS
UNAME_S := $(shell uname -s)

# Compiler settings
ifeq ($(UNAME_S),Darwin)
    CXX := clang++
    EXT := .exe
    RAYLIB_FLAGS := $(shell pkg-config --cflags raylib)
    RAYLIB_LIB := $(shell pkg-config --libs raylib)
    MACOS_FLAGS := -DBACKWARD
    FRAMEWORKS := -framework CoreFoundation
else ifeq ($(OS),Windows_NT)
    CXX := g++
    EXT := .exe
    RAYLIB_FLAGS := -IF:/RayLib/include
    RAYLIB_LIB := F:/RayLib/lib/raylib.dll
    MACOS_FLAGS :=
    FRAMEWORKS :=
else
    CXX := clang++
    EXT :=
    RAYLIB_FLAGS := $(shell pkg-config --cflags raylib)
    RAYLIB_LIB := $(shell pkg-config --libs raylib)
    MACOS_FLAGS :=
    FRAMEWORKS :=
endif

# C++ standard
CXXSTD := -std=c++23

# Base compiler flags from xmake.lua
CXXFLAGS_BASE := -g \
    -fmax-errors=10 \
    -Wall -Wextra -Wpedantic \
    -Wuninitialized -Wshadow -Wconversion \
    -Wcast-qual -Wchar-subscripts \
    -Wcomment -Wdisabled-optimization -Wformat=2 \
    -Wformat-nonliteral -Wformat-security -Wformat-y2k \
    -Wimport -Winit-self -Winline -Winvalid-pch \
    -Wlong-long -Wmissing-format-attribute \
    -Wmissing-include-dirs \
    -Wpacked -Wpointer-arith \
    -Wreturn-type -Wsequence-point \
    -Wstrict-overflow=5 -Wswitch -Wswitch-default \
    -Wswitch-enum -Wtrigraphs \
    -Wunused-label -Wunused-parameter -Wunused-value \
    -Wunused-variable -Wvariadic-macros -Wvolatile-register-var \
    -Wwrite-strings -Warray-bounds \
    -pipe \
    -fno-stack-protector \
    -fno-common

# Warning suppressions from xmake.lua
CXXFLAGS_SUPPRESS := -Wno-deprecated-volatile -Wno-missing-field-initializers \
    -Wno-c99-extensions -Wno-unused-function -Wno-sign-conversion \
    -Wno-implicit-int-float-conversion -Wno-implicit-float-conversion \
    -Wno-format-nonliteral -Wno-format-security -Wno-format-y2k \
    -Wno-import -Wno-inline -Wno-invalid-pch \
    -Wno-long-long -Wno-missing-format-attribute \
    -Wno-missing-noreturn -Wno-packed -Wno-redundant-decls \
    -Wno-sequence-point -Wno-trigraphs -Wno-variadic-macros \
    -Wno-volatile-register-var

# Time tracing for ClangBuildAnalyzer
CXXFLAGS_TIME_TRACE := -ftime-trace

# Coverage flags
COVERAGE_CXXFLAGS :=
COVERAGE_LDFLAGS :=
ifeq ($(COVERAGE),1)
    ifeq ($(UNAME_S),Darwin)
        COVERAGE_CXXFLAGS := -fprofile-instr-generate -fcoverage-mapping
        COVERAGE_LDFLAGS := -fprofile-instr-generate
    else
        COVERAGE_CXXFLAGS := --coverage
        COVERAGE_LDFLAGS := --coverage
    endif
endif

# Combine all CXXFLAGS
CXXFLAGS := $(CXXSTD) $(CXXFLAGS_BASE) $(CXXFLAGS_SUPPRESS) $(CXXFLAGS_TIME_TRACE) \
    $(MACOS_FLAGS) $(COVERAGE_CXXFLAGS) $(RAYLIB_FLAGS)

# Include directories
INCLUDES := -Ivendor/

# Library flags
LDFLAGS := -L. -Lvendor/ $(RAYLIB_LIB) $(FRAMEWORKS) $(COVERAGE_LDFLAGS)

# Directories
OBJ_DIR := output/objs
OUTPUT_DIR := output

# Source files for my_name_chef
MAIN_SRC := $(wildcard src/*.cpp)
MAIN_SRC := $(filter-out src/main.cpp, $(MAIN_SRC))
MAIN_SRC += src/main.cpp
MAIN_SRC += $(wildcard src/components/*.cpp)
MAIN_SRC += $(wildcard src/systems/*.cpp)
MAIN_SRC += $(wildcard src/ui/*.cpp)
MAIN_SRC += $(wildcard src/utils/*.cpp)
MAIN_SRC += src/server/file_storage.cpp
MAIN_SRC += src/testing/test_app.cpp
MAIN_SRC += src/testing/test_context.cpp
MAIN_SRC += src/testing/test_input.cpp

# Source files for battle_server
SERVER_SRC := $(wildcard src/server/*.cpp)
SERVER_SRC += $(wildcard src/server/async/*.cpp)
SERVER_SRC += $(wildcard src/server/tests/*.cpp)
SERVER_SRC += $(filter-out src/main.cpp, $(wildcard src/*.cpp))
SERVER_SRC += $(wildcard src/components/*.cpp)
SERVER_SRC += $(filter-out src/systems/TestSystem.cpp, $(wildcard src/systems/*.cpp))
SERVER_SRC += $(wildcard src/ui/*.cpp)
SERVER_SRC += $(wildcard src/utils/*.cpp)

# Object files
MAIN_OBJS := $(MAIN_SRC:src/%.cpp=$(OBJ_DIR)/main/%.o)
SERVER_OBJS := $(SERVER_SRC:src/%.cpp=$(OBJ_DIR)/server/%.o)

# Dependency files
MAIN_DEPS := $(MAIN_OBJS:.o=.d)
SERVER_DEPS := $(SERVER_OBJS:.o=.d)

# Output executables
MAIN_EXE := $(OUTPUT_DIR)/my_name_chef$(EXT)
SERVER_EXE := $(OUTPUT_DIR)/battle_server$(EXT)

# Code hash generation
CODE_HASH_GENERATED := src/utils/code_hash_generated.h

$(CODE_HASH_GENERATED): scripts/generate_code_hash.sh
	@echo "Generating code hash..."
	@./scripts/generate_code_hash.sh

# Default target
all: $(CODE_HASH_GENERATED) $(MAIN_EXE)

# Build both targets
both: $(CODE_HASH_GENERATED) $(MAIN_EXE) $(SERVER_EXE)

# Main executable
$(MAIN_EXE): $(CODE_HASH_GENERATED) $(MAIN_OBJS) | $(OUTPUT_DIR)/.stamp
	@echo "Linking $(MAIN_EXE)..."
	$(CXX) $(CXXFLAGS) $(MAIN_OBJS) $(LDFLAGS) -o $@
	@echo "Built $(MAIN_EXE)"

# Battle server executable
$(SERVER_EXE): CXXFLAGS += -DHEADLESS_MODE
$(SERVER_EXE): $(CODE_HASH_GENERATED) $(SERVER_OBJS) | $(OUTPUT_DIR)/.stamp
	@echo "Linking $(SERVER_EXE)..."
	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) $(LDFLAGS) -o $@
	@echo "Built $(SERVER_EXE)"

# Compile main object files
$(OBJ_DIR)/main/%.o: src/%.cpp $(CODE_HASH_GENERATED) | $(OBJ_DIR)/main
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@ -MMD -MP -MF $(@:.o=.d) -MT $@

# Compile server object files
$(OBJ_DIR)/server/%.o: src/%.cpp $(CODE_HASH_GENERATED) | $(OBJ_DIR)/server
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DHEADLESS_MODE $(INCLUDES) -c $< -o $@ -MMD -MP -MF $(@:.o=.d) -MT $@

# Create directories
$(OUTPUT_DIR)/.stamp:
	@mkdir -p $(OUTPUT_DIR)
	@touch $@

$(OBJ_DIR)/main:
	@mkdir -p $(OBJ_DIR)/main

$(OBJ_DIR)/server:
	@mkdir -p $(OBJ_DIR)/server

# Include dependency files
-include $(MAIN_DEPS)
-include $(SERVER_DEPS)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR)
	@echo "Clean complete"

clean-all: clean
	rm -f $(MAIN_EXE) $(SERVER_EXE)
	@echo "Cleaned all"

# Resource copying
ifeq ($(UNAME_S),Darwin)
    mkdir_cmd := mkdir -p $(OUTPUT_DIR)/resources/
    cp_resources_cmd := cp -r resources/* $(OUTPUT_DIR)/resources/
    sign_cmd := codesign -s - -f --verbose --entitlements ent.plist
else ifeq ($(OS),Windows_NT)
    mkdir_cmd := powershell -command "& {&'New-Item' -Path .\ -Name $(OUTPUT_DIR)\resources -ItemType directory -ErrorAction SilentlyContinue}"
    cp_resources_cmd := powershell -command "& {&'Copy-Item' .\resources\* $(OUTPUT_DIR)\resources -ErrorAction SilentlyContinue}"
    sign_cmd :=
else
    mkdir_cmd := mkdir -p $(OUTPUT_DIR)/resources/
    cp_resources_cmd := cp -r resources/* $(OUTPUT_DIR)/resources/
    sign_cmd :=
endif

output: $(MAIN_EXE)
	$(mkdir_cmd)
	$(cp_resources_cmd)

sign: $(MAIN_EXE)
	$(sign_cmd) $(MAIN_EXE)

run: output
	./$(MAIN_EXE)

# Utility targets
.PHONY: all both clean clean-all output sign run

# ClangBuildAnalyzer integration
cba: clean
	@echo "Building with make to generate trace data..."
	$(MAKE) $(MAIN_EXE)
	@echo "Analyzing build performance..."
	@mkdir -p $(OBJ_DIR)/main
	ClangBuildAnalyzer --all $(OBJ_DIR)/main/ build-analysis.html
	ClangBuildAnalyzer --analyze build-analysis.html | tee build-analysis.txt
	@echo ""
	@echo "Top 5 slowest files to parse:"
	@head -15 build-analysis.txt | grep -A 10 "Files that took longest to parse" || true

clean-cba:
	rm -f build-analysis.html build-analysis.txt
	@echo "Analysis files cleaned"

# Profiling targets (macOS only)
ifeq ($(UNAME_S),Darwin)
prof: output sign
	rm -rf recording.trace/
	xctrace record --template 'Time Profiler' --output 'recording.trace' --launch $(MAIN_EXE)

leak: output sign
	rm -rf recording.trace/
	xctrace record --template 'Leaks' --output 'recording.trace' --launch $(MAIN_EXE)

alloc: output sign
	rm -rf recording.trace/
	xctrace record --template 'Allocations' --output 'recording.trace' --launch $(MAIN_EXE)
endif

# Code counting
count:
	git ls-files | grep "src" | grep -v "resources" | grep -v "vendor" | xargs wc -l | sort -rn | pr -2 -t -w 100

countall:
	git ls-files | xargs wc -l | sort -rn

# Static analysis
cppcheck:
	cppcheck src/ -Ivendor/afterhours --enable=all --std=c++23 --language=c++ \
		--suppress=noConstructor --suppress=noExplicitConstructor \
		--suppress=useStlAlgorithm --suppress=unusedStructMember \
		--suppress=useInitializationList --suppress=duplicateCondition \
		--suppress=nullPointerRedundantCheck --suppress=cstyleCast

# Dependency graph targets
deps:
	cd tools && make run

deps-dot:
	cd tools && ./dependency_graph --src ../src --main ../src/main.cpp --outdir ../output

deps-svg:
	cd tools && ./dependency_graph --src ../src --main ../src/main.cpp --outdir ../output --svg

deps-html:
	cd tools && ./dependency_graph --src ../src --main ../src/main.cpp --outdir ../output

deps-check: deps
	@echo "Checking dependency graph against baseline..."
	@[ -f tools/dependency_baseline.json ] || (echo "No baseline found at tools/dependency_baseline.json" && exit 2)
	@diff -u tools/dependency_baseline.json output/dependency_summary.json || (echo "Dependency summary changed. Run 'make deps' and update baseline if intentional." && exit 1)

.PHONY: cba clean-cba prof leak alloc count countall cppcheck deps deps-dot deps-svg deps-html deps-check
