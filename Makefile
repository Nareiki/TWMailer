#############################################################################################
# Makefile
#############################################################################################
# G++ is part of GCC (GNU compiler collection) and is a compiler best suited for C++
CXX		:=g++



# Folders
SRC_DIR		:= src
OBJ_DIR		:= obj
BIN_DIR		:= bin

# searches all files within the folder, the wildcard only says that there is something to input
# use with $(call rwildcard,_directory_,_wildcardForFileEnding_)
rwildcard = $(foreach d,$(wildcard $(1)/*), $(call rwildcard,$(d),$(2))) $(wildcard $(1)/$(2))

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

CXXFLAGS	:= -g -Wall -Wextra -Werror -O -std=c++20 -MMD -MP
LDFLAGS		:= -pthread -lldap -llber

HEADER_DIRS := $(sort $(dir $(call rwildcard,$(SRC_DIR),*.h)) $(dir $(call rwildcard,$(SRC_DIR),*.hpp)))
INCLUDE_FLAGS := $(addprefix -I,$(HEADER_DIRS))

CPPFLAGS := -Isrc -Iinclude $(INCLUDE_FLAGS)#-Iinclude -Isrc -Iinclude/server -Iinclude/helpers -Iinclude/client -Iinclude/ldap #-Iinclude/server/ldap
#-Isrc $(INCLUDE_FLAGS)#

# Source Files
HELPER_SRCS := $(call rwildcard,$(SRC_DIR)/helpers,*.cpp)
# Helpers
SERVER_SRCS := $(call rwildcard,$(SRC_DIR)/server,*.cpp) $(HELPER_SRCS)
#myserver.cpp
CLIENT_SRCS	:= $(call rwildcard,$(SRC_DIR)/client,*.cpp) $(HELPER_SRCS)
#myclient.cpp

SERVER_OBJS	:= $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SERVER_SRCS))
#$(SERVER_SRCS:%.cpp=$(OBJ_DIR)/%.o)
CLIENT_OBJS	:= $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(CLIENT_SRCS))
#$(CLIENT_SRCS:%.cpp=$(OBJ_DIR)/%.o)

LDAP_SRCS	:= $(call rwildcard,$(SRC_DIR)/ldap,*.cpp) $(HELPER_SRCS)

LDAP_OBJS	:= $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(LDAP_SRCS))

SERVER_DEPS	:= $(SERVER_OBJS:.o=.d)
CLIENT_DEPS := $(CLIENT_OBJS:.o=.d)

.PHONY: all clean rebuild

all: ./$(BIN_DIR)/server ./$(BIN_DIR)/client

server: ./$(BIN_DIR)/server
client: ./$(BIN_DIR)/client
ldap: ./$(BIN_DIR)/ldap

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

$(BIN_DIR)/ldap: $(LDAP_SRCS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(LDAP_OBJS) $(CXXFLAGS) $(LDFLAGS) -o $@

-include $(SERVER_DEPS) $(CLIENT_DEPS)

