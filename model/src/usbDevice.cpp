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
    int error = usbModel::USBOK;
    int status;
    int numbits;

    USBDEVDEBUG ( "==> waitForExpectedPacket: waiting for a packet (0x%02x)\n", pid);

    while (true)
    {
        // Wait for a packet
        if ((status = waitForPkt(nrzi)) == usbModel::USBRESET)
        {
            // If a reset seen, reset state and return
            reset();
            break;
        }
        else if (status == usbModel::USBSUSPEND)
        {
            break;
        }

        if (decodePkt(nrzi, pid, args, data, databytes) != usbModel::USBOK)
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
                return usbModel::USBERROR;
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
        sendPktToHost(usbModel::PID_HSHK_STALL);

        USBERRMSG("waitForExpectedPacket: Received unexpected pid (got 0x%02x, expected 0x%02x)\n", pid, pktType);

        error = usbModel::USBERROR;
    }

    return error;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

// Data
void usbDevice::sendPktToHost(const int pid, const uint8_t data[], unsigned datalen, const int idle)
{
    // Generate packet
    int numbits = genUsbPkt(nrzi, pid, data, datalen);

    // Send over the USB line
    SendPacket(nrzi, numbits, idle);
}

// Token
void usbDevice::sendPktToHost(const int pid, const uint8_t addr, uint8_t endp, const int idle)
{
    // Generate packet
    int numbits = genUsbPkt(nrzi, pid, addr, endp);

    // Send over the USB line
    SendPacket(nrzi, numbits, idle);
}

// SOF
void usbDevice::sendPktToHost(const int pid, const uint16_t framenum, const int idle)
{
    // Generate packet
    int numbits = genUsbPkt(nrzi, pid, framenum);

    // Send over the USB line
    SendPacket(nrzi, numbits, idle);
}

// Handshake
void usbDevice::sendPktToHost(const int pid, const int idle)
{
    // Generate packet
    int numbits = genUsbPkt(nrzi, pid);

    // Send over the USB line
    SendPacket(nrzi, numbits, idle);
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int usbDevice::runUsbDevice(const int idle)
{
    int                  pid;
    uint32_t             args[4];
    int                  databytes;

    // Wait for reset deassertion
    waitOnNotReset();

    // Loop forever
    while (true)
    {
        if (waitForExpectedPacket(usbModel::PID_INVALID, pid, args, rxdata, databytes) != usbModel::USBOK)
        {
            return usbModel::USBERROR;
        }

        // Process initiating packet types
        switch(pid)
        {
        case usbModel::PID_TOKEN_SETUP:
            if (processControl(args[usbModel::ARGADDRIDX], args[usbModel::ARGENDPIDX], idle) != usbModel::USBOK)
            {
                return usbModel::USBERROR;
            }
            break;
        case usbModel::PID_TOKEN_IN:
            processIn(args, rxdata, databytes, idle);
            break;
        case usbModel::PID_TOKEN_OUT:
            processOut(args, rxdata, databytes, idle);
            break;
        case usbModel::PID_TOKEN_SOF:
            processSOF(args, idle);
            break;
        default:
            USBERRMSG("runUsbDevice: Received unexpected packet ID (0x%x)\n", pid);
            return usbModel::USBERROR;
            break;
        }
    }

    return usbModel::USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int usbDevice::processControl(const uint32_t addr, const uint32_t endp, const int idle)
{
    int                  pid;
    uint32_t             args[4];
    int                  databytes;

    USBDEVDEBUG ( "==> processControl (addr = 0x%02x, endp = 0x%02x)\n", addr, endp);

    // Check Address/endp is 0/0 or a previously set address and a valid endpoint
    if (!((addr == 0 && endp == 0) || (addr == devaddr && endp > 0 && endp <= numendpoints)))
    {
        // Generate a STALL handshake if an error
        sendPktToHost(usbModel::PID_HSHK_STALL, idle);

        USBERRMSG("processControl: Received bad addr/endp (0x%02x 0x%02x)\n", addr, endp);
        return usbModel::USBERROR;
    }

    // Loop until seen a valid packet or until reset
    while (true)
    {
        // Wait for DATA0 packet
        USBDEVDEBUG ( "Waiting for DATA0\n");
        if (waitForExpectedPacket(usbModel::PID_DATA_0, pid, args, rxdata, databytes) != usbModel::USBOK)
        {
            return usbModel::USBERROR;
        }

        USBDEVDEBUG ( "Send ACK\n");

        // Generate an ACK handshake for the DATA0 packet
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);

        // Map received data over the expected request type
        usbModel::setupRequest* sreq = (usbModel::setupRequest*)rxdata;

        USBDEVDEBUG ( "==> received device request (0x%x)\n", sreq->bmRequestType);

        // Decode request (device, interface, endpoint)
        switch(sreq->bmRequestType)
        {
        case usbModel::USB_DEV_REQTYPE_SET:
        case usbModel::USB_DEV_REQTYPE_GET:
            return handleDevReq(sreq, idle);
            break;
        case usbModel::USB_IF_REQTYPE_SET:
        case usbModel::USB_IF_REQTYPE_GET:
            break;
        case usbModel::USB_EP_REQTYPE_SET:
        case usbModel::USB_EP_REQTYPE_GET:
            break;
        default:
            // Generate a STALL handshake if an unknown bmRequestType
            sendPktToHost(usbModel::PID_HSHK_STALL, idle);
        }
    }

    return usbModel::USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int usbDevice::handleDevReq(const usbModel::setupRequest* sreq, const int idle)
{
    int                  pid;
    uint32_t             args[4];
    int                  databytes;
    int                  datasize;

   // USBDEVDEBUG ( "==> handleDevReq (0x%x 0x%04x)\n", sreq->bRequest, sreq->wLength);

    switch(sreq->bRequest)
    {
    case usbModel::USB_REQ_GET_STATUS:

        // Check request type is a "device get" type
        if (sreq->bmRequestType != usbModel::USB_DEV_REQTYPE_GET)
        {
            USBERRMSG("handleDevReq: Received unexpected bmRequestType with GET_STATUS (0x%02x)\n", sreq->bmRequestType);
            return usbModel::USBERROR;
        }

        // Check get status length is always 2
        if (sreq->wLength != 2)
        {
            USBERRMSG("handleDevReq: Received unexpected GET_STATUS length (got 0x%02x, expected 0x%02x)\n", sreq->wLength, 2);
            return usbModel::USBERROR;
        }

        // Construct status data
        uint8_t buf[2];
        buf[0]   = remoteWakeup | selfPowered;
        buf[1]   = 0;
        datasize = 2;

        USBDEVDEBUG ("==> handleDevReq: waiting for IN token\n");

        if (waitForExpectedPacket(usbModel::PID_TOKEN_IN, pid, args, rxdata, databytes) != usbModel::USBOK)
        {
            return usbModel::USBERROR;
        }
        else
        {
            USBDEVDEBUG ( "Seen GET_STATUS request\n");

            while (true)
            {
                // Send DATA1 packet
                sendPktToHost(usbModel::PID_DATA_1, buf, datasize, idle);

                USBDISPPKT("  %s RX DEV REQ: GET STATUS\n    " FMT_DATA_GREY "remWkup=%d selfPwd=%d" FMT_NORMAL "\n",
                    name.c_str(), remoteWakeup ? 1 : 0, selfPowered ? 1 : 0);

                // Wait for acknowledge (either ACK or NAK)
                if (waitForExpectedPacket(PID_NO_CHECK, pid, args, rxdata, databytes) != usbModel::USBOK)
                {
                    return usbModel::USBERROR;
                }

                // If ACK then end of transaction
                if (pid == usbModel::PID_HSHK_ACK)
                {
                    USBDEVDEBUG("==> handleDevReq: seen ACK for DATA1\n");
                    break;
                }
                // Unexpected PID in not NAK. NAK cause loop to send again, so no action.
                else if (pid != usbModel::PID_HSHK_NAK)
                {
                    return usbModel::USBERROR;
                }
            }
        }
        break;

    case usbModel::USB_REQ_CLEAR_FEATURE:
        break;
    case usbModel::USB_REQ_SET_FEATURE:
        break;
    case usbModel::USB_REQ_SET_ADDRESS:
        break;
    case usbModel::USB_REQ_GET_DESCRIPTOR:
        break;
    case usbModel::USB_REQ_SET_DESCRIPTOR:
        break;
    case usbModel::USB_REQ_GET_CONFIG:
        break;
    case usbModel::USB_REQ_SET_CONFIG:
       break;
    default:
        // Generate a STALL handshake if an unknown bRequest
        sendPktToHost(usbModel::PID_HSHK_STALL, idle);
        break;
    }

    return usbModel::USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int  usbDevice::processIn  (const uint32_t args[], const uint8_t  data[], const int databytes, const int idle)
{
    return usbModel::USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int  usbDevice::processOut (const uint32_t args[], uint8_t data[], const int databytes, const int idle)
{
    return usbModel::USBOK;
}

//-------------------------------------------------------------
//-------------------------------------------------------------

int  usbDevice::processSOF(const uint32_t args[], const int idle)
{
    return usbModel::USBOK;
}
