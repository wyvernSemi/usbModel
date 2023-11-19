//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 13th November 2023
//
// Contains the usbModel formatting, error and debug
// definitions and macros
//
// This file is part of the C++ usbModel
//
// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================

#ifndef _USB_FORMAT_H_
#define _USB_FORMAT_H_

#include "usbCommon.h"

// Definitions for use in formatted packet information display

//#define USBNOFORMAT

# ifndef USBNOFORMAT

#define FMT_NORMAL              "\033[0m"
#define FMT_BOLD                "\033[1m"
#define FMT_FAINT               "\033[2m"
#define FMT_ITALIC              "\033[3m"
#define FMT_UNDERLINE           "\033[4m"

#define FMT_BLACK               "\033[30m"
#define FMT_RED                 "\033[31m"
#define FMT_GREEN               "\033[32m"
#define FMT_YELLOW              "\033[33m"
#define FMT_BLUE                "\033[34m"
#define FMT_MAGENTA             "\033[35m"
#define FMT_CYAN                "\033[36m"
#define FMT_WHITE               "\033[37m"
#define FMT_BRIGHT_BLACK        "\033[90m"
#define FMT_BRIGHT_RED          "\033[91m"
#define FMT_BRIGHT_GREEN        "\033[92m"
#define FMT_BRIGHT_YELLOW       "\033[93m"
#define FMT_BRIGHT_BLUE         "\033[94m"
#define FMT_BRIGHT_MAGENTA      "\033[95m"
#define FMT_BRIGHT_CYAN         "\033[96m"
#define FMT_BRIGHT_WHITE        "\033[97m"

#define FMT_DATA_GREY           "\033[38;5;244m"

# else
    
#define FMT_NORMAL              ""
#define FMT_BOLD                ""
#define FMT_FAINT               ""
#define FMT_ITALIC              ""
#define FMT_UNDERLINE           ""

#define FMT_BLACK               ""
#define FMT_RED                 ""
#define FMT_GREEN               ""
#define FMT_YELLOW              ""
#define FMT_BLUE                ""
#define FMT_MAGENTA             ""
#define FMT_CYAN                ""
#define FMT_WHITE               ""
#define FMT_BRIGHT_BLACK        ""
#define FMT_BRIGHT_RED          ""
#define FMT_BRIGHT_GREEN        ""
#define FMT_BRIGHT_YELLOW       ""
#define FMT_BRIGHT_BLUE         ""
#define FMT_BRIGHT_MAGENTA      ""
#define FMT_BRIGHT_CYAN         ""
#define FMT_BRIGHT_WHITE        ""

#define FMT_DATA_GREY           ""

# endif

#define FMT_DEVICE              FMT_BRIGHT_BLUE FMT_BOLD
#define FMT_HOST                FMT_RED FMT_BOLD

// Macro for constructing error messages into an error buffer. Default enabled.
#ifndef DISABLEUSBDEBUG
#define USBERRMSG(...) {snprintf(errbuf, usbModel::ERRBUFSIZE, __VA_ARGS__);}
#else
#define USBERRMSG(...) {}
#endif

// Macro for displaying received packet information. Default enabled.
#ifndef DISABLEUSBDISPPKT
#define USBDISPPKT(...) {fprintf(stderr, __VA_ARGS__);}
#else
#define USBDISPPKT(...) {}
#endif

//#define ENABLEDEVDEBUG

// Macro for displaying development debug messages. Default disabled
#ifdef ENABLEDEVDEBUG
#define USBDEVDEBUG(...) {fprintf(stderr, __VA_ARGS__);}
#else
#define USBDEVDEBUG(...) {}
#endif

namespace usbModel
{
int         fmtDevDescriptor          (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);
int         fmtCfgDescriptor          (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);
int         fmtIfDescriptor           (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);
int         fmtEpDescriptor           (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);
int         fmtHdrFuncDescriptor      (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);
int         fmtAcmFuncDescriptor      (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);
int         fmtUnionFuncDescriptor    (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);
int         fmtCallMgmtFuncDescriptor (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);
int         fmtCfgAllDescriptor       (char sbuf[], const uint8_t rawdata[], const unsigned indent = 0, const int maxstrsize = ERRBUFSIZE);

const char* fmtFuncDescSubtype        (const uint8_t subtype);
const char* fmtDecriptorType          (const uint8_t desc);
const char* fmtLineState              (const unsigned linestate);

int         fmtStrToUnicode           (uint16_t* dst, const char*     src, const int maxstrsize = MAXSTRDESCSTRING);
int         fmtUnicodeToStr           (char*     dst, const uint16_t* src, const int length, const int maxstrsize = MAXSTRDESCSTRING);
}

#endif
