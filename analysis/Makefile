# Host-compatible build for IKOS analysis
.PHONY: all build clean

all: build

build:
	@echo "Building host-compatible version for IKOS analysis..."
	@mkdir -p build
	@cd build && cp ../CMakeLists_host.txt CMakeLists.txt
	@cd build && cmake . && make -j4

clean:
	@echo "Cleaning analysis build..."
	@rm -rf build