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

    USBDEVDEBUG ( "==> waitForExpectedPacket: waiting for a packet (0x%02x)\n", pktType);

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
            USBDEVDEBUG ( "==> waitForExpectedPacket: seen bad packet\n%s    ", errbuf);
            for(int i = 0; i < databytes; i++)
                USBDEVDEBUG("%02x ", data[i]);
            USBDEVDEBUG("\n");

            // Ignore any packets that have errors
            if (ignorebadpkts)
            {
                continue;
            }
            else
            {
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
    if (!((addr == 0 && endp == 0) || (addr == devaddr && endp <= numendpoints)))
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
            USBDEVDEBUG("%s", errbuf);
            return usbModel::USBERROR;
        }

        USBDEVDEBUG ( "==> Send ACK\n");

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
    int                  index;

    uint8_t              desctype;
    uint8_t              descidx;

    USBDEVDEBUG ( "==> handleDevReq (bRequest=0x%x wValue=0x%04x wLength=0x%04x)\n", sreq->bRequest, sreq->wValue, sreq->wLength);

    //return usbModel::USBOK;

    switch(sreq->bRequest)
    {
    case usbModel::USB_REQ_GET_STATUS:

        // Construct status data
        uint8_t buf[2];
        buf[0]   = remoteWakeup | selfPowered;
        buf[1]   = 0;
        datasize = 2;

        // Construct a formatted output string
        snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET STATUS\n    " FMT_DATA_GREY "remWkup=%d selfPwd=%d" FMT_NORMAL "\n",
                 name.c_str(),
                 remoteWakeup ? 1 : 0,
                 selfPowered  ? 1 : 0);

        // Send the response to the GET_STATUS command
        return sendGetResp (sreq, buf, datasize, sbuf);
        break;

    case usbModel::USB_REQ_CLEAR_FEATURE:
        break;

    case usbModel::USB_REQ_SET_FEATURE:
        break;

    case usbModel::USB_REQ_SET_ADDRESS:
        // Extract address
        devaddr = sreq->wValue;

        USBDEVDEBUG("==> Received SET_ADDRESS 0x%02x\n", sreq->wValue);
        break;

    case usbModel::USB_REQ_GET_DESCRIPTOR:

        // Extract the descriptor type (upper bytes) and index (lower byte)
        // from the wValue field.
        desctype = sreq->wValue >> 8;
        descidx  = sreq->wValue & 0xff;

        // Select on the requested descriptor type
        switch(desctype)
        {
        case usbModel::DEVICE_DESCRIPTOR_TYPE:

            // Set returned data size to be the whole of the descriptor if this is smaller than requested length,
            // else return the requested length
            datasize = (sreq->wLength > sizeof(usbModel::deviceDesc)) ? sizeof(usbModel::deviceDesc) : sreq->wLength;

            // Construct a formatted output string
            snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET DEVICE DESCRIPTOR (wLength = %d)\n", name.c_str(), sreq->wLength);

            // Send the response to the GET_DESCRIPTOR (DEVICE) command
            return sendGetResp(sreq, (uint8_t*)&devdesc, datasize, sbuf);
            break;

        case usbModel::CONFIG_DESCRIPTOR_TYPE:

            // Set returned data size to be the whole of all of the descriptors if this is smaller than requested length,
            // else return the requested length
            datasize = (sreq->wLength > sizeof(configAllDesc)) ? sizeof(configAllDesc) : sreq->wLength;

            // Construct a formatted output string
            snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET CONFIG DESCRIPTOR (wLength = %d)\n", name.c_str(), sreq->wLength);

            // Send the response to the GET_DESCRIPTOR (CONFIG) command
            return sendGetResp(sreq, cfgalldesc.rawbytes, datasize, sbuf);
            break;

        case usbModel::STRING_DESCRIPTOR_TYPE:
            // Set returned data size to be the whole of all of the descriptors if this is smaller than requested length,
            // else return the requested length
            datasize = (sreq->wLength > strdesc[descidx].bLength) ? strdesc[descidx].bLength : sreq->wLength;

            // Construct a formatted output string
            snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET STRING DESCRIPTOR (wLength = %d)\n", name.c_str(), sreq->wLength);

            // Send the response to the GET_DESCRIPTOR (STRING) command
            return sendGetResp(sreq, (uint8_t*)&strdesc[descidx], datasize, sbuf);
            break;

        case usbModel::IF_DESCRIPTOR_TYPE:
            break;

        case usbModel::EP_DESCRIPTOR_TYPE:
            break;

        case usbModel::CS_IF_DESCRIPTOR_TYPE:
            break;

        default:
            USBERRMSG("handleDevReq: Received unexpected wValue descriptor type (0x%02x)\n", sreq->wValue);
            return usbModel::USBERROR;
            break;
        }

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

int usbDevice::sendGetResp (const usbModel::setupRequest* sreq, const uint8_t data[], const int databytes, const char* fmtstr, const int idle)
{
    int                  pid;
    uint32_t             args[4];
    int                  datasize;
    int                  numbytes;

    int datasent       = 0;
    int currdatapid    = usbModel::PID_DATA_1;

    USBDEVDEBUG("==> sendGetResp: databytes=%d\n", databytes);

    // Check request type is a "device get" type
    if (sreq->bmRequestType != usbModel::USB_DEV_REQTYPE_GET)
    {
        USBERRMSG("getResp: Received unexpected bmRequestType with a GET command (0x%02x)\n", sreq->bmRequestType);
        return usbModel::USBERROR;
    }

    while (true)
    {
        int remaining_data = databytes - datasent;

        if (remaining_data > devdesc.bMaxPacketSize)
        {
            datasize  = devdesc.bMaxPacketSize;
        }
        else
        {
            if (remaining_data > 0)
            {
                datasize  = remaining_data;
            }
            else
            {
                if (remaining_data == 0)
                {
                    break;
                }
            }
        }

        USBDEVDEBUG ("==> sendGetResp: waiting for IN token\n");

        // Wait for an IN token
        if (waitForExpectedPacket(usbModel::PID_TOKEN_IN, pid, args, rxdata, numbytes) != usbModel::USBOK)
        {
            USBERRMSG ("getResp: Received unexpected bmRequestType with a GET command (0x%02x)\n", sreq->bmRequestType);
            return usbModel::USBERROR;
        }

        // Send DATAx packet
        sendPktToHost(currdatapid, &data[datasent], datasize, idle);
        datasent += datasize;

        currdatapid = (currdatapid == usbModel::PID_DATA_0) ? usbModel::PID_DATA_1 : usbModel::PID_DATA_0;

        USBDISPPKT("%s", fmtstr);

        USBDEVDEBUG ("==> sendGetResp: waiting for ACK/NAK token\n");

        // Wait for acknowledge (either ACK or NAK)
        if (waitForExpectedPacket(PID_NO_CHECK, pid, args, rxdata, databytes) != usbModel::USBOK)
        {
            USBERRMSG ("getResp: unexpected error wait for ACK/NAK\n");
            return usbModel::USBERROR;
        }

        // If ACK then end of transaction if no more bytes
        if (pid == usbModel::PID_HSHK_ACK)
        {
            USBDEVDEBUG("==> getResp: seen ACK for DATAx\n");
            if ((databytes - datasent) == 0)
            {
                USBDEVDEBUG("==> getResp: remaining_data = %s\n", databytes - datasent);
                break;
            }
        }
        // Unexpected PID if not a NAK. NAK causes loop to send again, so no action.
        else if (pid != usbModel::PID_HSHK_NAK)
        {
            return usbModel::USBERROR;
        }
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
