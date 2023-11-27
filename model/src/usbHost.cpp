//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 9th November 2023
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

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Public method definitions
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostWaitForConnection (const unsigned polldelay, const unsigned timeout)
{
    int      linestate;
    unsigned clkcycles  = 0;

    // Make sure reset is deasserted before checking for a connection
    apiWaitOnNotReset();

    while ((linestate = apiReadLineState()) == usbModel::USB_SE0 && clkcycles < timeout)
    {
        apiSendIdle(polldelay);
        clkcycles += polldelay;
    }

    if (clkcycles >= timeout)
    {
        USBERRMSG("waitForConnection: timed out waiting for a device to be connected");
        linestate = usbModel::USBERROR;
    }
    else
    {
        USBDISPPKT("  %s USB DEVICE CONNECTED (at cycle %d)\n", name.c_str(), apiGetClkCount());
        connected = true;
    }

    return linestate;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostGetDeviceStatus (const uint8_t addr, const uint8_t endp, uint16_t &status, const unsigned idle)
{

    return getStatus(addr, endp, usbModel::USB_DEV_REQTYPE_GET, status, idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostGetDeviceConfig (const uint8_t addr, const uint8_t endp, uint8_t &cfgstate, const uint8_t index, const unsigned idle)
{
    int error = usbModel::USBOK;
    int databytes;

    // Send out the request
    if (sendStandardRequest(addr, endp,
                            usbModel::USB_DEV_REQTYPE_GET,
                            usbModel::USB_REQ_GET_CONFIG,
                            (uint16_t)index,                          // wValue
                            0,                                        // wIndex
                            1,                                        // wLength
                            idle) != usbModel::USBOK)
    {
        error = usbModel::USBERROR;
    }
    else
    {
        // Send IN
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);

        // Receive requested data
        if (getDataFromDevice(dataPid(endp), rxdata, databytes, idle) != usbModel::USBOK)
        {
            error = usbModel::USBERROR;
        }
        else
        {
            dataPidUpdate(endp);
            cfgstate = rxdata[0];
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostGetStrDescriptor (const uint8_t  addr,     const uint8_t  endp,
                                      const uint8_t  strindex, uint8_t  data[],
                                      const uint16_t reqlen,   uint16_t       &rxlen,
                                      const bool     chklen,
                                      const uint16_t langid,
                                      const unsigned idle)
{
    int error = usbModel::USBOK;

    int      receivedbytes = 0;
    int      databytes;

    // Send out the request
    if (sendStandardRequest(addr, endp,
                            usbModel::USB_DEV_REQTYPE_GET,
                            usbModel::USB_REQ_GET_DESCRIPTOR,
                            (usbModel::STRING_DESCRIPTOR_TYPE << 8) | strindex, // wValue
                            0,                                                  // wIndex
                            reqlen,                                             // wLength
                            idle) != usbModel::USBOK)
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
            if (getDataFromDevice(dataPid(endp), &rxdata[receivedbytes], databytes, idle) != usbModel::USBOK)
            {
                error = usbModel::USBERROR;
            }
            else
            {
                receivedbytes += databytes;
                dataPidUpdate(endp);
            }


        } while ((receivedbytes < reqlen && receivedbytes < ((usbModel::deviceDesc*)rxdata)->bLength));

        if (chklen && receivedbytes != reqlen)
        {
            USBERRMSG("getDeviceDescriptor: unexpected length of data received (got %d, expected %d)\n", receivedbytes, reqlen);
            error = usbModel::USBERROR;
        }
        else
        {
            if (strindex)
            {
                usbModel::fmtUnicodeToStr((char*)data, (uint16_t*)&rxdata[2], (receivedbytes-2)/2);
                rxlen = (receivedbytes - 2)/2;
            }
            else
            {
                std::memcpy(data, rxdata, receivedbytes);
                rxlen = receivedbytes;
            }
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostGetDeviceDescriptor (const uint8_t  addr,   const uint8_t  endp,
                                               uint8_t  data[], const uint16_t reqlen, uint16_t &rxlen,
                                        const  bool     chklen, const unsigned idle)
{
    int      error         = usbModel::USBOK;
    int      receivedbytes = 0;
    int      databytes;

    // Send out the request
    if (sendStandardRequest(addr, endp,
                            usbModel::USB_DEV_REQTYPE_GET,
                            usbModel::USB_REQ_GET_DESCRIPTOR,
                            usbModel::DEVICE_DESCRIPTOR_TYPE << 8,    // wValue
                            0,                                        // wIndex
                            reqlen,                                   // wLength
                            idle) != usbModel::USBOK)
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
            if (getDataFromDevice(dataPid(endp), &rxdata[receivedbytes], databytes, idle) != usbModel::USBOK)
            {
                error = usbModel::USBERROR;
            }
            else
            {
                receivedbytes += databytes;
                dataPidUpdate(endp);
            }

        } while ((receivedbytes < reqlen && receivedbytes < ((usbModel::deviceDesc*)rxdata)->bLength));

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

int usbHost::usbHostGetConfigDescriptor  (const uint8_t  addr,   const uint8_t  endp,
                                                uint8_t  data[], const uint16_t reqlen, uint16_t &rxlen,
                                          const bool     chklen, const unsigned idle)
{
    int      error         = usbModel::USBOK;
    int      receivedbytes = 0;
    int      databytes;

    // Send out the request
    if (sendStandardRequest(addr, endp,
                            usbModel::USB_DEV_REQTYPE_GET,
                            usbModel::USB_REQ_GET_DESCRIPTOR,
                            usbModel::CONFIG_DESCRIPTOR_TYPE << 8,    // wValue
                            0,                                        // wIndex
                            reqlen,                                   // wLength
                            idle) != usbModel::USBOK)
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
            if (getDataFromDevice(dataPid(endp), &rxdata[receivedbytes], databytes, idle) != usbModel::USBOK)
            {
                error = usbModel::USBERROR;
                fprintf(stderr, "==>***ERROR: %s\n", errbuf);
            }
            else
            {
                receivedbytes += databytes;
                dataPidUpdate(endp);
            }
        } while ((receivedbytes < reqlen && receivedbytes < ((usbModel::configDesc*)rxdata)->wTotalLength));


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
int  usbHost::usbHostSetDeviceAddress (const uint8_t addr, const uint8_t endp, const uint16_t wValue, const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_DEV_REQTYPE_SET,
                               usbModel::USB_REQ_SET_ADDRESS,
                               wValue,                                  // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostSetDeviceConfig (const uint8_t  addr, const uint8_t  endp, const uint8_t index, const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_DEV_REQTYPE_SET,
                               usbModel::USB_REQ_SET_CONFIG,
                               (uint16_t)index,                         // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostClearDeviceFeature (const uint8_t  addr, const uint8_t endp,
                                        const uint16_t feature,
                                        const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_DEV_REQTYPE_SET,
                               usbModel::USB_REQ_CLEAR_FEATURE,
                               feature,                                 // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostSetDeviceFeature (const uint8_t  addr, const uint8_t endp,
                                      const uint16_t feature,
                                      const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_DEV_REQTYPE_SET,
                               usbModel::USB_REQ_SET_FEATURE,
                               feature,                                 // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostGetInterfaceStatus (const uint8_t addr, const uint8_t endp, uint16_t &status, const unsigned idle)
{
    return getStatus(addr, endp, usbModel::USB_IF_REQTYPE_GET, status, idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostClearInterfaceFeature (const uint8_t  addr,    const uint8_t endp,
                                           const uint16_t feature,
                                           const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_IF_REQTYPE_SET,
                               usbModel::USB_REQ_CLEAR_FEATURE,
                               feature,                                 // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostSetInterfaceFeature (const uint8_t    addr,    const uint8_t endp,
                                         const uint16_t feature,
                                         const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_IF_REQTYPE_SET,
                               usbModel::USB_REQ_SET_FEATURE,
                               feature,                                 // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostGetInterface (const uint8_t  addr,      const uint8_t endp,
                                  const uint16_t index,           uint8_t &altif,
                                  const unsigned idle)
{
    int error = usbModel::USBOK;
    int databytes;

    // Send out the request
    if (sendStandardRequest(addr, endp,
                          usbModel::USB_IF_REQTYPE_GET,
                          usbModel::USB_REQ_GET_INTERFACE,
                          0,                                        // wValue
                          index,                                    // wIndex
                          1,                                        // wLength
                          idle) != usbModel::USBOK)
    {
        error = usbModel::USBERROR;
    }
    else
    {
        // Send IN
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);

        // Receive requested data
        if (getDataFromDevice(dataPid(endp), rxdata, databytes, idle) != usbModel::USBOK)
        {
            error = usbModel::USBERROR;
        }
        else
        {
            altif = rxdata[0];
            dataPidUpdate(endp);
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostSetInterface (const uint8_t  addr,      const uint8_t endp,
                                  const uint16_t index,     const uint8_t altif,
                                  const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_IF_REQTYPE_SET,
                               usbModel::USB_REQ_SET_INTERFACE,
                               altif,                               // wValue
                               index,                               // wIndex
                               0,                                   // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostGetEndpointStatus (const uint8_t  addr,    const uint8_t  endp,
                                             uint16_t &status, const unsigned idle)
{
    return getStatus(addr, endp, usbModel::USB_EP_REQTYPE_GET, status, idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostClearEndpointFeature (const uint8_t  addr,    const uint8_t endp,
                                          const uint16_t feature,
                                          const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_EP_REQTYPE_SET,
                               usbModel::USB_REQ_CLEAR_FEATURE,
                               feature,                                 // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostSetEndpointFeature (const uint8_t  addr,    const uint8_t endp,
                                        const uint16_t feature,
                                        const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_EP_REQTYPE_SET,
                               usbModel::USB_REQ_SET_FEATURE,
                               feature,                                 // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostGetEndpointSynchFrame (const uint8_t  addr,      const uint8_t  endp,
                                                 uint16_t &framenum,
                                           const unsigned idle)
{
    int error = usbModel::USBOK;
    int databytes;

    // Send out the request
    if (sendStandardRequest(addr, endp,
                            usbModel::USB_EP_REQTYPE_GET,
                            usbModel::USB_REQ_GET_STATUS,
                            0,                                        // wValue
                            endp,                                     // wIndex
                            2,                                        // wLength
                            idle) != usbModel::USBOK)
    {
        error = usbModel::USBERROR;
    }
    else
    {
        // Send IN
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);

        // Receive requested data
        if (getDataFromDevice(dataPid(endp), rxdata, databytes, idle) != usbModel::USBOK)
        {
            error = usbModel::USBERROR;
        }
        else
        {
            framenum = (uint16_t)rxdata[0] | (((uint16_t)rxdata[1]) << 8);
            dataPidUpdate(endp);
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostBulkDataOut (const uint8_t  addr,   const uint8_t  endp,
                                       uint8_t  data[], const int      databytes,
                                 const int      maxpktsize,
                                 const unsigned idle)
{
    int                  error = usbModel::USBOK;
    int                  pid;
    uint32_t             args[4];
    int                  datasize;
    int                  numbytes;
    int                  numnaks = 0;
    int                  remaining_data;

    int datasent       = 0;

    while (true)
    {
        remaining_data = databytes - datasent;

        if (remaining_data > maxpktsize)
        {
            datasize = maxpktsize;
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

        // Send the OUT token
        sendTokenToDevice(usbModel::PID_TOKEN_OUT, addr, endp, idle);

        // Send data
        sendDataToDevice(dataPidUpdate(endp), &data[datasent], datasize, idle);


        USBDEVDEBUG ("==> usbHostBulkDataOut: waiting for ACK/NAK token\n");

        // Wait for acknowledge (either ACK or NAK)
        if ((error = apiWaitForPkt(nrzi, usbPliApi::IS_HOST)) < 0)
        {
            USBERRMSG ("***ERROR: usbHostBulkDataOut: error waiting for ACK\n");
            break;
        }
        else if ((error = usbPktDecode(nrzi, pid, args, data, numbytes)) != usbModel::USBOK)
        {
            usbPktGetErrMsg(sbuf);
            USBERRMSG ("***ERROR: usbHostBulkDataOut: received bad packet waiting for data\n%s", sbuf);
        }

        // If ACK then end of transaction if no more bytes
        if (pid == usbModel::PID_HSHK_ACK)
        {
            USBDEVDEBUG("==> usbHostBulkDataOut: seen ACK for DATAx\n");

            datasent += datasize;

            if ((databytes - datasent) == 0)
            {
                USBDEVDEBUG("==> usbHostBulkDataOut: remaining_data = %d\n", databytes - datasent);
                break;
            }
        }
        
        // Unexpected PID if not a NAK. NAK causes loop to send again, so no action.
        else if (pid == usbModel::PID_HSHK_NAK)
        {
            numnaks++;

            if (numnaks > MAXNAKS)
            {
                USBERRMSG ("usbHostBulkDataOut: seen too many NAKs\n");
                return usbModel::USBERROR;
            }
        }
        else
        {
            return usbModel::USBERROR;
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::usbHostBulkDataIn (const uint8_t  addr,      const uint8_t  endp,
                                      uint8_t* data,      const int      reqlen, const int maxpktsize,
                                const unsigned idle)
{
    int                  error = usbModel::USBOK;
    int                  rxbytes;
    int                  datasize;
    int                  receivedbytes = 0;

    while (true)
    {
        int remaining_data = reqlen - receivedbytes;
        
        USBDEVDEBUG("==> usbHostBulkDataIn: remaining_data = %d\n", remaining_data);

        if (remaining_data > maxpktsize)
        {
            datasize = maxpktsize;
        }
        else
        {
            if (remaining_data > 0)
            {
                datasize  = remaining_data;
            }
            else
            {
                if (remaining_data <= 0)
                {
                    break;
                }
            }
        }

        // Send IN token
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);
        
        USBDEVDEBUG("==> usbHostBulkDataIn: sent IN token to addr=%d endp=0x%02x\n", addr, endp);


        // Receive requested data
        if (getDataFromDevice(dataPid(endp), &data[receivedbytes], rxbytes, idle) != usbModel::USBOK)
        {
            error = usbModel::USBERROR;
        }
        else
        {
            receivedbytes += rxbytes;
            dataPidUpdate(endp);
        }
    }

    return error;
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Private method definitions
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

void usbHost::sendTokenToDevice (const int pid, const uint8_t addr, const uint8_t endp, const unsigned idle)
{
    int numbits = usbPktGen(nrzi, pid, addr, endp);
    
    USBDEVDEBUG("==> sendTokenToDevice: pid=0x%02x addr=%d endp=0x%02x numbits=%d\n", pid, addr, endp, numbits);

    if (numbits >= usbModel::MINPKTSIZEBITS)
    {        
        apiSendPacket(nrzi, numbits, idle);
    }
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

void usbHost::sendSofToDevice (const int pid, const uint16_t framenum, const unsigned idle)
{
    int numbits = usbPktGen(nrzi, pid, framenum);

    apiSendPacket(nrzi, numbits, idle);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::sendDataToDevice (const int datatype, const uint8_t data[], const int len, const unsigned idle)
{
    int error = usbModel::USBOK;

    USBDEVDEBUG("==> sendDataToDevice (datatype=0x%02x len=%d)\n", datatype, len);

    if (datatype != usbModel::PID_DATA_0 && datatype != usbModel::PID_DATA_1)
    {
        USBERRMSG ("***ERROR: sendDataToDevice: bad pid (0x%02x) when sending data\n", datatype);
        error = usbModel::USBERROR;
    }
    else
    {
        int numbits = usbPktGen(nrzi, datatype, data, len);

        apiSendPacket(nrzi, numbits, idle);
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
    if (apiWaitForPkt(nrzi, usbPliApi::IS_HOST) == usbModel::USBDISCONNECTED)
    {
        USBERRMSG ("***ERROR: getDataFromDevice: no device connected\n");
        error = usbModel::USBDISCONNECTED;
    }
    else if ((error = usbPktDecode(nrzi, pid, args, data, databytes)) != usbModel::USBOK)
    {
        USBERRMSG ("***ERROR: getDataFromDevice: received bad packet waiting for data\n");
        usbPktGetErrMsg(sbuf);
        USBERRMSG ("%s\n", sbuf);
    }
    else
    {
        if (pid == expPID)
        {
            USBDEVDEBUG("==> getDataFromDevice: Sending an ACK\n");

            // Send ACK
            int numbits = usbPktGen(nrzi, usbModel::PID_HSHK_ACK);
            apiSendPacket(nrzi, numbits, idle);
        }
        else
        {
            USBERRMSG ("***ERROR: getDataFromDevice: received unexpected packet ID waiting for data (0x%02x)\n", pid);
            error = usbModel::USBERROR;
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
int usbHost::sendStandardRequest(const uint8_t  addr,    const uint8_t  endp,
                                 const uint8_t  reqtype, const uint8_t  request,
                                 const uint16_t value,   const uint16_t index, const uint16_t length,
                                 const unsigned idle)
{
    int                  error = usbModel::USBOK;

    USBDEVDEBUG("==> sendStandardRequest (%d %d 0x%02x 0x%02x %d 0x%04x %d %d)\n", addr, endp, reqtype, request, length, value, index, idle);

    int                  pid;
    int                  databytes;
    uint32_t             args[4];

    // Check an SOF isn't due before sending packet
    checkSof();

    // SETUP
    sendTokenToDevice(usbModel::PID_TOKEN_SETUP, addr, endp, idle);

    // DATA0 (device get request)
    usbModel::setupRequest setup;
    setup.bmRequestType = reqtype;
    setup.bRequest      = request;
    setup.wValue        = value;
    setup.wIndex        = index;
    setup.wLength       = length;

    sendDataToDevice(usbModel::PID_DATA_0, (uint8_t*)&setup, sizeof(usbModel::setupRequest), idle);

    // After sending a setup token and DAT0 packat, the next data pid will be PID_DATA_1
    epdata0[epIdx(endp)][epDirIn(endp)] = false;

    do
    {
        // Wait for ACK
        if (apiWaitForPkt(nrzi, usbPliApi::IS_HOST) == usbModel::USBDISCONNECTED)
        {
            USBERRMSG ("***ERROR: sendStandardRequest: no device connected\n");
            error = usbModel::USBDISCONNECTED;
            break;
        }

        if (usbPktDecode(nrzi, pid, args, rxdata, databytes) != usbModel::USBOK)
        {
            USBERRMSG("***ERROR: sendStandardRequest: received bad packet waiting for ACK\n");
            usbPktGetErrMsg(sbuf);
            USBERRMSG("%s\n", sbuf);
            error = usbModel::USBERROR;
            break;
        }

        if (pid != usbModel::PID_HSHK_ACK && pid != usbModel::PID_HSHK_NAK)
        {
            USBERRMSG("***ERROR: sendStandardRequest: received unexpected packet ID (0x%02x)\n", pid);
            error = usbModel::USBERROR;
            break;
        }

    } while (pid == usbModel::PID_HSHK_NAK);

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbHost::getStatus (const uint8_t addr, const uint8_t endp, const uint8_t type, uint16_t &status, const unsigned idle)
{
    int error = usbModel::USBOK;
    int databytes;

    // Send out the request
    if ((error = sendStandardRequest(addr, endp,
                                     type,
                                     usbModel::USB_REQ_GET_STATUS,
                                     0,                                        // wValue
                                     0,                                        // wIndex
                                     2,                                        // wLength
                                     idle)) == usbModel::USBOK)
    {
        // Send IN
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);

        // Receive requested data
        if ((error = getDataFromDevice(dataPid(endp), rxdata, databytes, idle)) == usbModel::USBOK)
        {
            status = (uint16_t)rxdata[0] | (((uint16_t)rxdata[1]) << 8);
            dataPidUpdate(endp);
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

bool usbHost::checkConnected()
{
    unsigned line = apiReadLineState();

    if (line == usbModel::USB_SE0 && connected)
    {
        connected = false;
        USBDISPPKT("  %s USB DEVICE DISCONNECTED (at cycle %d)\n", name.c_str(), apiGetClkCount());
    }
    else if (line != usbModel::USB_SE0 && !connected)
    {
        connected = true;
        USBDISPPKT("  %s USB DEVICE CONNECTED (at cycle %d)\n", name.c_str(), apiGetClkCount());
    }

    return connected;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

void usbHost::checkSof (const unsigned idle)
{
    if (checkConnected() && keepalive)
    {
        // Get the current time in milliseconds
        float currtimeMs = usbHostGetTimeUs() / (float)1000.0;

        // If the frame number is less that the current time, send an SOF
        if ((float)framenum < currtimeMs)
        {
            apiSendIdle(idle);

            sendSofToDevice(usbModel::PID_TOKEN_SOF, (uint16_t)(framenum & 0x7ff));

            // Schedule a new SOF at the next 1 ms boundary
            framenum = (uint64_t)floor(currtimeMs) + 1;
        }
    }
}





