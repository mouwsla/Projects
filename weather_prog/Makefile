CMAKE ?= cmake
CTEST ?= ctest

BUILD_DIR ?= build/make
BUILD_TYPE ?= Release
SCENARIO ?= examples/demo_scenario.json

.PHONY: all configure build aviwx app run run-app test clean distclean help

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	$(CMAKE) --build $(BUILD_DIR)

aviwx: configure
	$(CMAKE) --build $(BUILD_DIR) --target aviwx

app: configure
	$(CMAKE) --build $(BUILD_DIR) --target aviwx_app

run: aviwx
	$(BUILD_DIR)/aviwx $(SCENARIO)

run-app: app
	$(BUILD_DIR)/aviwx_app $(SCENARIO)

test: configure
	$(CMAKE) --build $(BUILD_DIR) --target aviwx_tests
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean

distclean:
	$(CMAKE) -E rm -rf $(BUILD_DIR)

help:
	@printf '%s\n' \
		'Targets:' \
		'  make                 Configure and build all available targets' \
		'  make aviwx           Build the CLI risk report tool' \
		'  make app             Build the Qt GUI application (requires Qt6)' \
		'  make run             Run CLI with SCENARIO=examples/demo_scenario.json' \
		'  make run-app         Run GUI with SCENARIO=examples/demo_scenario.json' \
		'  make test            Build and run tests' \
		'  make clean           Clean CMake build artifacts' \
		'  make distclean       Remove the configured CMake build directory' \
		'' \
		'Variables:' \
		'  BUILD_DIR=build/make CMake build directory' \
		'  BUILD_TYPE=Release   CMake build type' \
		'  SCENARIO=path.json   Scenario file for run targets'
