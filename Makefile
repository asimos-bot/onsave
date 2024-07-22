.PHONY: build install

TARGET=onsave
CC=clang
build:
	 $(CC) $(TARGET).c -o $(TARGET)
run: build
	./$(TARGET) -v -i a -i b test "sleep 0.5; echo hello"
install: build
	mkdir -p ~/.local/bin/
	cp $(TARGET) ~/.local/bin/
