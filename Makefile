LANG = g++
FLAGS = -std=c++17 -Wall -Wextra -Werror -g
SRC = src/main.cpp
TARGET = glyph

$(TARGET): $(SRC)
	$(LANG) $(FLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean
