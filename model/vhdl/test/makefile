###################################################################
# Makefile for usbModel test bench for Questa logic simulator
#
# Copyright (c) 2024 Simon Southwell.
#
# This file is part of tcpIpPg.
#
# This file is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# The file is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this file. If not, see <http://www.gnu.org/licenses/>.
#
###################################################################

#------------------------------------------------------
# User overridable definitions
#------------------------------------------------------

HDL                = VHDL
ARCHFLAG           = -m64

#------------------------------------------------------
# Internal variables
#------------------------------------------------------

# User files to build, passed into vproc makefile build
USERCODE           = VUserMain0.cpp \
                     VUserMain1.cpp

USBCODE            = usbDevice.cpp  \
                     usbHost.cpp    \
                     usbFormat.cpp  \
                     usbPkt.cpp

USRCDIR            = $(CURDIR)/usercode
USBCDIR            = $(CURDIR)/../../src

# Set up Variables for tools
MAKE_EXE           = make
VSIMEXE            = $(MODEL_TECH)/vsim
VSIMARGS           =

# VProc location, relative to this directory
VPROC_TOP          = ../../../../vproc

# Define the github repository URL for the VProc virtual processor
VPROC_REPO         = https://github.com/wyvernSemi/vproc.git

USRCFLAGS          = -I$(CURDIR)/../../src -DUSBTESTMODE

#
# Usb C++ auto-generated memory map for Verilog
# accessible signals and registers
#
USBVLOGDIR         = ../../verilog/src
USBCMAP            = $(USRCDIR)/usbMap.h
USBVLOGMAP         = $(USBVLOGDIR)/usbModel.vh

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

PLI_SO             = VProc.so

# Specify the VPr  oc makefile and .do files to use with simulator
MAKEFILEARG        = HDL=$(HDL)
SIMDO              = sim_vhdl.do
SIMGDO             = simg_vhdl.do
SIMLOGDO           = simlog_vhdl.do
COMPDO             = compile_vhdl.do

#------------------------------------------------------
# Flags for simulator
#------------------------------------------------------

TOP           = test
TOP_VC        = test.vc
VSIMFLAGS     = -pli $(PLI_SO) $(TOP)
VHDLFLAGS     = -2008 -f files_vhdl.tcl

#------------------------------------------------------
# BUILD RULES
#------------------------------------------------------

.PHONY : vhdl, vproc, all, run, rungui, runlog, compile, waves, gui, log help, clean

# Build is dependant on processing makefile in vproc and riscV/iss
all: vproc vhdl

#
# Check that the MODEL_TECH environment variable is defined
#
.PHONY: checkenv
checkenv:
ifndef MODEL_TECH
	$(error MODEL_TECH is undefined)
endif

# Call the vproc make file for it to determine if anything
# needs building.
vproc: checkenv $(USBCMAP) $(VPROC_TOP)
	@$(MAKE_EXE) --no-print-directory              \
                 $(MAKEFILEARG)                    \
                 $(CURDIR)/VProc.so                \
                 -C $(VPROC_TOP)/test              \
                 USRFLAGS="$(USRCFLAGS)"           \
                 ARCHFLAG=$(ARCHFLAG)              \
                 USRCDIR=$(USRCDIR)                \
                 USER_C="$(USERCODE)"              \
                 MEM_C="$(USBCODE)"                \
                 MEMMODELDIR="$(USBCDIR)"          \
                 TESTDIR=$(CURDIR)                 \
                 MODELSIMDIR="$(MODEL_TECH)\.."

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

run: vproc
	@$(VSIMEXE) -c -GGUI_RUN=0 -do sim_vhdl.do

sim: vproc
	@$(VSIMEXE) -c -GGUI_RUN=0 -do sim_vhdl_norun.do

rungui: vproc
	@$(VSIMEXE) -gui -GGUI_RUN=1 -do simg_vhdl.do 

simgui: vproc
	@$(VSIMEXE) -gui -GGUI_RUN=1 -do simg_vhdl_norun.do

gui: rungui

help:
	@echo "make help          Display this message"
	@echo "make               Build C/C++ code without running simulation"
	@echo "make simgui        Build and run GUI simulation (sim not started)"
	@echo "make sim           Build and run command line interactive (sim not started)"
	@echo "make run           Build and run batch simulation"
	@echo "make rungui/gui    Build and run GUI simulation"
	@echo "make clean         clean previous build artefacts"

#------------------------------------------------------
# CLEANING RULES
#------------------------------------------------------

clean: $(VPROC_TOP)
	@$(MAKE_EXE) --no-print-directory -C $(VPROC_TOP)/test USER_C="$(USERCODE)" TESTDIR="$(CURDIR)" clean
	@vsim -c -do cleanvlib.do -do quit 2>&1 > /dev/null
	@rm -rf transcript *wlf*
