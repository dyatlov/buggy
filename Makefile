CXX=clang++
CXXLAGS=-c -g -std=c++17 -Wall

MKDIR_P=mkdir -p

SRC=hhwheel_timer.cc main.cc

SRC_DIR=src/
BUILD_DIR=build/

OBJECTS=$(addprefix $(BUILD_DIR), $(SRC:.cc=.o))

OUTPUT=$(BUILD_DIR)loop

.PHONY: all clean
all: $(addprefix $(SRC_DIR), $(SRC)) $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

build/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CXXLAGS) $< -o $@

clean:
	rm -rf build