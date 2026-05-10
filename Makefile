LANG = g++
FLAGS = -std=c++17 -Wall -Wextra -Werror -g
SRC = src/main.cpp src/lexer.cpp src/parser.cpp src/evaluator.cpp
TARGET = glyph

$(TARGET): $(SRC)
	$(LANG) $(FLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: clean
