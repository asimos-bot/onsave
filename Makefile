.PHONY: build install

TARGET=onsave
CC=clang
build:
	 $(CC) $(TARGET).c -o $(TARGET)
run: build
	./$(TARGET) README.md echo hello
install: build
	mkdir -p ~/.local/bin/
	cp $(TARGET) ~/.local/bin/
