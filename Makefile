#############################################################################################
# Makefile
#############################################################################################
# G++ is part of GCC (GNU compiler collection) and is a compiler best suited for C++
CXX		:=g++

# Compiler Flags: https://linux.die.net/man/1/g++
#############################################################################################
# -g: produces debugging information (for gdb)
# -Wall: enables all the warnings
# -Wextra: further warnings
# -Werror: treat warnings as errors
# -O: Optimizer turned on
# -std: use the C++ 20 standard
# -c: says not to run the linker
# -pthread: Add support for multithreading using the POSIX threads library. This option sets
#           flags for both the preprocessor and linker. It does not affect the thread safety
#           of object code produced by the compiler or that of libraries supplied with it.
#           These are HP-UX specific flags.
#############################################################################################
CPPFLAGS := -Iinclude -Isrc -Iinclude/server -Iinclude/helpers -Iinclude/client
CXXFLAGS	:= -g -Wall -Wextra -Werror -O -std=c++20 -MMD -MP
LDFLAGS		:= -pthread



# Folders
SRC_DIR		:= src
OBJ_DIR		:= obj
BIN_DIR		:= bin

# searches all files within the folder, the wildcard only says that there is something to input
# use with $(call rwildcard,_directory_,_wildcardForFileEnding_)
rwildcard = $(foreach d,$(wildcard $(1)/*), $(call rwildcard,$(d),$(2))) $(wildcard $(1)/$(2))

COMMON_SRCS	:= $(call rwildcard,$(SRC_DIR)/common,*.cpp)
# Source Files
HELPER_SRCS := $(call rwildcard,$(SRC_DIR)/helpers,*.cpp)
# Helpers
SERVER_SRCS := $(call rwildcard,$(SRC_DIR)/server,*.cpp) $(HELPER_SRCS) $(COMMON_SRCS)
#myserver.cpp
CLIENT_SRCS	:= $(call rwildcard,$(SRC_DIR)/client,*.cpp) $(HELPER_SRCS) $(COMMON_SRCS)
#myclient.cpp

SERVER_OBJS	:= $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SERVER_SRCS))
#$(SERVER_SRCS:%.cpp=$(OBJ_DIR)/%.o)
CLIENT_OBJS	:= $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CLIENT_SRCS))
#$(CLIENT_SRCS:%.cpp=$(OBJ_DIR)/%.o)

SERVER_DEPS	:= $(SERVER_OBJS:.o=.d)
CLIENT_DEPS := $(CLIENT_OBJS:.o=.d)

.PHONY: all clean rebuild

all: ./$(BIN_DIR)/server ./$(BIN_DIR)/client

server: ./$(BIN_DIR)/server
client: ./$(BIN_DIR)/client

rebuild:
	$(MAKE) clean
	$(MAKE) all

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Object building
# Using automatic variables from make
# $< = sources from the patternmatching, e.g. myserver.cpp - all after the :
# $@ = target from the patternmatching, e.g. obj/myserver.o - all until the :
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# link
$(BIN_DIR)/server: $(SERVER_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(SERVER_OBJS) $(CXXFLAGS) $(LDFLAGS) -o $@

$(BIN_DIR)/client: $(CLIENT_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CLIENT_OBJS) $(CXXFLAGS) $(LDFLAGS) -o $@

-include $(SERVER_DEPS) $(CLIENT_DEPS)

