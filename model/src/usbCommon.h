//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 8th Novenmber 2023
//
// Contains the code for the usbModel common definitions
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

#ifndef _USB_COMMON_H_
#define _USB_COMMON_H_

// -------------------------------------------------------------------------
// Define a name space for all the common constant values and type
// definitions for the usbModel code.
// -------------------------------------------------------------------------

namespace usbModel
{
    // USB1.0
    static const int      PID_SPCL_PREAMB          = 0xc;

    // USB1.1
    static const int      NRZI_BITSPERBYTE         = 8;
    static const int      NRZI_BYTELSBMASK         = 1;
    static const int      NRZI_BYTEMSBMASK         = 0x80;

    static const int      PID_TOKEN_OUT            = 0x1;
    static const int      PID_TOKEN_IN             = 0x9;
    static const int      PID_TOKEN_SOF            = 0x5;
    static const int      PID_TOKEN_SETUP          = 0xd;

    static const int      PID_DATA_0               = 0x3;
    static const int      PID_DATA_1               = 0xb;

    static const int      PID_HSHK_ACK             = 0x2;
    static const int      PID_HSHK_NAK             = 0xa;
    static const int      PID_HSHK_STALL           = 0xe;

    // USB2.0
    static const int      PID_HSHK_NYET            = 0x6;
    static const int      PID_TOKEN_ERR            = 0xc;
    static const int      PID_TOKEN_SPLIT          = 0x8;
    static const int      PID_TOKEN_PING           = 0x4;
    static const int      PID_DATA_2               = 0x7;
    static const int      PID_DATA_M               = 0xf;

    static const int      PID_INVALID              = -1;

    static const int      SYNC                     = 0x80;

    static const int      SE0P                     = 0xfc;
    static const int      SE0M                     = 0x00;

    static const int      MAXDEVADDR               = 127;
    static const int      MAXENDP                  = 15;
    static const int      MAXFRAMENUM              = 4095;
    static const int      MAXONESLENGTH            = 6;

    static const int      POLY16                   = 0x8005;
    static const unsigned BIT16                    = 0x8000U;

    static const int      POLY5                    = 0x5;
    static const unsigned BIT5                     = 0x10U;

    static const int      USBOK                    = 0;
    static const int      USBERROR                 = -1;
    static const int      USBUNSUPPORTED           = -2;
    static const int      USBRESET                 = -3;
    static const int      USBSUSPEND               = -4;
    static const int      ERRBUFSIZE               = 8192;
    static const int      MAXBUFSIZE               = 2048;

    static const int      MINPKTSIZEBITS           = 16;

    static const int      SYNCBYTEOFFSET           = 0;
    static const int      PIDBYTEOFFSET            = 1;
    static const int      FRAMEBYTEOFFSET          = 2;
    static const int      ADDRBYTEOFFSET           = 2;
    static const int      ENDPBYTEOFFSET           = 2;
    static const int      DATABYTEOFFSET           = 2;
    static const int      CRC5BYTEOFFSET           = 3;

    static const int      ARGADDRIDX               = 0;
    static const int      ARGENDPIDX               = 1;
    static const int      ARGTKNCRC5IDX            = 2;

    static const int      ARGFRAMEIDX              = 0;
    static const int      ARGSOFCRC5IDX            = 1;

    static const int      ARGCRC16IDX              = 0;

    static const int      USB_SE0                  = 0;
    static const int      USB_SE1                  = 3;
    static const int      USB_J                    = 1;
    static const int      USB_K                    = 2;

    static const uint8_t  USB_REQ_GET_STATUS       = 0x00;
    static const uint8_t  USB_REQ_CLEAR_FEATURE    = 0x01;
    static const uint8_t  USB_REQ_SET_FEATURE      = 0x03;
    static const uint8_t  USB_REQ_SET_ADDRESS      = 0x05;
    static const uint8_t  USB_REQ_GET_DESCRIPTOR   = 0x06;
    static const uint8_t  USB_REQ_SET_DESCRIPTOR   = 0x07;
    static const uint8_t  USB_REQ_GET_CONFIG       = 0x08;
    static const uint8_t  USB_REQ_SET_CONFIG       = 0x09;
    static const uint8_t  USB_REQ_GET_INTERFACE    = 0x0a;
    static const uint8_t  USB_REQ_SET_INTERFACE    = 0x11;
    static const uint8_t  USB_REQ_SYNCH_FRAME      = 0x12;

    static const uint8_t  USB_DEV_REQTYPE_SET      = 0x00;
    static const uint8_t  USB_DEV_REQTYPE_GET      = 0x80;
    static const uint8_t  USB_IF_REQTYPE_SET       = 0x01;
    static const uint8_t  USB_IF_REQTYPE_GET       = 0x81;
    static const uint8_t  USB_EP_REQTYPE_SET       = 0x02;
    static const uint8_t  USB_EP_REQTYPE_GET       = 0x82;

    static const uint8_t  USB_REMOTE_WAKEUP_OFF    = 0x0000;
    static const uint8_t  USB_REMOTE_WAKEUP_ON     = 0x0002;

    static const uint8_t  USB_NOT_SELF_POWERED     = 0x0000;
    static const uint8_t  USB_SELF_POWERED         = 0x0001;

    static const uint8_t  USB_DEV_DESCRIPTOR_TYPE  = 0x01;
    static const uint8_t  USB_CFG_DESCRIPTOR_TYPE  = 0x02;
    static const uint8_t  USB_STR_DESCRIPTOR_TYPE  = 0x03;
    static const uint8_t  USB_IF_DESCRIPTOR_TYPE   = 0x04;
    static const uint8_t  USB_EP_DESCRIPTOR_TYPE   = 0x05;
    static const uint8_t  USB_FUNC_DESCRIPTOR_TYPE = 0x24;

    static const int      USB_NO_ASSIGNED_ADDR     = -1;

    static const uint8_t  DEVICE_DESCRIPTOR_TYPE   = 0x01;
    static const uint8_t  CONFIG_DESCRIPTOR_TYPE   = 0x02;
    static const uint8_t  STRING_DESCRIPTOR_TYPE   = 0x03;
    static const uint8_t  IF_DESCRIPTOR_TYPE       = 0x04;
    static const uint8_t  EP_DESCRIPTOR_TYPE       = 0x05;
    static const uint8_t  CS_IF_DESCRIPTOR_TYPE    = 0x24;

    static const uint8_t  HEADER_SUBTYPE           = 0x00;
    static const uint8_t  CALL_MGMT_SUBTYPE        = 0x01;
    static const uint8_t  ACM_SUBTYPE              = 0x02;
    static const uint8_t  UNION_SUBTYPE            = 0x06;
    
    static const int      MAXSTRDESCSTRING         = 64;
    
    static const uint8_t  CONTROL_ADDR             = 0x00;
    static const uint8_t  CONTROL_EP               = 0x00;

// As these descriptor structures will used to form a single
// super-configuration structure, padding beyond bytes must be disabled
#pragma pack(push)
#pragma pack(1)
    struct deviceDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint16_t       bcdUSB;
        uint8_t        bDeviceClass;
        uint8_t        bDeviceSubClass;
        uint8_t        bDeviceProtocol;
        uint8_t        bMaxPacketSize;
        uint16_t       idVendor;
        uint16_t       idProduct;
        uint16_t       bcdDevice;
        uint8_t        iManufacturer;
        uint8_t        iProduct;
        uint8_t        iSerialNumber;
        uint8_t        bNumConfigurations;

        deviceDesc(uint8_t maxpktsize = 0x20, uint8_t numcfgs = 1)
        {
            bLength                    = 0x12;                         // 18 Bytes
            bDescriptorType            = DEVICE_DESCRIPTOR_TYPE;       // 0x01 = device descriptor
            bcdUSB                     = 0x1001;                       // USB 1.1
            bDeviceClass               = 0x02;                         // CDC
            bDeviceSubClass            = 0x00;                         // unused
            bDeviceProtocol            = 0x00;                         // unused
            bMaxPacketSize             = maxpktsize;                   // Max packet 32 bytes
            idVendor                   = 0xdead;                       // Fake vendor ID
            idProduct                  = 0x0001;                       // Product #1
            bcdDevice                  = 0x0001;                       // Release number
            iManufacturer              = 0x01;                         // Manufacturer string index
            iProduct                   = 0x02;                         // Product string index
            iSerialNumber              = 0x00;                         // Serial number index (none)
            bNumConfigurations         = numcfgs;                      // Number of configurations
        }
    } ;

    struct configDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint16_t       wTotalLength;
        uint8_t        bNumInterfaces;
        uint8_t        bConfigurationValue;
        uint8_t        iConfiguration;
        uint8_t        bmAttributes;
        uint8_t        bMaxPower;

        configDesc(uint16_t totalLen = 0x0039)
        {
            bLength             = 0x09;                                // 9 bytes
            bDescriptorType     = CONFIG_DESCRIPTOR_TYPE;              // 0x02 = configuration descriptor
            wTotalLength        = totalLen;                            // Number of bytes for all descriptors
            bNumInterfaces      = 0x02;                                // 2 interfaces
            bConfigurationValue = 0x01;                                // Index for this configuration
            iConfiguration      = 0x00;                                // Index to configuration description string (none)
            bmAttributes        = 0x80;                                // Powered from bus
            bMaxPower           = 0x64;                                // Maximum power consumption mA (100mA)
        }
    };

    struct interfaceDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint8_t        bInterfaceNumber;
        uint8_t        bAlternateSetting;
        uint8_t        bNumEndpoints;
        uint8_t        bInterfaceClass;
        uint8_t        bInterfaceSubClass;
        uint8_t        bInterfaceProtocol;
        uint8_t        iInterface;

        interfaceDesc(uint8_t index = 0, uint8_t numep = 1)
        {
            bLength             = 0x09;                                // 9 bytes
            bDescriptorType     = IF_DESCRIPTOR_TYPE;                  // 0x04 = interface descriptor
            bInterfaceNumber    = index;                               // Index of this interface descriptor
            bAlternateSetting   = 0x00;                                // Selection index for alternative setting
            bNumEndpoints       = numep;                               // Number of endpoints under interface
            bInterfaceClass     = 0x02;                                // Interface class (0x02 = CDC)
            bInterfaceSubClass  = 0x02;                                // Interface sub-class (0x02)
            bInterfaceProtocol  = 0x01;                                // Interface protocol (0x01)
            iInterface          = 0x00;                                // Index to interface description string (none)
        }
    };

    struct endpointDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint8_t        bEndpointAddress;
        uint8_t        bmAttributes;
        uint16_t       wMaxPacketSize;
        uint8_t        bInterval;

        endpointDesc(uint8_t epaddr = 0x01, uint8_t attr = 0x02, uint8_t interval = 0x00, uint16_t maxpktsize = 0x0020)
        {
            bLength             = 0x07;                                // 7 bytes
            bDescriptorType     = EP_DESCRIPTOR_TYPE;                  // 0x05 = endpoint descriptor
            bEndpointAddress    = epaddr;                              // Endpoint address (default 0x01, OUT)
            bmAttributes        = attr;                                // Default EP transfer attributes (0x02 = bulk)
            wMaxPacketSize      = maxpktsize;                          // Maximum packet size (32)
            bInterval           = interval;                            // Polling interval (default 0x00, unused for bulk)
        }
    };

    struct headerFuncDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint8_t        bDescriptorSubType;
        uint16_t       bcdCDC;

        headerFuncDesc()
        {
            bLength             = 0x05;                                // 4 Bytes
            bDescriptorType     = CS_IF_DESCRIPTOR_TYPE;               // CS_INTERFACE
            bDescriptorSubType  = HEADER_SUBTYPE;                      // Header
            bcdCDC              = 0x0110;                              // vesrion 1.1
        }
    };

    struct acmFuncDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint8_t        bDescriptorSubType;
        uint8_t        bmCapabilities;

        acmFuncDesc()
        {
            bLength             = 0x04;                                // 4 Bytes
            bDescriptorType     = CS_IF_DESCRIPTOR_TYPE;               // CS_INTERFACE
            bDescriptorSubType  = ACM_SUBTYPE;                         // Abstract control managment
            bmCapabilities      = 0x02;                                // ACM subset support
        }
    };

    struct unionFuncDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint8_t        bDescriptorSubType;
        uint8_t        bControlInterface;
        uint8_t        bSubordinateInterface0;

        unionFuncDesc()
        {
            bLength               = 0x05;                              // 4 Bytes
            bDescriptorType       = CS_IF_DESCRIPTOR_TYPE;             // CS_INTERFACE
            bDescriptorSubType    = UNION_SUBTYPE;                     // Union
            bControlInterface     = 0x00;                              // Control interface is interface 0
            bSubordinateInterface0 = 0x01;                             // Subordinate interface is interface 1
        }
    };

    struct callMgmtFuncDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint8_t        bDescriptorSubType;
        uint8_t        bmCapabilities;
        uint8_t        bmDataInterface;

        callMgmtFuncDesc()
        {
            bLength               = 0x05;                              // 4 Bytes
            bDescriptorType       = CS_IF_DESCRIPTOR_TYPE;             // CS_INTERFACE
            bDescriptorSubType    = CALL_MGMT_SUBTYPE;                 // Call management
            bmCapabilities        = 0x03;                              // DIY
            bmDataInterface       = 0x01;                              // Data interface is interface 1
        }
    };
    
    struct string0Desc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint16_t       wLangid[2];

        string0Desc()
        {
            bLength             = 0x08;                                // 6 Bytes
            bDescriptorType     = STRING_DESCRIPTOR_TYPE;              // 0x03 = string descriptor;
            wLangid[0]          = 0x0809;                              // Engish (UK)
            wLangid[1]          = 0x0409;                              // English (USA)
        }
    };

    struct stringDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint16_t       bString[MAXSTRDESCSTRING];

        stringDesc()
        {
            bLength             = 0x03;                                // 3 Bytes
            bDescriptorType     = STRING_DESCRIPTOR_TYPE;              // 0x03 = string descriptor;
            bString[0]          = 0x00;                                // Empty string
        }
    };
   
#pragma pack(pop)

    struct setupRequest
    {
        uint8_t        bmRequestType;
        uint8_t        bRequest;
        uint16_t       wValue;
        uint16_t       wIndex;
        uint16_t       wLength;
    };

    // Basic Differential line signal, as bytes
    typedef struct
    {
        uint8_t        dp;
        uint8_t        dm;
    } usb_signal_t;

    // Line speed types
    enum class usb_speed_e
    {
        LS,
        FS,
        HS
    };
}

#endif
