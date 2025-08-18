dev:
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=On
	cmake --build build --parallel

test: dev
	./build/test_leo_runtime

release:
	cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DLEO_BUILD_SHARED=ON -DLEO_BUILD_TESTS=On
	cmake --build build --parallel
