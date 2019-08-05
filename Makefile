##
#  Author: Marius Meyer
#  eMail:  marius.meyer@upb.de
#  Date:   2019/07/24
#
#  This makefile compiles the Random Access benchmark for FPGA and its OpenCL kernels.
#  Currently only the  Intel(R) FPGA SDK for OpenCL(TM) utitlity is supported.
#
#  Please read the README contained in this folder for more information and
#  instructions how to compile and execute the benchmark.

# Used compilers for C code and OpenCL kernels
CXX := g++ --std=c++11
AOC := aoc
AOCL := aocl
MKDIR_P := mkdir -p

CODE_GENERATOR := pycodegen/generator.py

$(info ***************************)
$(info Collected information)

# Check Quartus version
ifndef QUARTUS_VERSION
$(info QUARTUS_VERSION not defined! Quartus is not set up correctly or version is too old (<17.1). In the latter case:)
$(info Define the variable with the used Quartus version by giving it as an argument to make: e.g. make QUARTUS_VERSION=17.1.0)
$(error QUARTUS_VERSION not defined!)
else
$(info QUARTUS_VERSION     = $(QUARTUS_VERSION))
endif

# OpenCL compile and link flags.
AOCL_COMPILE_CONFIG := $(shell $(AOCL) compile-config )
AOCL_LINK_CONFIG := $(shell $(AOCL) link-config )

BIN_DIR := bin/
SRC_DIR := src/
GEN_SRC_DIR := generated/

ifdef BUILD_SUFFIX
	EXT_BUILD_SUFFIX := _$(BUILD_SUFFIX)
endif

## Build settings
#
# This section contains all build parameters that might be considered to
# change for compilation and synthesis.
#
##
TYPE := simple
REPLICATIONS := 4
UPDATE_SPLIT := 128
GLOBAL_MEM_UNROLL := 8
BOARD := p520_max_sg280l
GLOBAL_MEM_SIZE := 268435456
AOC_FLAGS :=-no-interleaving=default
## End build settings

GEN_SRC := $(SRC_DIR)host/random_$(TYPE).cpp
GEN_KERNEL_SRC := $(SRC_DIR)device/random_access_kernels_$(TYPE).cl

SRCS := random_$(TYPE)_$(UPDATE_SPLIT)_$(REPLICATIONS).cpp
TARGET := $(SRCS:.cpp=)$(EXT_BUILD_SUFFIX)
KERNEL_SRCS := random_access_kernels_$(TYPE)_$(UPDATE_SPLIT)_$(REPLICATIONS).cl
KERNEL_TARGET := $(KERNEL_SRCS:.cl=)$(EXT_BUILD_SUFFIX)

COMMON_FLAGS := -DUPDATE_SPLIT=$(UPDATE_SPLIT) -DREPLICATIONS=$(REPLICATIONS)
AOC_PARAMS := $(AOC_FLAGS) -board=$(BOARD) -DGLOBAL_MEM_UNROLL=$(GLOBAL_MEM_UNROLL)

ifdef DEBUG
CXX_FLAGS += -g
endif

ifdef EMBARRASSINGLY_PARALLEL
COMMON_FLAGS += -DEMBARRASSINGLY_PARALLEL
endif

$(info BOARD                   = $(BOARD))
$(info BUILD_SUFFIX            = $(BUILD_SUFFIX))
$(info AOC_FLAGS               = $(AOC_FLAGS))
$(info REPLICATIONS            = $(REPLICATIONS))
$(info GLOBAL_MEM_SIZE         = $(GLOBAL_MEM_SIZE))
$(info UPDATE_SPLIT            = $(UPDATE_SPLIT))
$(info TYPE                    = $(TYPE))
$(info EMBARRASSINGLY_PARALLEL = $(EMBARRASSINGLY_PARALLEL))
$(info ***************************)

default: info
	$(error No target specified)

info:
	$(info *************************************************)
	$(info Please specify one ore more of the listed targets)
	$(info *************************************************)
	$(info Host Code:)
	$(info host                         = Use memory interleaving to store the arrays on the FPGA)
	$(info *************************************************)
	$(info Kernels:)
	$(info kernel                       = Compile global memory kernel)
	$(info kernel_emulate               = Compile  global memory kernel for emulation)
	$(info kernel_profile               = Compile  global memory kernel with profiling information enabled)
	$(info kernel_profile               = Compile  global memory kernel with profiling information enabled)
	$(info ************************************************)
	$(info info                         = Print this list of available targets)
	$(info ************************************************)
	$(info Additional compile flags for the kernels can be provided in AOC_FLAGS.)
	$(info To disable memory interleaving: make kernel AOC_FLAGS=-no-interleaving=default)

$(GEN_SRC_DIR)$(SRCS): $(GEN_SRC)
	$(MKDIR_P) $(GEN_SRC_DIR)
	$(CODE_GENERATOR) $(GEN_SRC) -o $(GEN_SRC_DIR)$(SRCS) -p replications=$(REPLICATIONS)

$(GEN_SRC_DIR)$(KERNEL_SRCS): $(GEN_KERNEL_SRC)
	$(MKDIR_P) $(GEN_SRC_DIR)
	$(CODE_GENERATOR) $(GEN_KERNEL_SRC) -o $(GEN_SRC_DIR)$(KERNEL_SRCS) -p replications=$(REPLICATIONS)

host: $(GEN_SRC_DIR)$(SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(CXX) $(CXX_FLAGS) $(AOCL_COMPILE_CONFIG) $(COMMON_FLAGS) -DDATA_LENGTH=$(GLOBAL_MEM_SIZE) \
	-DFPGA_KERNEL_FILE_GLOBAL=\"$(KERNEL_TARGET).aocx\" \
	$(GEN_SRC_DIR)$(SRCS) $(AOCL_LINK_CONFIG) -o $(BIN_DIR)$(TARGET)

kernel: $(GEN_SRC_DIR)$(KERNEL_SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(AOC) $(AOC_PARAMS) $(COMMON_FLAGS) -o $(BIN_DIR)$(KERNEL_TARGET) $(GEN_SRC_DIR)$(KERNEL_SRCS)

kernel_emulate: $(GEN_SRC_DIR)$(KERNEL_SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(AOC) -march=emulator $(COMMON_FLAGS) -o $(BIN_DIR)$(KERNEL_TARGET)_emulate $(GEN_SRC_DIR)$(KERNEL_SRCS)

run_emu: cleangen host kernel_emulate
	CL_CONTEXT_EMULATOR_DEVICE_INTELFPGA=1 gdb --args $(BIN_DIR)$(TARGET) $(BIN_DIR)$(KERNEL_TARGET)_emulate.aocx

kernel_profile: $(GEN_SRC_DIR)$(KERNEL_SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(AOC) $(AOC_PARAMS) $(COMMON_FLAGS) -profile -o $(BIN_DIR)$(KERNEL_TARGET)_profile $(GEN_SRC_DIR)$(KERNEL_SRCS)

kernel_report: $(GEN_SRC_DIR)$(KERNEL_SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(AOC) $(AOC_PARAMS) $(COMMON_FLAGS) -rtl -o $(BIN_DIR)$(KERNEL_TARGET)_report $(GEN_SRC_DIR)$(KERNEL_SRCS) -report

cleanhost:
	rm -f $(BIN_DIR)$(TARGET)

cleangen:
	rm -rf $(GEN_SRC_DIR)

cleanall: cleanhost cleangen
	rm -rf $(BIN_DIR)
