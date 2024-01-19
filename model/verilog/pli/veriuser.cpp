//=====================================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// This file is part of C++ usbModel.
//
// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
//=====================================================================
// 
// PLI task/function registration table for plitask
// 
//=====================================================================

extern "C"
{
// Definitions of VProc PLI C functions
#include "VSched_pli.h"

#ifndef VPROC_PLI_VPI

#ifdef ICARUS
typedef int (*p_tffn)(int, int);
#endif

char *veriuser_version_str = "Virtual Processor PLI V0.1 Copyright (c) 2005-2023 Simon Southwell.";

// Define the PLI task table for just the tasks from VProc needed
s_tfcell veriusertfs[4] =
{
    {usertask, 0, NULL, 0, (p_tffn)VInit,     NULL,  "$vinit",     1},
    {usertask, 0, NULL, 0, (p_tffn)VSched,    NULL,  "$vsched",    1},
    {usertask, 0, NULL, 0, (p_tffn)VIrq,      NULL,  "$virq",      1},
    {0} 
};

p_tfcell bootstrap ()
{
    return veriusertfs;
}

#ifdef ICARUS
static void veriusertfs_register(void)
{
    veriusertfs_register_table(veriusertfs);
}

void (*vlog_startup_routines[])() = { &veriusertfs_register, 0 };
#endif

#endif

}
