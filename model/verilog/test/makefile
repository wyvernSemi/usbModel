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
# modelsim.ini file and binary directory respectively

#
# User modifiable flags
#
USRFLAGS      = -DUSBTESTMODE
ARCH          = -m32

NUM_VPROC     = 2

# Location of VProc directory. Assumes in same directory as usb.
# Update if relocated.
VPROC_TOP     = ../../../../vproc

# Define the github repository URL for the VProc virtual processor
VPROC_REPO    = https://github.com/wyvernSemi/vproc.git

#
# USB VHost test definitions
#
USBVLOGDIR    = ../src
SRCDIR        = ../../src
USRCDIR       = usercode
WORKDIR       = work

#
# PLI C/C++ code definitions
#
PLI_SO        = VProc.so
VPROC_LIB     = libvproc.a
VPROCLIBPATH  = ${VPROC_TOP}/${VPROC_LIB}

#
# Test c++ code
#
USER_CPP      = VUserMain0.cpp                   \
                VUserMain1.cpp                   \
                veriuser.cpp

#
# All source files (model and user code)
#
ALLSRC        = ${SRCDIR}/*.cpp ${USER_CPP:%.cpp=${USRCDIR}/%.cpp}

#
# Flags for simulator
#
TOP           = test
TOP_VC        = test.vc
VSIMFLAGS     = -pli ${PLI_SO} ${TOP}
VLOGFLAGS     = -quiet -incr +incdir+${VPROC_TOP} +incdir+${USBVLOGDIR} -f ${TOP_VC}

#
# Flags for C compiler
#
CC            = gcc
C++           = g++
CFLAGS        = ${ARCH} -fPIC                    \
                -Wno-write-strings               \
                -I${SRCDIR}                      \
                -I${USRCDIR}                     \
                -I${VPROC_TOP}/code              \
                -I${MODEL_TECH}/../include       \
                ${USRFLAGS}

# Get OS type
OSTYPE       :=$(shell uname)

# Set OS specific variables between Linux and Windows (MinGW)
ifeq (${OSTYPE}, Linux)
  CFLAGS_SO   = -shared -lpthread -lrt -rdynamic
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
all: ${PLI_SO} verilog

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
${PLI_SO}: ${VPROCLIBPATH} ${ALLSRC} ${SRCDIR}/*.h ${USRCDIR}/*.h
	@${C++} ${CFLAGS_SO} ${CFLAGS}               \
	       -Wl,-whole-archive                    \
 	       ${VPROCLIBPATH}                       \
           ${ALLSRC}                             \
           -lpthread                             \
           -L${MODEL_TECH}                       \
           -lmtipli                              \
	       -Wl,-no-whole-archive                 \
	       -o $@

#
# VProc library.
#
${VPROCLIBPATH}: ${VPROC_TOP}
	@${MAKE} --no-print-directory -C ${VPROC_TOP} MAX_NUM_VPROC=${NUM_VPROC} USER_C="" ${VPROC_LIB}

#
# Checkout VProc from github if it doesn't exist at the specified location
#
${VPROC_TOP}:
	git clone ${VPROC_REPO} ${VPROC_TOP} --recursive

#------------------------------------------------------
# EXECUTION RULES
#------------------------------------------------------

run: ${VPROCLIBPATH} verilog ${PLI_SO}
	@vsim -c -GGUI_RUN=0 ${VSIMFLAGS} -do "run -all"

rungui: ${VPROCLIBPATH} verilog ${PLI_SO}
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
	@${MAKE} --no-print-directory -C ${VPROC_TOP}     clean
	@rm -rf ${PLI_SO} *.wlf transcript work


