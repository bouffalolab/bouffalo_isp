CMAKE = cmake # use user cmake
cmake_generator = "Unix Makefiles"

build:Makefile
	$(CMAKE) -S . -B build -G $(cmake_generator)
	make -C build -j8

clean::
	rm -rf build

.PHONY:build clean