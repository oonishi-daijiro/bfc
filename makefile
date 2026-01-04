install:
	cmake -G "Ninja" -B build
	ninja -C build
	cmake --install build --config Release --prefix "./bfc/"
clean:
	rm ./build/* -rf
	rm ./build/.* -rf
	rmdir build
	rm ./bfc/* -rf
	rmdir bfc
