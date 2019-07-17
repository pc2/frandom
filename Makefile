##
#  Author: Marius Meyer
#  eMail:  marius.meyer@upb.de
#  Date:   2019/04/05
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

KERNEL_SRCS := random_access_kernels.cl
KERNEL_INPUTS = $(KERNEL_SRCS:.cl=.aocx)

BOARD := p520_max_sg280l

# Used local memory size should be half of the available M20K size
#
# For Stratix 10, 229 Mbits are available:
#      229000000 / (8 * 8) = 3.578.125 long values.
#      Final size should be: 1.750.000 long values
GLOBAL_MEM_SIZE := 268435456

BIN_DIR := bin/
SRC_DIR := src/
GEN_SRC_DIR := generated/

ifdef BUILD_SUFFIX
	EXT_BUILD_SUFFIX := _$(BUILD_SUFFIX)
endif

##
# Change aoc params to the ones desired (emulation,simulation,synthesis).
#
AOC_PARAMS := $(AOC_PARAMS) -board=$(BOARD) 

GEN_SRC := random_channels.cpp
GEN_KERNEL_SRC := random_access_kernels_channels.cl

SRCS := random.cpp
TARGET := $(SRCS:.cpp=)$(EXT_BUILD_SUFFIX)
KERNEL_TARGET := $(KERNEL_SRCS:.cl=)$(EXT_BUILD_SUFFIX)
REPLICATIONS := 4


$(info BOARD               = $(BOARD))
$(info BUILD_SUFFIX        = $(BUILD_SUFFIX))
$(info AOC_PARAMS          = $(AOC_PARAMS))

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
	$(info emulate_kernel               = Compile  global memory kernel for emulation)
	$(info kernel_profile               = Compile  global memory kernel with profiling information enabled)
	$(info kernel_profile               = Compile  global memory kernel with profiling information enabled)	
	$(info ************************************************)
	$(info info                         = Print this list of available targets)
	$(info ************************************************)
	$(info Additional compile flags for the kernels can be provided in AOC_PARAMS.)
	$(info To disable memory interleaving: make kernel AOC_PARAMS=-no-interleaving=default)

$(SRCS):
	$(MKDIR_P) $(GEN_SRC_DIR)
	$(CODE_GENERATOR) $(SRC_DIR)host/$(GEN_SRC) -o $(SRC_DIR)host/$(SRCS) -p replications=$(REPLICATIONS)

$(KERNEL_SRCS):
	$(MKDIR_P) $(GEN_SRC_DIR)
	$(CODE_GENERATOR) $(SRC_DIR)device/$(GEN_KERNEL_SRC) -o $(SRC_DIR)device/$(KERNEL_SRCS) -p replications=$(REPLICATIONS)

host: $(SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(CXX) $(AOCL_COMPILE_CONFIG) -DDATA_LENGTH=$(GLOBAL_MEM_SIZE) \
	-DFPGA_KERNEL_FILE_GLOBAL=\"$(KERNEL_TARGET).aocx\" \
	$(GEN_SRC_DIR)$(SRCS) $(AOCL_LINK_CONFIG) -o $(BIN_DIR)$(TARGET)

kernel: $(KERNEL_SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(AOC) $(AOC_PARAMS) -o $(BIN_DIR)$(KERNEL_TARGET)_local $(GEN_SRC_DIR)$(KERNEL_SRCS)

emulate_kernel: $(KERNEL_SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(AOC) -march=emulator -o $(BIN_DIR)$(KERNEL_TARGET)_emulate $(GEN_SRC_DIR)$(KERNEL_SRCS)

kernel_profile: $(KERNEL_SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(AOC) $(AOC_PARAMS) -profile -o $(BIN_DIR)$(KERNEL_TARGET)_profile $(GEN_SRC_DIR)$(KERNEL_SRCS)

kernel_report: $(KERNEL_SRCS)
	$(MKDIR_P) $(BIN_DIR)
	$(AOC) $(AOC_PARAMS) -rtl -o $(BIN_DIR)$(KERNEL_TARGET)_report $(GEN_SRC_DIR)$(KERNEL_SRCS) -report

cleanhost:
	rm -f $(BIN_DIR)$(TARGET)

cleanall: cleanhost
	rm -f *.aoco *.aocr *.aocx *.source
	rm -rf $(KERNEL_SRCS:.cl=)
	rm -rf $(BIN_DIR)
	rm -rf $(GEN_SRC_DIR)
