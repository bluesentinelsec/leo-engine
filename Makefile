.PHONY: build run test clean fmt

build:
	cmake -B build -S .
	cmake --build build

run: build
	./build/leo-engine

test: build
	cd build && ctest --output-on-failure

clean:
	rm -rf build

fmt:
	find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i
