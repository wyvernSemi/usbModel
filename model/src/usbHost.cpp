//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 9th Novenmber 2023
//
// Contains the method definitions for the usbModel host class
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

#include "usbHost.h"

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
int usbHost::getDeviceStatus (const uint8_t addr, const uint8_t endp, uint16_t &status, const unsigned idle)
{
    int error = usbModel::USBOK;
    int databytes;

    // Send out the request
    if (sendGetStatusRequest(addr, endp, idle) != usbModel::USBOK)
    {
        error = usbModel::USBERROR;
    }
    else
    {
        // Send IN
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);

        // Receive requested data
        if (getDataFromDevice(usbModel::PID_DATA_1, rxdata, databytes, idle) != usbModel::USBOK)
        {
            error = usbModel::USBERROR;
        }
        else
        {
            status = (uint16_t)rxdata[0] | (((uint16_t)rxdata[1]) << 8);
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::getDeviceDescriptor (const uint8_t  addr,   const uint8_t  endp,
                                        uint8_t  data[], const uint16_t reqlen, uint16_t &rxlen,
                                  const bool     chklen, const unsigned idle)
{
    int error         = usbModel::USBOK;
    int receivedbytes = 0;
    int databytes;
    
    
    // Send out the request
    if (sendGetDevDescRequest(addr, endp, idle) != usbModel::USBOK)
    {
        error = usbModel::USBERROR;
    }
    else
    {
        do
        {
            // Send IN token
            sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);
            
            // Receive requested data
            if (getDataFromDevice(usbModel::PID_DATA_1, &rxdata[receivedbytes], databytes, idle) != usbModel::USBOK)
            {
                error = usbModel::USBERROR;
            }
            else
            {
                receivedbytes += databytes;
            }
        } while (receivedbytes < reqlen);
        
        if (chklen && receivedbytes != reqlen)
        {
            USBERRMSG("getDeviceDescriptor: unexpected length of data received (got %d, expected %d)\n", receivedbytes, reqlen);
            error = usbModel::USBERROR;
        }
        else
        {
            std::memcpy(data, rxdata, reqlen);
            rxlen = receivedbytes;
        }
    }
    
    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

void usbHost::sendTokenToDevice (const int pid, const uint8_t addr, const uint8_t endp, const unsigned idle)
{
    int numbits = genUsbPkt(nrzi, pid, addr, endp);

    SendPacket(nrzi, numbits, idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::sendDataToDevice (const int datatype, const uint8_t data[], const int len, const unsigned idle)
{
    int error = usbModel::USBOK;

    if (datatype != usbModel::PID_DATA_0 && datatype != usbModel::PID_DATA_1)
    {
        error = usbModel::USBERROR;
    }
    else
    {
        int numbits = genUsbPkt(nrzi, datatype, data, len);

        SendPacket(nrzi, numbits, idle);
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::getDataFromDevice(const int expPID, uint8_t data[], int &databytes, const unsigned idle)
{
    int                  error = usbModel::USBOK;
    int                  pid;
    uint32_t             args[4];

    // Wait for data
    waitForPkt(nrzi);

    if (decodePkt(nrzi, pid, args, data, databytes) != usbModel::USBOK)
    {
        USBDEVDEBUG ("***ERROR: getInData: received bad packet waiting for data\n");
        getUsbErrMsg(sbuf);
        USBDEVDEBUG ("%s\n", sbuf);
        error = usbModel::USBERROR;
    }
    else
    {
        if (pid == expPID)
        {
            // Send ACK
            int numbits = genUsbPkt(nrzi, usbModel::PID_HSHK_ACK);
            SendPacket(nrzi, numbits, idle);
        }
        else
        {
            USBDEVDEBUG ("***ERROR: getInData: received unexpected packet ID waiting for data (0x%02x)\n", pid);
            error = usbModel::USBERROR;
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
int usbHost::sendDeviceRequest(const uint8_t  addr,    const uint8_t  endp,
                               const uint8_t  request, const uint16_t length, const uint16_t value, const uint16_t index,
                               const unsigned idle)
{
    int                  error = usbModel::USBOK;

    int                  pid;
    int                  databytes;
    uint32_t             args[4];

    // SETUP
    sendTokenToDevice(usbModel::PID_TOKEN_SETUP, addr, endp, idle);

    // DATA0 (device get request)
    usbModel::setupRequest setup;
    setup.bmRequestType = usbModel::USB_DEV_REQTYPE_GET;
    setup.bRequest      = request;
    setup.wValue        = value;
    setup.wIndex        = index;
    setup.wLength       = length;

    sendDataToDevice(usbModel::PID_DATA_0, (uint8_t*)&setup, sizeof(usbModel::setupRequest), idle);

    do
    {
        // Wait for ACK
        waitForPkt(nrzi);

        if (decodePkt(nrzi, pid, args, rxdata, databytes) != usbModel::USBOK)
        {
            USBDEVDEBUG("***ERROR: sendGetStatusRequest: received bad packet waiting for ACK\n");
            getUsbErrMsg(sbuf);
            USBDEVDEBUG("%s\n", sbuf);
            error = usbModel::USBERROR;
            break;
        }

        if (pid != usbModel::PID_HSHK_ACK && pid != usbModel::PID_HSHK_NAK)
        {
            USBDEVDEBUG("***ERROR: sendGetStatusRequest: received unexpected packet ID (0x%02x)\n", pid);
            break;
        }

    } while (pid == usbModel::PID_HSHK_NAK);

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::sendGetStatusRequest(const uint8_t addr, const uint8_t endp, const unsigned idle)
{
    return sendDeviceRequest(addr, endp, usbModel::USB_REQ_GET_STATUS, 2);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::sendGetDevDescRequest(const uint8_t addr, const uint8_t endp, const uint16_t length, const unsigned idle)
{
    return sendDeviceRequest(addr, endp, usbModel::USB_REQ_GET_DESCRIPTOR, length);
}