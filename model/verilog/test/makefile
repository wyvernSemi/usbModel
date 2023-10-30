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

# MODEL_TECH environment variable must be set for pointing to 
# modelsim.ini file and binary dierctory respectively

NUM_VPROC     = 2

# Location of VProc directory. Assumes in same directory as usb.
# Update if relocated.
VPROC_TOP     = ../../../../vproc

# Define the github repository URL for the VProc virtual processor 
VPROC_REPO    = https://github.com/wyvernSemi/vproc.git

#
# USB VHost test defintions
#
USBVLOGDIR    = ../src
SRCDIR        = ../../src
USRCDIR       = usercode
WORKDIR       = work

PLI_SO        = VProc.so

USER_CPP      = VUserMain0.cpp VUserMain1.cpp veriuser.cpp

#
# Flags for ModelSim
#
TOP           = test
TOP_VC        = test.vc
HDRDIR        = ../headers

VSIMFLAGS     = -pli ${PLI_SO} ${TOP}
VLOGFLAGS     = -quiet -incr +incdir+${VPROC_TOP} +incdir+${USBVLOGDIR} -f ${TOP_VC}

#
# Flags for C compiler
#
CC            = gcc
C++           = g++
ARCH          = -m32
CFLAGS        = ${ARCH} -fPIC                    \
                -Wno-write-strings               \
                -I${SRCDIR}                      \
                -I${USRCDIR}                     \
                -I${HDRDIR}                      \
                -I${VPROC_TOP}/code              \
                -I${MODEL_TECH}/../include       \
                -DMODELSIM                       \
                -D_REENTRANT

# Get OS type
OSTYPE       :=$(shell uname)

# Set OS specific variables between Linux and Windows (MinGW)
ifeq (${OSTYPE}, Linux)
  CFLAGS_SO   = -shared -lpthread -lrt -rdynamic
  CPPSTD      = -std=c++11
else
  CFLAGS_SO   = -shared -Wl,-export-all-symbols
  CPPSTD      = -std=c+11
endif

#------------------------------------------------------
# BUILD RULES
#------------------------------------------------------

#
# Build the verilog and the PLI shared object by default
#
all: ${VPROC_SO} verilog

#
# Verilog. Phony target
#
.PHONY: verilog

verilog:
	@if [ ! -d ${WORKDIR} ]; then                \
	  vlib ${WORKDIR};                           \
    fi
	@vlog ${VLOGFLAGS}


#
# PLI shared object
#
${PLI_SO}: vproc_lib ${SRCDIR}/*.cpp ${USER_CPP:%.cpp=${USRCDIR}/%.cpp}
	@${C++} ${CFLAGS_SO} ${CFLAGS}               \
	       -Wl,-whole-archive                    \
 	       ${USER_CPP:%.cpp=${USRCDIR}/%.cpp}    \
 	       ${SRCDIR}/*.cpp                       \
           -lpthread                             \
           -L${MODEL_TECH}                       \
           -lmtipli                              \
	       -L${VPROC_TOP}     -lvproc            \
	       -Wl,-no-whole-archive                 \
	       -o $@

#
# VProc library.
#
vproc_lib: ${VPROC_TOP}
	@${MAKE} -C ${VPROC_TOP} MAX_NUM_VPROC=${NUM_VPROC} USER_C=""

#
# Checkout VProc from github if it doesn't exist at the specified location
#
${VPROC_TOP}:
	git clone ${VPROC_REPO} ${VPROC_TOP} --recursive

#------------------------------------------------------
# EXECUTION RULES
#------------------------------------------------------

run: vproc_lib verilog ${PLI_SO}
	@vsim -c ${VSIMFLAGS} -do "run -all"

rungui: vproc_lib verilog ${PLI_SO}
	@if [ -e wave.do ]; then                                                \
         vsim -gui -GGUI_RUN=1 -do wave.do ${VSIMFLAGS} -do "run -all";     \
     else                                                                   \
         vsim -gui -GGUI_RUN=1 ${VSIMFLAGS};                                \
     fi

gui: rungui

#------------------------------------------------------
# CLEANING RULES
#------------------------------------------------------

clean:
	@if [ -d "${WORKDIR}" ]; then                \
	  vdel -all;                                 \
     fi
	@${MAKE} -C ${VPROC_TOP}     clean
	@rm -rf ${PLI_SO} *.wlf transcript


