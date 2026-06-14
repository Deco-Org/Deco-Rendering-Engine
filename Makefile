.PHONY: build run clean

build:
	cmake -B build -G Xcode
	cmake --build build --target rendering_engine_harness

run: build
	./build/harness/Debug/rendering_engine_harness.app/Contents/MacOS/rendering_engine_harness

clean:
	rm -rf build/