//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 31st October 2023
//
// Contains the headers for the usbModel device endpoint
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


#include <cstring>
#include "usbPliApi.h"

class usbDevice : public usbPliApi
{
public:

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

    static const uint8_t  USB_REMOTE_WAKUP_OFF     = 0x0000;
    static const uint8_t  USB_REMOTE_WAKUP_ON      = 0x0002;

    static const uint8_t  USB_NOT_SELF_POWERED     = 0x0000;
    static const uint8_t  USB_SELF_POWERED         = 0x0001;

    static const int      USB_NO_ASSIGNED_ADDR     = -1;

    static const int      PID_NO_CHECK             = PID_INVALID;

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

        deviceDesc()
        {
            bLength                    = 0x12;         // 18 Bytes
            bDescriptorType            = 0x01;         // 0x01 = device descriptor
            bcdUSB                     = 0x1001;       // USB 1.1
            bDeviceClass               = 0x02;         // CDC
            bDeviceSubClass            = 0x00;         // unused
            bDeviceProtocol            = 0x00;         // unused
            bMaxPacketSize             = 0x20;         // Max packet 32 bytes
            idVendor                   = 0xdead;       // Fake vendor ID
            idProduct                  = 0x0001;       // Product #1
            bcdDevice                  = 0x0001;       // Release number
            iManufacturer              = 0x01;         // Manufacturer string index
            iProduct                   = 0x02;         // Product string index
            iSerialNumber              = 0x00;         // Serial number index (none)
            bNumConfigurations         = 0x01;         // Number of configurations
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

        configDesc()
        {
            bLength             = 0x09;          // 9 bytes
            bDescriptorType     = 0x02;          // 0x02 = configuration descriptor
            wTotalLength        = 0x39;          // Number of bytes for all descriptors
            bNumInterfaces      = 0x02;          // 2 interfaces
            bConfigurationValue = 0x01;          // Index for this configuration
            iConfiguration      = 0x00;          // Index to configuration description string (none)
            bmAttributes        = 0x80;          // Powered from bus
            bMaxPower           = 0x64;          // Maximum power consumption mA (100mA)
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

        interfaceDesc()
        {
            bLength            = 0x09;          // 9 bytes
            bDescriptorType    = 0x04;          // 0x04 = interface descriptor
            bInterfaceNumber   = 0x00;          // Index of this interface descriptor
            bAlternateSetting  = 0x00;          // Selection index for alternative setting
            bNumEndpoints      = 0x01;          // Number of endpoints under interface (1)
            bInterfaceClass    = 0x02;          // Interface class (0x02 = CDC)
            bInterfaceSubClass = 0x02;          // Interface sub-class (0x02)
            bInterfaceProtocol = 0x01;          // Interface protocol (0x01)
            iInterface         = 0x00;          // Index to interface description string (none)
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

        endpointDesc()
        {
            bLength          = 0x07;          // 7 bytes
            bDescriptorType  = 0x05;          // 0x05 = endpoint descriptor
            bEndpointAddress = 0x01;          // Endpoint address (0x01, OUT)
            bmAttributes     = 0x02;          // EP transfer attributes (0x02 = bulk)
            wMaxPacketSize   = 0x0020;        // Maximum packet size (32)
            bInterval        = 0x00;          // Polling interval (0x00, unused for bulk)
        }
    };

    struct stringDesc
    {
        uint8_t        bLength;
        uint8_t        bDescriptorType;
        uint16_t       bString[64];

        stringDesc()
        {
            bLength                    = 0x03;          // 3 Bytes
            bDescriptorType            = 0x03;          // 0x03 = string descriptor;
            bString[0]                 = 0x00;          // Empty string
        }
    };

    struct setupRequest
    {
        uint8_t        bmRequestType;
        uint8_t        bRequest;
        uint16_t       wValue;
        uint16_t       wIndex;
        uint16_t       wLength;
    };

    // Constructor
    usbDevice (int nodeIn, std::string name = std::string(FMT_DEVICE "DEV " FMT_NORMAL)) :
        usbPliApi(nodeIn, name),
        numendpoints(1),
        remoteWakeup(USB_REMOTE_WAKUP_OFF),
        selfPowered(USB_NOT_SELF_POWERED),
        timeSinceLastSof(0)
    {
        reset();
    };

    // User entry method to start the USB device model
    int runUsbDevice();

private:

    // Reset method, called on detecting a reset state on the line
    void reset()
    {
        usbPliApi::reset();
        devaddr   = USB_NO_ASSIGNED_ADDR;
    }

    // Methods for sending packets back towards the host
    void         sendPktToHost         (const int pid, const uint8_t  data[], unsigned datalen); // DATA
    void         sendPktToHost         (const int pid, const uint8_t  addr,   uint8_t  endp);    // Token
    void         sendPktToHost         (const int pid, const uint16_t framenum);                 // SOF
    void         sendPktToHost         (const int pid);                                          // Handshake

    // Method to wait for the receipt of a particular packet type
    int          waitForExpectedPacket (const int pktType, int &pid, uint32_t args[], uint8_t data[], int databytes, bool ignorebadpkts = true);

    // Methods for processing different packet types
    int          processControl        (const uint32_t addr,   const uint32_t endp);
    int          processIn             (const uint32_t args[], const uint8_t  data[], const int databytes);
    int          processOut            (const uint32_t args[],       uint8_t  data[], const int databytes);
    int          processSOF            (const uint32_t args[]);

    // Method for handling device requests
    int          handleDevReq          (const setupRequest* sreq);

    // Internal device state
    int          devaddr;
    uint8_t      numendpoints;
    uint8_t      remoteWakeup;
    uint8_t      selfPowered;

    int          timeSinceLastSof;

    // Internal buffers for use by class methods
    uint8_t      rxdata [usbPkt::MAXBUFSIZE];
    usb_signal_t nrzi   [usbPkt::MAXBUFSIZE];
    char         sbuf   [usbPkt::ERRBUFSIZE];

};