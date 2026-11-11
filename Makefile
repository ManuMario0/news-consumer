CXX      := c++
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -O2
LDFLAGS  :=

SRC_DIR := src
BUILD   := build

SRCS        := $(filter-out $(SRC_DIR)/main_client.cpp,$(wildcard $(SRC_DIR)/*.cpp))
OBJS        := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD)/%.o,$(SRCS))
CLIENT_OBJS := $(BUILD)/main_client.o $(BUILD)/framing.o $(BUILD)/transport.o

TARGET        := $(BUILD)/server
CLIENT_TARGET := $(BUILD)/client

all: $(TARGET) $(CLIENT_TARGET)

client: $(CLIENT_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(BUILD)/%.o: $(SRC_DIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: all client clean
