//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 16th Novenmber 2023
//
// Contains the usbModel formatting functions
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

#include <stdio.h>
#include <stdint.h>

#include "usbFormat.h"
#include "usbCommon.h"

namespace usbModel
{
    const char* fmtDecriptorType(const uint8_t desc)
    {
        switch(desc)
        {
        case USB_DEV_DESCRIPTOR_TYPE : return "USB_DEV_DESCRIPTOR_TYPE";
        case USB_CFG_DESCRIPTOR_TYPE : return "USB_CFG_DESCRIPTOR_TYPE";
        case USB_STR_DESCRIPTOR_TYPE : return "USB_STR_DESCRIPTOR_TYPE";
        case USB_IF_DESCRIPTOR_TYPE  : return "USB_IF_DESCRIPTOR_TYPE";
        case USB_EP_DESCRIPTOR_TYPE  : return "USB_EP_DESCRIPTOR_TYPE";
        case USB_FUNC_DESCRIPTOR_TYPE: return "USB_FUNC_DESCRIPTOR_TYPE";
        default:                       return "UNKNOWN";
        }
    }
    
    
    void fmtDevDescriptor(char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        usbModel::deviceDesc *desc = (deviceDesc*)rawdata;
        
        snprintf(sbuf, maxstrsize,
                     FMT_DATA_GREY
                     "  bLength            = %d\n"
                     "  bDescriptorType    = %s\n"
                     "  bcdUSB             = 0x%04x\n"
                     "  bDeviceClass       = 0x%02x\n"
                     "  bDeviceSubClass    = 0x%02x\n"
                     "  bDeviceProtocol    = 0x%02x\n"
                     "  bMaxPacketSize     = 0x%02x\n"
                     "  idVendor           = 0x%04x\n"
                     "  idProduct          = 0x%04x\n"
                     "  bcdDevice          = 0x%04x\n"
                     "  iManufacturer      = 0x%02x\n"
                     "  iProduct           = 0x%02x\n"
                     "  iSerialNumber      = 0x%02x\n"
                     "  bNumConfigurations = 0x%02x\n\n"
                     FMT_NORMAL,
                     desc->bLength, fmtDecriptorType(desc->bDescriptorType), desc->bcdUSB, desc->bDeviceClass,
                     desc->bDeviceSubClass, desc->bDeviceProtocol, desc->bMaxPacketSize, desc->idVendor,
                     desc->idProduct, desc->bcdDevice, desc->iManufacturer, desc->iProduct,
                     desc->iSerialNumber, desc->bNumConfigurations);
    }
    
    const char* fmtLineState(const unsigned linestate)
    {
        return (linestate == usbModel::USB_K)   ? "K" : 
               (linestate == usbModel::USB_J)   ? "J" :
               (linestate == usbModel::USB_SE0) ? "SE0" :
               (linestate == usbModel::USB_SE1) ? "SE1" : "?";
    }
}