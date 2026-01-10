dev:
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=On
	cmake --build build --parallel

test: dev
	./build/test_leo_runtime

release:
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=On
	cmake --build build --parallel

fmt:
	find include/leo -name "*.h" -exec clang-format-14 -i {} +
	find src -name "*.c" -exec clang-format-14 -i {} +
	find tests -name "*.cpp" -exec clang-format-14 -i {} +

web:
	docker build . -t dev:latest
	docker run -it -p 8000:8000 dev:latest

web-podman:
	podman build . -t dev:latest
	podman run -it -p 8000:8000 dev:latest

clean:
	rm -rf build
