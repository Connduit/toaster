# === Compiler and flags ===
CC      := gcc
CXX     := g++
CFLAGS  := -Wall -Wextra
CXXFLAGS:= -std=c++17 -Wall -Wextra
#CXXFLAGS:= -std=c++17 -Wall -Wextra -g
LDFLAGS := -lrtlsdr

OBJ_DIR  := obj
BIN_DIR  := bin

# === Include paths ===
# CFLAGS  += -Iinstaller/include -Iloader/include -Ibuilder/include -Iengine/include -Icommon/include
# CXXFLAGS+= -Iinstaller/include -Iloader/include -Ibuilder/include -Iengine/include -Icommon/include

# === Source discovery ===
# INSTALLER_SOURCES := $(wildcard installer/src/*.c)
# LOADER_SOURCES    := $(wildcard loader/src/*.c) 
# BUILDER_SOURCES   := $(wildcard builder/src/*.cpp)
# ENGINE_SOURCES    := $(wildcard engine/src/*.c)
# COMMON_SOURCES    := $(wildcard common/src/*.c)
DEFAULT_SOURCES    := $(wildcard *.cpp)

# === Object files (flat in obj/ with unique names) ===
# INSTALLER_OBJS := $(patsubst installer/src/%.c,$(OBJ_DIR)/installer_%.o,$(INSTALLER_SOURCES))
# LOADER_OBJS := $(patsubst loader/src/%.c,$(OBJ_DIR)/loader_%.o,$(LOADER_SOURCES))
# BUILDER_OBJS := $(patsubst builder/src/%.c,$(OBJ_DIR)/builder_%.o,$(BUILDER_SOURCES))
# BUILDER_OBJS := $(patsubst builder/src/%.cpp,$(OBJ_DIR)/builder_%.o,$(BUILDER_OBJS))
# ENGINE_OBJS := $(patsubst engine/src/%.c,$(OBJ_DIR)/engine_%.o,$(ENGINE_SOURCES))
# COMMON_OBJS := $(patsubst common/src/%.c,$(OBJ_DIR)/common_%.o,$(COMMON_SOURCES))
# COMMON_OBJS := $(patsubst common/src/%.cpp,$(OBJ_DIR)/common_%.o,$(COMMON_OBJS))

# INSTALLER_OBJS += $(COMMON_OBJS)
# LOADER_OBJS    += $(COMMON_OBJS)
# BUILDER_OBJS   += $(COMMON_OBJS)
# ENGINE_OBJS    += $(COMMON_OBJS)

# === Output files ===
# INSTALLER_DLL := $(BIN_DIR)/installer.dll
# LOADER_DLL    := $(BIN_DIR)/loader.dll
# BUILDER_EXE   := $(BIN_DIR)/builder.exe
# ENGINE_DLL    := $(BIN_DIR)/engine.dll
TOASTER    := $(BIN_DIR)/toaster

# === Default target ===
# all: $(INSTALLER_DLL) $(LOADER_DLL) $(BUILDER_EXE) $(ENGINE_DLL)
all: $(TOASTER)

# === Build targets ===
# $(INSTALLER_DLL): $(INSTALLER_OBJS)
# 	@mkdir -p $(BIN_DIR) $(OBJ_DIR)
# 	$(CXX) $(LDFLAGS) $^ -o $@
# 
# $(LOADER_DLL): $(LOADER_OBJS)
# 	@mkdir -p $(BIN_DIR) $(OBJ_DIR)
# 	$(CXX) $(LDFLAGS) $^ -o $@
# 
# $(BUILDER_EXE): $(BUILDER_OBJS)
# 	@mkdir -p $(BIN_DIR) $(OBJ_DIR)
# 	$(CXX) $^ -o $@
# 
# $(ENGINE_DLL): $(ENGINE_OBJS)
# 	@mkdir -p $(BIN_DIR) $(OBJ_DIR)
# 	$(CXX) $(LDFLAGS) $^ -o $@
# 
# # === Compile rules for installer ===
# $(OBJ_DIR)/installer_%.o: installer/src/%.c
# 	@mkdir -p $(OBJ_DIR)
# 	$(CC) $(CFLAGS) -c $< -o $@
# 
# # === Compile rules for loader ===
# $(OBJ_DIR)/loader_%.o: loader/src/%.c
# 	@mkdir -p $(OBJ_DIR)
# 	$(CC) $(CFLAGS) -c $< -o $@
# 
# # === Compile rules for builder ===
# $(OBJ_DIR)/builder_%.o: builder/src/%.c
# 	@mkdir -p $(OBJ_DIR)
# 	$(CC) $(CFLAGS) -c $< -o $@
# 
# $(OBJ_DIR)/builder_%.o: builder/src/%.cpp
# 	@mkdir -p $(OBJ_DIR)
# 	$(CXX) $(CXXFLAGS) -c $< -o $@
# 
# # === Compile rules for engine ===
# $(OBJ_DIR)/engine_%.o: engine/src/%.c
# 	@mkdir -p $(OBJ_DIR)
# 	$(CC) $(CFLAGS) -c $< -o $@
# 
# # === Compile rules for common ===
# $(OBJ_DIR)/common_%.o: common/src/%.c
# 	@mkdir -p $(OBJ_DIR)
# 	$(CC) $(CFLAGS) -c $< -o $@
# 
# $(OBJ_DIR)/common_%.o: common/src/%.cpp
# 	@mkdir -p $(OBJ_DIR)
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TOASTER): $(DEFAULT_SOURCES)
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

# === Clean target ===
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean
