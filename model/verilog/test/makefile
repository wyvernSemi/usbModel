###################################################################
# Makefile for Virtual USB code in Modelsim
#
# Copyright (c) 2023 Simon Southwell.
#
# This file is part of usbModel pattern generator.
#
# This code is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The code is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this code. If not, see <http://www.gnu.org/licenses/>.
#
###################################################################

#------------------------------------------------------
# User modifiable flags

USRFLAGS      = -DUSBTESTMODE
USRSRCDIR     = usercode
USER_CPP      = VUserMain0.cpp VUserMain1.cpp
ARCHFLAG      = -m64

#------------------------------------------------------
# Set MODEL_TECH if absent
#------------------------------------------------------

# Get OS type
OSTYPE             = $(shell uname)

# If run from a place where MODEL_TECH is not defined, construct from path to PLI library
ifeq ("$(MODEL_TECH)", "")
  ifeq ($(OSTYPE), Linux)
    PLILIB         = libmtipli.so
  else
    PLILIB         = mtipli.dll
  endif
  
  VSIMPATH         = $(shell which vsim)
  SIMROOT          = $(shell dirname $(VSIMPATH))/..
  PLILIBPATH       = $(shell find $(SIMROOT) -name "$(PLILIB)")
  MODEL_TECH       = $(shell dirname $(PLILIBPATH))
endif

#------------------------------------------------------
# Definitions for VProc virtual processor
#------------------------------------------------------

#
# Only need two virtual processor nodes
#
NUM_VPROC     = 2

#
# Location of VProc directory. Assumes in same directory as usbModel repository.
# Update/override if located elsewhere.
#
VPROC_TOP     = ../../../../vproc

#
# Define the github repository URL for the VProc virtual processor
#
VPROC_REPO    = https://github.com/wyvernSemi/vproc.git

#
# USB VHost test definitions
#
USBVLOGDIR    = ../src
PLIDIR        = ../pli
SRCDIR        = ../../src
WORKDIR       = work

#------------------------------------------------------
# User code definitions (non-modifiable)
#------------------------------------------------------

#
# Find any user code header files (if any)
#
USRHDRS       = $(wildcard $(USRSRCDIR)/*.h)

#
# Get all the header files for the model source code
#
SRCHDRS       = $(wildcard $(SRCDIR)/*.h)

#
# Usb C++ auto-generated memory map for Verilog
# accessible signals and registers
#
USBCMAP       = $(USRSRCDIR)/usbMap.h

#------------------------------------------------------
# PLI shared object definitions
#------------------------------------------------------

#
# PLI C/C++ code definitions
#
PLI_SO        = VProc.so
VPROC_LIB     = libvproc.a
VPROCLIBPATH  = $(VPROC_TOP)/test/$(VPROC_LIB)
USBVLOGMAP    = $(USBVLOGDIR)/usbModel.vh

#
# All source files (model and user code)
#
ALLSRC        = $(SRCDIR)/*.cpp $(PLIDIR)/*.cpp $(USER_CPP:%.cpp=$(USRSRCDIR)/%.cpp) $(USBCMAP)

#------------------------------------------------------
# Flags for simulator
#------------------------------------------------------

TOP           = test
TOP_VC        = test.vc
VSIMFLAGS     = -pli $(PLI_SO) $(TOP)
VLOGFLAGS     = -quiet -incr +incdir+$(VPROC_TOP) +incdir+$(USBVLOGDIR) -f $(TOP_VC)

#------------------------------------------------------
# Flags for C compiler
#------------------------------------------------------
CC            = gcc
C++           = g++
CFLAGS        = $(ARCHFLAG) -fPIC                \
                -Wno-write-strings               \
                -I$(SRCDIR)                      \
                -I$(PLIDIR)                      \
                -I$(USRSRCDIR)                   \
                -I$(VPROC_TOP)/code              \
                -I$(MODEL_TECH)/../include       \
                $(USRFLAGS)

# Get OS type
OSTYPE       :=$(shell uname)

# Set OS specific variables between Linux and Windows (MinGW)
ifeq ($(OSTYPE), Linux)
  CFLAGS_SO   = -shared -lpthread -lrt -rdynamic -Wno-format-truncation
else
  CFLAGS_SO   = -shared -Wl,-export-all-symbols
endif

# C++ compilation standard
CPPSTD        = -std=c+11

#------------------------------------------------------
# BUILD RULES
#------------------------------------------------------

#
# Build the verilog and the PLI shared object by default
#
all: $(PLI_SO) verilog

#
# Check that the MODEL_TECH environment variable is defined
#
.PHONY: checkenv
checkenv:
ifndef MODEL_TECH
	$(error MODEL_TECH is undefined)
endif

#
# Verilog
#
.PHONY: verilog
verilog:
	@if [ ! -d $(WORKDIR) ]; then                \
	  vlib $(WORKDIR);                           \
    fi
	@vlog $(VLOGFLAGS)

#
# PLI shared object
#
$(PLI_SO): checkenv $(VPROCLIBPATH) $(ALLSRC) $(SRCHDRS) $(USRHDRS)
	@$(C++) $(CFLAGS_SO) $(CFLAGS)               \
	       -Wl,-whole-archive                    \
 	       $(VPROCLIBPATH)                       \
           $(ALLSRC)                             \
           -lpthread                             \
           -L$(MODEL_TECH)                       \
           -lmtipli                              \
	       -Wl,-no-whole-archive                 \
	       -o $@

#
# VProc library.
#
$(VPROCLIBPATH): $(VPROC_TOP)
	@$(MAKE) --no-print-directory -C $(VPROC_TOP)/test MAX_NUM_VPROC=$(NUM_VPROC) ARCHFLAG=$(ARCHFLAG) USER_C="" $(VPROC_LIB)

#
# Checkout VProc from github if it doesn't exist at the specified location
#
$(VPROC_TOP):
	@git clone $(VPROC_REPO) $(VPROC_TOP) --recursive

#
# Auto-generate the C++ header from the Verilog header so they always match
#
$(USBCMAP): $(USBVLOGMAP)
	@sed -e 's/`/#/g' -e 's/_USB_VH_/_USB_MAP_H_/g' < $^ > $@

#------------------------------------------------------
# EXECUTION RULES
#------------------------------------------------------

run: $(VPROCLIBPATH) verilog $(PLI_SO)
	@vsim -c -GGUI_RUN=0 $(VSIMFLAGS) -do "run -all"

sim: $(VPROCLIBPATH) verilog $(PLI_SO)
	@vsim -c -GGUI_RUN=0 $(VSIMFLAGS)

rungui: $(VPROCLIBPATH) verilog $(PLI_SO)
	@if [ -e wave.do ]; then                                                \
         vsim -gui -GGUI_RUN=1 -do wave.do $(VSIMFLAGS) -do "run -all";     \
     else                                                                   \
         vsim -gui -GGUI_RUN=1 $(VSIMFLAGS);                                \
     fi

simgui: $(VPROCLIBPATH) verilog $(PLI_SO)
	@if [ -e wave.do ]; then                                                \
         vsim -gui -GGUI_RUN=1 -do wave.do $(VSIMFLAGS);                    \
     else                                                                   \
         vsim -gui -GGUI_RUN=1 $(VSIMFLAGS);                                \
     fi

gui: rungui

help:
	@echo "make help          Display this message"
	@echo "make               Build C/C++ code without running simulation"
	@echo "make sim           Build and run command line interactive (sim not started)"
	@echo "make run           Build and run batch simulation"
	@echo "make rungui/gui    Build and run GUI simulation"
	@echo "make clean         clean previous build artefacts"

#------------------------------------------------------
# CLEANING RULES
#------------------------------------------------------

clean: $(VPROC_TOP)
	@if [ -d "$(WORKDIR)" ]; then                \
	  vdel -all;                                 \
     fi
	@$(MAKE) --no-print-directory -C $(VPROC_TOP)/test clean
	@rm -rf $(PLI_SO) $(USBCMAP) *.wlf transcript work


