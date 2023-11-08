//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 31st October 2023
//
// Contains the code for the usbModel device endpoint
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
#include "usbDevice.h"

//-------------------------------------------------------------
// waitForExpectedPacket()
//
// Method to wait for the receipt of a particular PID packet
// type. A packet type of PID_INVALID will not check the type,
// otherwise a STALL acknowledge is sent if a mismatch on
// received PID. Will reset the device is reset detected on
// USB line. Will ignore bad packets.
//
//-------------------------------------------------------------

int usbDevice::waitForExpectedPacket(const int pktType, int &pid, uint32_t args[], uint8_t data[], int databytes, bool ignorebadpkts)
{
    int error = USBOK;
    int status;
    int numbits;

    USBDEVDEBUG ( "==> waitForExpectedPacket: waiting for a packet (0x%02x)\n", pid);

    while (true)
    {
        // Wait for a packet
        if ((status = waitForPkt(nrzi)) == USBRESET)
        {
            // If a reset seen, reset state and return
            reset();
            break;
        }
        else if (status == USBSUSPEND)
        {
            break;
        }

        if (decodePkt(nrzi, pid, args, data, databytes) != usbPkt::USBOK)
        {
            USBDEVDEBUG ( "==> waitForExpectedPacket: seen bad packet\n");
            // Ignore any packets that have errors
            if (ignorebadpkts)
            {
                continue;
            }
            else
            {
                USBERRMSG("waitForExpectedPacket: seen bad packet\n");
                return USBERROR;
            }
        }
        else
        {
            break;
        }
    }

    // Check packet is as expected
    if (pktType != PID_NO_CHECK && pid != pktType)
    {
        // Generate a STALL handshake for the error
        sendPktToHost(PID_HSHK_STALL);

        USBERRMSG("waitForExpectedPacket: Received unexpected pid (got 0x%02x, expected 0x%02x)\n", pid, pktType);

        error = USBERROR;
    }

    return error;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

// Data
void usbDevice::sendPktToHost(const int pid, const uint8_t data[], unsigned datalen)
{
    // Generate packet
    int numbits = genPkt(nrzi, pid, data, datalen);

    // Send over the USB line
    SendPacket(nrzi, numbits);
}

// Token
void usbDevice::sendPktToHost(const int pid, const uint8_t addr, uint8_t endp)
{
    // Generate packet
    int numbits = genPkt(nrzi, pid, addr, endp);

    // Send over the USB line
    SendPacket(nrzi, numbits);
}

// SOF
void usbDevice::sendPktToHost(const int pid, const uint16_t framenum)
{
    // Generate packet
    int numbits = genPkt(nrzi, pid, framenum);

    // Send over the USB line
    SendPacket(nrzi, numbits);
}

// Handshake
void usbDevice::sendPktToHost(const int pid)
{
    // Generate packet
    int numbits = genPkt(nrzi, pid);

    // Send over the USB line
    SendPacket(nrzi, numbits);
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int usbDevice::runUsbDevice()
{
    int                  error = 0;
    int                  pid;
    uint32_t             args[4];
    uint8_t              data[4096];
    int                  databytes;
    usbPkt::usb_signal_t nrzi[usbPkt::MAXBUFSIZE];

    // Wait for reset deassertion
    waitOnNotReset();

    // Loop forever
    while (true)
    {
        if (waitForExpectedPacket(PID_INVALID, pid, args, rxdata, databytes) != USBOK)
        {
            return USBERROR;
        }

        // Process initiating packet types
        switch(pid)
        {
        case PID_TOKEN_SETUP:
            if (processControl(args[ARGADDRIDX], args[ARGENDPIDX]) != USBOK)
            {
                return USBERROR;
            }
            break;
        case PID_TOKEN_IN:
            processIn(args, data, databytes);
            break;
        case PID_TOKEN_OUT:
            processOut(args, data, databytes);
            break;
        case PID_TOKEN_SOF:
            processSOF(args);
            break;
        default:
            USBERRMSG("runUsbDevice: Received unexpected packet ID (0x%x)\n", pid);
            return USBERROR;
            break;
        }
    }

    return usbPkt::USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int usbDevice::processControl(const uint32_t addr, const uint32_t endp)
{
    int                  numbits;
    int                  pid;
    uint32_t             args[4];
    int                  databytes;

    USBDEVDEBUG ( "==> processControl (addr = 0x%02x, endp = 0x%02x)\n", addr, endp);

    // Check Address/endp is 0/0 or a previously set address and a valid endpoint
    if (!((addr == 0 && endp == 0) || (addr == devaddr && endp > 0 && endp <= numendpoints)))
    {
        // Generate a STALL handshake if an error
        sendPktToHost(PID_HSHK_STALL);

        USBERRMSG("processControl: Received bad addr/endp (0x%02x 0x%02x)\n", addr, endp);
        return USBERROR;
    }

    // Loop until seen a valid packet or until reset
    while (true)
    {
        // Wait for DATA0 packet
        USBDEVDEBUG ( "Waiting for DATA0\n");
        if (waitForExpectedPacket(PID_DATA_0, pid, args, rxdata, databytes) != USBOK)
        {
            return USBERROR;
        }

        USBDEVDEBUG ( "Send ACK\n");

        // Generate an ACK handshake for the DATA0 packet
        sendPktToHost(PID_HSHK_ACK);

        // Map received data over the expected request type
        setupRequest* sreq = (setupRequest*)rxdata;

        USBDEVDEBUG ( "==> received device request (0x%x)\n", sreq->bmRequestType);

        // Decode request (device, interface, endpoint)
        switch(sreq->bmRequestType)
        {
        case USB_DEV_REQTYPE_SET:
        case USB_DEV_REQTYPE_GET:
            return handleDevReq(sreq);
            break;
        case USB_IF_REQTYPE_SET:
        case USB_IF_REQTYPE_GET:
            break;
        case USB_EP_REQTYPE_SET:
        case USB_EP_REQTYPE_GET:
            break;
        default:
            // Generate a STALL handshake if an unknown bmRequestType
            sendPktToHost(PID_HSHK_STALL);
        }
    }

    return USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int usbDevice::handleDevReq(const setupRequest* sreq)
{
    int                  numbits;
    int                  pid;
    uint32_t             args[4];
    int                  databytes;

    USBDEVDEBUG ( "==> handleDevReq (0x%x 0x%04x)\n", sreq->bRequest, sreq->wLength);


    switch(sreq->bRequest)
    {
    case USB_REQ_GET_STATUS:

        // Check request type is a "device get" type
        if (sreq->bmRequestType != USB_DEV_REQTYPE_GET)
        {
            USBERRMSG("handleDevReq: Received unexpected bmRequestType with GET_STATUS (0x%02x)\n", sreq->bmRequestType);
            return USBERROR;
        }

        // Check get status length is always 2
        if (sreq->wLength != 2)
        {
            USBERRMSG("handleDevReq: Received unexpected GET_STATUS length (got 0x%02x, expected 0x%02x)\n", sreq->wLength, 2);
            return USBERROR;
        }

        // Construct status data
        uint8_t buf[2];
        buf[0] = remoteWakeup | selfPowered;
        buf[1] = 0;

        USBDEVDEBUG ( "==> handleDevReq: waiting for IN token\n");

        if (waitForExpectedPacket(PID_TOKEN_IN, pid, args, rxdata, databytes) != USBOK)
        {
            return USBERROR;
        }
        else
        {
            USBDEVDEBUG ( "Seen GET_STATUS request\n");

            while (true)
            {
                // Send DATA1 packet
                sendPktToHost(PID_DATA_1, buf, 2);

                USBDISPPKT("  %s RX DEV REQ: GET STATUS\n    " FMT_DATA_GREY "remWkup=%d selfPwd=%d" FMT_NORMAL "\n",
                    name.c_str(), remoteWakeup ? 1 : 0, selfPowered ? 1 : 0);

                // Wait for acknowledge (either ACK or NAK)
                if (waitForExpectedPacket(PID_NO_CHECK, pid, args, rxdata, databytes) != USBOK)
                {
                    return USBERROR;
                }

                // If ACK then end of transaction
                if (pid == PID_HSHK_ACK)
                {
                    USBDEVDEBUG("==> handleDevReq: seen ACK for DATA1\n");
                    break;
                }
                // Unexpected PID in not NAK. NAK cause loop to send again, so no action.
                else if (pid != PID_HSHK_NAK)
                {
                    return USBERROR;
                }
            }
        }
        break;

    case USB_REQ_CLEAR_FEATURE:
        break;
    case USB_REQ_SET_FEATURE:
        break;
    case USB_REQ_SET_ADDRESS:
        break;
    case USB_REQ_GET_DESCRIPTOR:
        break;
    case USB_REQ_SET_DESCRIPTOR:
        break;
    case USB_REQ_GET_CONFIG:
        break;
    case USB_REQ_SET_CONFIG:
       break;
    default:
        // Generate a STALL handshake if an unknown bRequest
        sendPktToHost(PID_HSHK_STALL);
        break;
    }

    return USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int  usbDevice::processIn  (const uint32_t args[], const uint8_t  data[], const int databytes)
{
    return USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int  usbDevice::processOut (const uint32_t args[], uint8_t data[], const int databytes)
{
    return USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int  usbDevice::processSOF(const uint32_t args[])
{
    return USBOK;
}
