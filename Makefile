FLAGS = \
	-ferror-limit=1 \
	-fshort-enums \
	-Iraylib/include \
	-lGL \
	-lm \
	-lraylib \
	-Lraylib/lib \
	-march=native \
	-O3 \
	-std=c23 \
	-Werror \
	-Weverything \
	-Wno-declaration-after-statement \
	-Wno-gnu-folding-constant \
	-Wno-padded \
	-Wno-pre-c23-compat \
	-Wno-unsafe-buffer-usage
DEBUG_FLAGS = \
	-fsanitize=address \
	-fsanitize=bounds \
	-fsanitize=float-divide-by-zero \
	-fsanitize=implicit-conversion \
	-fsanitize=integer \
	-fsanitize=nullability \
	-fsanitize=undefined \
	-g

.PHONY: all debug release clean

all: bin/debug bin/release

debug: bin/debug
	./bin/debug

release: bin/release
	./bin/release

clean:
	rm -f .ready bin/debug bin/release
	rmdir bin/

raylib/:
	git clone --depth 1 https://github.com/raysan5/raylib.git raylib

raylib/lib/libraylib.a: raylib/
	./scripts/install.sh

bin/:
	mkdir -p bin/

.ready: src/main.c
	clang-format -i src/main.c
	touch .ready

bin/debug: bin/ raylib/lib/libraylib.a .ready
	mold -run clang $(FLAGS) $(DEBUG_FLAGS) src/main.c -o bin/debug

bin/release: bin/ raylib/lib/libraylib.a .ready
	mold -run clang $(FLAGS) src/main.c -o bin/release
