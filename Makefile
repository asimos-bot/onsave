.PHONY: build install

TARGET=onsave
CC=clang
build:
	 $(CC) $$(cat config | awk '{ print "-D" $$1 }' | tr '\n' ' ') `pkg-config --cflags libnotify` $(TARGET).c -o $(TARGET) `pkg-config --libs libnotify`
run: build
	./$(TARGET)
install: build
	mkdir -p ~/.local/bin/
	cp $(TARGET) ~/.local/bin/
