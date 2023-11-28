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
// usbHostWaitForConnection
//
// Public method to wait for the USB line to go from SE0 to J, indicating
// that a device is connected. It waits for the power on reset line to
// go inactive (via the low level API) before monitoring for a connection.
//
// Two optional arguments can be specified for the period to poll the line
// for a connection (polldelay, defaults to 10us) and a time out period
// to give up waiting (timeout, defaults to 3ms).
//
// The method returns the linestate if a connection detected, else returns
// usbModel::USBERROR if it timed out.
//
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
// usbHostGetDeviceStatus
//
// Public method to fetch the device status.
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a status argument reference
// in which the fetched status is returned. An optional idle argument
// specifies a period to wait before instigating the transaction (default 4
// clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostGetDeviceStatus (const uint8_t addr, const uint8_t endp, uint16_t &status, const unsigned idle)
{

    return getStatus(addr, endp, usbModel::USB_DEV_REQTYPE_GET, status, 0, 0, idle);
}

// -------------------------------------------------------------------------
// usbHostGetDeviceConfig
//
// Public method to fetch the device configuration (disabled/enabled)
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a cfgstate argument
// reference in which the fetched config state is returned. An optional
// index argument specifies which configuration to select if more than one
// (default 1). An optional idle argument specifies a period to wait before
// instigating the transaction (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
        if (getDataFromDevice(dataPid(endp), rxdata, databytes, false, idle) != usbModel::USBOK)
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
// usbHostGetStrDescriptor
//
// Public method to fetch a string descriptor from the connected device.
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a string index to select
// which string configuration to select (stridx). The string data is
// returned in the data buffer argument. The amount of data requested is
// specified in reqlen, but the actual amount of data returned is placed
// in the rxlen refernce argument. If the requested length is greater than
// the string length, then the data returned is only that of the string. If
// the requested length is less that the string length, then the data is
// truncated to the requested length. The string descriptors are stored
// as 16-bit unicode, but the returned data is decoded to 8-bit ASCII.
//
// The requested length can be checked against the returned length for a
// match if the optional chklen argument is true (default false) where an error
// is returned if not equal. An optional language ID (langid, default
// usbModel::LANGID_ENG_UK) can be specified to select a supported language
// of the device's string. An optional idle argument specifies a period to
// wait before instigating the transaction (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostGetStrDescriptor (const uint8_t  addr,     const uint8_t  endp,
                                      const uint8_t  stridx,         uint8_t  data[],
                                      const uint16_t reqlen,         uint16_t &rxlen,
                                      const bool     chklen,
                                      const uint16_t langid,
                                      const unsigned idle)
{
    int      error         = usbModel::USBOK;
    int      status;
    int      receivedbytes = 0;
    int      databytes;

    // Send out the request
    if (sendStandardRequest(addr, endp,
                            usbModel::USB_DEV_REQTYPE_GET,
                            usbModel::USB_REQ_GET_DESCRIPTOR,
                            (usbModel::STRING_DESCRIPTOR_TYPE << 8) | stridx,   // wValue
                            langid,                                             // wIndex
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
            if ((status = getDataFromDevice(dataPid(endp), &rxdata[receivedbytes], databytes, false, idle)) != usbModel::USBOK)
            {
                error = status;
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
            if (stridx)
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
// usbHostGetDeviceDescriptor
//
// Public method to fetch a device descriptor from the connected device.
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a data buffer (data)
// pointer, in which the returned data is placed, and requested data length
// (reqlen). The actual length of data returned is placed in the rxlen
// reference. If the size of the descriptor is less than the requested length,
// then only the length of the descriptor is returned. If it is greater than
// the requested length, then it is truncated to the request length. The
// requested length can be checked against the returned length for a match
// if the optional chklen argument is true default false) where an error is
// returned if not equal.  An optional idle argument specifies a period to
// wait before instigating the transaction (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostGetDeviceDescriptor (const uint8_t  addr,   const uint8_t  endp,
                                               uint8_t  data[], const uint16_t reqlen, uint16_t &rxlen,
                                        const  bool     chklen, const unsigned idle)
{
    int      error         = usbModel::USBOK;
    int      status;
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
            if ((status = getDataFromDevice(dataPid(endp), &rxdata[receivedbytes], databytes, false, idle)) != usbModel::USBOK)
            {
                error = status;
            }
            else
            {
                receivedbytes += databytes;
                dataPidUpdate(endp);
            }

        } while ((receivedbytes < reqlen && receivedbytes < ((usbModel::deviceDesc*)rxdata)->bLength) && !error);

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
// usbHostGetConfigDescriptor
//
// Public method to fetch a configuration descriptor from the connected
// device.
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a data buffer (data) pointer,
// in which the returned data is placed, and requested data length (reqlen).
// The actual length of data returned is placed in the rxlen reference.
// If the size of the descriptor is less than the requested length, then
// only the length of the descriptor is returned. If it is greater than the
// requested length, then it is truncated to the request length. The requested
// length can be checked against the returned length for a match if the
// optional chklen argument is true default false) where an error is returned
// if not equal. An optional idle  argument specifies a period to wait before
// instigating the transaction (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostGetConfigDescriptor  (const uint8_t  addr,   const uint8_t  endp,
                                                uint8_t  data[], const uint16_t reqlen, uint16_t &rxlen,
                                          const bool     chklen, const unsigned idle)
{
    int      error         = usbModel::USBOK;
    int      status;
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
            if ((status = getDataFromDevice(dataPid(endp), &rxdata[receivedbytes], databytes, false, idle)) != usbModel::USBOK)
            {
                error = status;
            }
            else
            {
                receivedbytes += databytes;
                dataPidUpdate(endp);
            }
        } while ((receivedbytes < reqlen && receivedbytes < ((usbModel::configDesc*)rxdata)->wTotalLength) && !error);


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
// usbHostSetDeviceAddress
//
// Public method to set the address of the connected device.
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a device address value
// (devaddr). An optional idle argument specifies a period to wait before
// instigating the transaction (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int  usbHost::usbHostSetDeviceAddress (const uint8_t addr, const uint8_t endp, const uint16_t devaddr, const unsigned idle)
{
    return sendStandardRequest(addr, endp,
                               usbModel::USB_DEV_REQTYPE_SET,
                               usbModel::USB_REQ_SET_ADDRESS,
                               devaddr,                                 // wValue
                               0,                                       // wIndex
                               0,                                       // wLength
                               idle);
}

// -------------------------------------------------------------------------
// usbHostSetDeviceConfig
//
// Public method to set the configuration of the connected device (enable)
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with an index argument to
// select which configuration if multiple available. An optional idle
// argument specifies a period to wait before instigating the transaction
// (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
// usbHostClearDeviceFeature
//
// Public method to clear a device feature of the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a feature argument to
// select which feature is cleared. An optional idle argument specifies a
// period to wait before instigating the transaction (default 4 clock
// periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
// usbHostSetDeviceFeature
//
// Public method to set a device feature of the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a feature argument to
// select which feature is set. An optional idle argument specifies a
// period to wait before instigating the transaction (default 4 clock
// periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
// usbHostGetInterfaceStatus
//
// Public method to get an interface's status on the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with an interface index (ifidx)
// argument to select which interface is selected if multiple interfaces.
// The interface status is returned in the referenced status argument.
// An optional idle argument specifies a period to wait before instigating
// the transaction (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostGetInterfaceStatus (const uint8_t addr, const uint8_t endp, const uint16_t ifidx, uint16_t &status, const unsigned idle)
{
    return getStatus(addr, endp, usbModel::USB_IF_REQTYPE_GET, status, 0, ifidx, idle);
}

// -------------------------------------------------------------------------
// usbHostClearInterfaceFeature
//
// Public method to clear an interface's feature on the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a feature argument to
// select which feature is cleared. An optional idle argument specifies a
// period to wait before instigating the transaction (default 4 clock
// periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
// usbHostSetInterfaceFeature
//
// Public method to set an interface's feature on the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a feature argument to
// select which feature is set. An optional idle argument specifies a
// period to wait before instigating the transaction (default 4 clock
// periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
// usbHostGetInterface
//
// Public method to get the alternative interface setting of a particular
// interface on the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with an index to select which
// interface to fetch from (index). The aternative interface setting
// value is returned in the referenced altif argument.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostGetInterface (const uint8_t  addr,      const uint8_t endp,
                                  const uint16_t index,           uint8_t &altif,
                                  const unsigned idle)
{
    int error = usbModel::USBOK;
    int status;
    int databytes;

    // Send out the request
    if ((status = sendStandardRequest(addr, endp,
                                      usbModel::USB_IF_REQTYPE_GET,
                                      usbModel::USB_REQ_GET_INTERFACE,
                                      0,                                        // wValue
                                      index,                                    // wIndex
                                      1,                                        // wLength
                                      idle)) != usbModel::USBOK)
    {
        error = status;
    }
    else
    {
        // Send IN
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);

        // Receive requested data
        if ((status = getDataFromDevice(dataPid(endp), rxdata, databytes, false, idle)) != usbModel::USBOK)
        {
            error = status;
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
// usbHostSetInterface
//
// Public method to set the alternative interface setting of a particular
// interface on the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with an index to select which
// interface to set (index). The aternative interface setting to be set
// passed in with the altif argument.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
// usbHostGetEndpointStatus
//
// Public method to get an endpoint's status from the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), and returns the status in the
// referenced status argument. An optional idle argument specifies a period
// to wait before instigating the transaction (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostGetEndpointStatus (const uint8_t  addr,    const uint8_t  endp,
                                             uint16_t &status, const unsigned idle)
{
    return getStatus(addr, endp, usbModel::USB_EP_REQTYPE_GET, status, 0, endp, idle);
}

// -------------------------------------------------------------------------
// usbHostClearEndpointFeature
//
// Public method to clear an endpoint's feature on the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a feature argument to
// select which feature is cleared. An optional idle argument specifies a
// period to wait before instigating the transaction (default 4 clock
// periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
// usbHostSetEndpointFeature
//
// Public method to set an endpoint's feature on the connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a feature argument to
// select which feature is cleared. An optional idle argument specifies a
// period to wait before instigating the transaction (default 4 clock
// periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
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
// usbHostGetEndpointSynchFrame
//
// Public method to get an endpoint's current sunch frame number from the
// connected device
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), and returns the frame number in the
// referenced framenum argument. An optional idle argument specifies a period
// to wait before instigating the transaction (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostGetEndpointSynchFrame (const uint8_t  addr,      const uint8_t  endp,
                                                 uint16_t &framenum,
                                           const unsigned idle)
{
    int error = usbModel::USBOK;
    int status;
    int databytes;

    // Send out the request
    if ((status = sendStandardRequest(addr, endp,
                                      usbModel::USB_EP_REQTYPE_GET,
                                      usbModel::USB_REQ_GET_STATUS,
                                      0,                                        // wValue
                                      endp,                                     // wIndex
                                      2,                                        // wLength
                                      idle)) != usbModel::USBOK)
    {
        error = status;
    }
    else
    {
        // Send IN
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);

        // Receive requested data
        if ((status = getDataFromDevice(dataPid(endp), rxdata, databytes, false, idle)) != usbModel::USBOK)
        {
            error = status;
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
// usbHostBulkDataOut
//
// Public method to send bulk data to a device's endpoint
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along a pointer to a data buffer (data)
// containing the data to be sent, with its length in databytes. The maximum
// packet size to use for the connected device is passed in with maxpktsize,
// and the method will divide the data into chunks of this size (or less for
// last chunk). An optional idle argument specifies a period to wait before
// instigating the transaction (default 4 clock periods).
//
// The method will send OUT token and data for each chunk, waiting for an
// acknowledgment from the device for each one.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostBulkDataOut (const uint8_t  addr,       const uint8_t  endp,
                                       uint8_t  data[],     const int      databytes,
                                 const int      maxpktsize,
                                 const unsigned idle)
{
    return sendDataOut(addr, endp, data, databytes, maxpktsize, false, idle);
}

// -------------------------------------------------------------------------
// usbHostIsoDataOut
//
// Public method to send isochronous data to a device's endpoint
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a pointer to a data buffer
// (data) containing the data to be sent, with its length in databytes. The
// maximum packet size to use for the connected device is passed in with
// maxpktsize, and the method will divide the data into chunks of this size
// (or less for last chunk). An optional idle argument specifies a period
// to wait before instigating the transaction (default 4 clock periods).
//
// The method will send OUT token and data for each chunk, but does not wait
// for any acknowledgements from the device.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostIsoDataOut (const uint8_t  addr,       const uint8_t  endp,
                                      uint8_t  data[],     const int      databytes,
                                const int      maxpktsize,
                                const unsigned idle)
{
    return sendDataOut(addr, endp, data, databytes, maxpktsize, true, idle);
}

// -------------------------------------------------------------------------
// usbHostBulkDataIn
//
// Public method to fetch bulk data from a device's endpoint
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a pointer to a data buffer
// (data) to return the data, with its requested length in reqlen. The
// maximum packet size to use for the connected device is passed in with
// maxpktsize, and an optional idle argument specifies a period to wait
// before instigating the transaction (default 4 clock periods).
//
// The method will send IN tokens and receive data, expecting no more than
// maxpktsize, and sends an acknowledgment. It will repeat this procedure
// until it has received all the requested data.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostBulkDataIn (const uint8_t  addr,      const uint8_t  endp,
                                      uint8_t* data,      const int      reqlen, const int maxpktsize,
                                const unsigned idle)
{
    return getDataIn(addr, endp, data, reqlen, maxpktsize, false, idle);
}

// -------------------------------------------------------------------------
// usbHostIsoDataIn
//
// Public method to fetch isochronous data from a device's endpoint
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a pointer to a data buffer
// (data) to return the data, with its requested length in reqlen. The
// maximum packet size to use for the connected device is passed in with
// maxpktsize, and an optional idle argument specifies a period to wait
// before instigating the transaction (default 4 clock periods).
//
// The method will send IN tokens and receive data, expecting no more than
// maxpktsize, but does not send acknowledgements for the data. It will
// repeat this procedure until it has received all the requested data.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::usbHostIsoDataIn (const uint8_t  addr,      const uint8_t  endp,
                                     uint8_t* data,      const int      reqlen, const int maxpktsize,
                               const unsigned idle)
{
    return getDataIn(addr, endp, data, reqlen, maxpktsize, true, idle);
}


// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Private method definitions
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// -------------------------------------------------------------------------
// sendDataOut
//
// Generic method to send data to the device.
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a pointer to a data buffer
// (data) containing the data to be sent, with its length in databytes. The
// maximum packet size to use for the connected device is passed in with
// maxpktsize, and the method will divide the data into chunks of this size
// (or less for last chunk). An isochronous argument selects whether a
// isochronous or bulk transfer. An optional idle argument specifies a period
// to wait before instigating the transaction (default 4 clock periods).
//
// The method will send OUT token and data for each chunk. It will wait for
// an acknowledgment if not an isochronous transfer.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::sendDataOut (const uint8_t  addr,       const uint8_t  endp,
                                uint8_t  data[],     const int      databytes,
                          const int      maxpktsize, const bool     isochronous,
                          const unsigned idle)
{
    int                  error = usbModel::USBOK;
    int                  pid;
    uint32_t             args[4];
    int                  datasize;
    int                  numbytes;
    int                  remaining_data;
    int                  numnaks            = 0;
    int                  datasent           = 0;

    // Loop until all the data sent, or an error occurs
    while ((databytes - datasent) && !error)
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
        if ((error = sendDataToDevice(dataPidUpdate(endp), &data[datasent], datasize, idle)) != usbModel::USBOK)
        {
            break;
        }
        // Wait for an acknowledgment if not an isochronous endpoint
        else if (!isochronous)
        {
            USBDEVDEBUG ("==> usbHostBulkDataOut: waiting for ACK/NAK token\n");

            // Wait for acknowledge (either ACK or NAK)
            if ((error = apiWaitForPkt(nrzi, usbPliApi::IS_HOST)) < 0)
            {
                USBERRMSG ("***ERROR: usbHostBulkDataOut: error waiting for ACK\n");
            }
            else if ((error = usbPktDecode(nrzi, pid, args, data, numbytes)) != usbModel::USBOK)
            {
                usbPktGetErrMsg(sbuf);
                USBERRMSG ("***ERROR: usbHostBulkDataOut: received bad packet waiting for data\n%s", sbuf);
            }
            // If ACK then end of transaction if no more bytes
            else if (pid == usbModel::PID_HSHK_ACK)
            {
                USBDEVDEBUG("==> usbHostBulkDataOut: seen ACK for DATAx\n");

                datasent += datasize;

                if ((databytes - datasent) == 0)
                {
                    USBDEVDEBUG("==> usbHostBulkDataOut: remaining_data = %d\n", databytes - datasent);
                }
            }
            // Unexpected PID if not a NAK. NAK causes loop to send again, so no action.
            else if (pid == usbModel::PID_HSHK_NAK)
            {
                numnaks++;

                if (numnaks > MAXNAKS)
                {
                    USBERRMSG ("usbHostBulkDataOut: seen too many NAKs\n");
                    error =  usbModel::USBERROR;
                }
            }
            else
            {
                error = usbModel::USBERROR;
            }
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// getDataIn
//
// Generic method to fetch data from a device's endpoint
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7), along with a pointer to a data buffer
// (data) to return the data, with its requested length in reqlen. The
// maximum packet size to use for the connected device is passed in with
// maxpktsize, and n isochronous argument selects whether an isochronous or
// bulk transfer. An optional idle argument specifies a period to wait
// before instigating the transaction (default 4 clock periods).
//
// The method will send IN tokens and receive data, expecting no more than
// maxpktsize, and sends an acknowledgment. It will repeat this procedure
// until it has received all the requested data.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::getDataIn (const uint8_t  addr,       const uint8_t  endp,
                              uint8_t* data,       const int      reqlen,
                        const int      maxpktsize, const bool     isochronous,
                        const unsigned idle)
{
    int                  error = usbModel::USBOK;
    int                  rxbytes;
    int                  datasize;
    int                  receivedbytes = 0;

    USBDEVDEBUG("==> usbHostBulkDataIn: addr=%d endp=0x%02x reqlen=%d\n", addr, endp, reqlen);

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
        if ((error = getDataFromDevice(dataPid(endp), &data[receivedbytes], rxbytes, isochronous, idle)) != usbModel::USBOK)
        {
            USBDEVDEBUG("==> usbHostBulkDataIn: seen error getting data from device\n");
            break;
        }
        else
        {
            receivedbytes += rxbytes;
            dataPidUpdate(endp);
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// sendTokenToDevice
//
// Method to send a token packet (not SOF) to the device.
//
// The token PID is specified in pid with a device address (addr) and an
// endpoint index (endp), including direction bit in bit 7). An optional
// idle argument specifies a period to wait before instigating the
// transaction (default 4 clock periods).
//
// No return value
//
// -------------------------------------------------------------------------

void usbHost::sendTokenToDevice (const int pid, const uint8_t addr, const uint8_t endp, const unsigned idle)
{
    int numbits = usbPktGen(nrzi, pid, addr, endp);

    USBDEVDEBUG("==> sendTokenToDevice: pid=0x%02x addr=%d endp=0x%02x numbits=%d\n", pid, addr, endp, numbits);

    apiSendPacket(nrzi, numbits, idle);
}

// -------------------------------------------------------------------------
// sendSofToDevice
//
// Overloaded method to send a token packet to the device.
//
// The SOF token PID is specified in pid with a frma enumber (framenum).
// An optional idle argument specifies a period to wait before instigating the
// transaction (default 4 clock periods).
//
// No return value
//
// -------------------------------------------------------------------------

void usbHost::sendSofToDevice (const int pid, const uint16_t framenum, const unsigned idle)
{
    int numbits = usbPktGen(nrzi, pid, framenum);

    apiSendPacket(nrzi, numbits, idle);
}

// -------------------------------------------------------------------------
// sendDataToDevice
//
// Method to send data to the device in a DATAx packet
//
// The datatype PID is specified in datatype, with that data buffer
// containing  the data (data) and its length (len). An optional idle
// argument specifies a period to wait before instigating the transaction
// (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an invalid datatype
// PID (i.e. not usbModel::PID_DATA_0 or usbModel::PID_DATA_1), the
// usbModel::USBERROR is returned.
//
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
// getDataFromDevice
//
// Method to fetch a data packet from the connected device
//
// The expected data packet PID (usbModel::PID_DATA_0 or usbModel::PID_DATA_1)
// is passed in with expPID, along with a data buffer pointer (data) to
// placed the returned bytes. The length of the data received is returned in
// databytes. No acknowledgment of the received data is sent when the nack
// argument is true, but one is sent if false (the default). An optional idle
// argument specifies a period to wait before instigating the transaction
// (default 4 clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::getDataFromDevice(const int expPID, uint8_t data[], int &databytes, bool noack, const unsigned idle)
{
    int                  error = usbModel::USBOK;
    int                  status;
    int                  pid;
    uint32_t             args[4];

    // Wait for data
    status = apiWaitForPkt(nrzi, usbPliApi::IS_HOST);

    if (status == usbModel::USBDISCONNECTED)
    {
        USBERRMSG ("***ERROR: getDataFromDevice: no device connected\n");
        error = status;
    }
    else if (status == usbModel::USBERROR || status == usbModel::USBNORESPONSE)
    {
        USBERRMSG ("***ERROR: getDataFromDevice: bad status waiting for packet (%d)\n", status);
        error = status;
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
            if (!noack)
            {
                USBDEVDEBUG("==> getDataFromDevice: Sending an ACK\n");

                // Send ACK
                int numbits = usbPktGen(nrzi, usbModel::PID_HSHK_ACK);
                apiSendPacket(nrzi, numbits, idle);
            }
        }
        else
        {
            USBDEVDEBUG("==> getDataFromDevice: unexpected pid. Got 0x%02x, exp 0x%02x\n", pid, expPID);

            USBERRMSG ("***ERROR: getDataFromDevice: received unexpected packet ID waiting for data (0x%02x)\n", pid);
            error = usbModel::USBERROR;
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// sendStandardRequest
//
// Method to send a standard device request to the connected device, with
// a setup packet as the OUT data
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7). The bmRequest type (reqtype) and
// bRequest (request) are also specified, along with the wValue (value)
// wIndex (index), and wLength (length values, )to set in the appropriate
// setup packet fields. These last three paarmeters default to 0. An optional
// idle argument specifies a period to wait before instigating the transaction
// (default 4 clock periods).
//
// The method first checks whether an SOF token needs to be sent, and sends
// one if the time since the last one has expired. A SETUP token is
// sent, and a setup packet constructed as data which is then sent
// in a DATA0 OUT packet. The internal state for DATA0/DATA1 is reset
// for DATA1 for the selected endpoint, since these are sync'd on a
// SETUP token. The method then waist for an acknowledgement packet from
// the device.
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::sendStandardRequest(const uint8_t  addr,    const uint8_t  endp,
                                 const uint8_t  reqtype, const uint8_t  request,
                                 const uint16_t value,   const uint16_t index, const uint16_t length,
                                 const unsigned idle)
{
    int                  error = usbModel::USBOK;
    int                  status;
    int                  pid;
    int                  databytes;
    uint32_t             args[4];

    USBDEVDEBUG("==> sendStandardRequest (%d %d 0x%02x 0x%02x %d 0x%04x %d %d)\n", addr, endp, reqtype, request, length, value, index, idle);

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

    if (error = sendDataToDevice(usbModel::PID_DATA_0, (uint8_t*)&setup, sizeof(usbModel::setupRequest), idle))
    {
        return error;
    }

    // After sending a setup token and DATA0 packet, the next data pid will be PID_DATA_1
    epdata0[epIdx(endp)][epDirIn(endp)] = false;

    do
    {
        // Wait for ACK
        status = apiWaitForPkt(nrzi, usbPliApi::IS_HOST);

        if (status == usbModel::USBDISCONNECTED)
        {
            USBERRMSG ("***ERROR: sendStandardRequest: no device connected\n");
            error = status;
        }
        else if (status == usbModel::USBNORESPONSE || status == usbModel::USBERROR)
        {
            USBERRMSG ("***ERROR: sendStandardRequest: bad status waiting for packet (%d)\n", status);
            error = status;
        }
        else
        {

            if ((status = usbPktDecode(nrzi, pid, args, rxdata, databytes)) != usbModel::USBOK)
            {
                USBERRMSG("***ERROR: sendStandardRequest: received bad packet waiting for ACK\n");
                usbPktGetErrMsg(sbuf);
                USBERRMSG("%s\n", sbuf);
                error = status;
                break;
            }

            if (pid != usbModel::PID_HSHK_ACK && pid != usbModel::PID_HSHK_NAK)
            {
                USBERRMSG("***ERROR: sendStandardRequest: received unexpected packet ID (0x%02x)\n", pid);
                error = usbModel::USBERROR;
                break;
            }
        }

    } while (pid == usbModel::PID_HSHK_NAK && !error);

    return error;
}

// -------------------------------------------------------------------------
// getStatus
//
// Generic method to get a status from  device, interface or endpoint of
// connected device.
//
// The method takes a device address (addr) and an endpoint index (endp),
// including direction bit in bit 7). The type of access is passed in (type),
// specifying whether device (USB_DEV_REQTYPE_GET), interface (USB_IF_REQTYPE_GET)
// or endpoint (USB_EP_REQTYPE_GET). The status is returned in the referenced
// status parameter. The optional wValue and wIndex can be used to set the
// setup request fields, with default values of 0. An optional idle argument
// specifies a period to wait before instigating the transaction (default 4
// clock periods).
//
// The method returns usbModel::USBOK on success. If an error occurred during
// the transaction, then usbModel::USBERROR is returned, or if a device
// disconnection occurred, then usbModel::USBDISCONNECTED is returned.
// If a valid, but unsupported, response packet is received from the device
// then it returns usbModel::USBUNSUPPORTED. If a timeout occurred waiting
// for a response packet, then usbModel::USBNORESPONSE is returned.
//
// -------------------------------------------------------------------------

int usbHost::getStatus (const uint8_t addr, const uint8_t endp, const uint8_t type, uint16_t &status, const uint16_t wValue, const uint16_t wIndex, const unsigned idle)
{
    int error = usbModel::USBOK;
    int databytes;

    // Send out the request
    if ((error = sendStandardRequest(addr, endp,
                                     type,
                                     usbModel::USB_REQ_GET_STATUS,
                                     wValue,                                   // wValue
                                     wIndex,                                   // wIndex
                                     2,                                        // wLength
                                     idle)) == usbModel::USBOK)
    {
        // Send IN
        sendTokenToDevice(usbModel::PID_TOKEN_IN, addr, endp, idle);

        // Receive requested data
        if ((error = getDataFromDevice(dataPid(endp), rxdata, databytes, false, idle)) == usbModel::USBOK)
        {
            status = (uint16_t)rxdata[0] | (((uint16_t)rxdata[1]) << 8);
            dataPidUpdate(endp);
        }
    }

    return error;
}

// -------------------------------------------------------------------------
// checkConnected
//
// Method to retuen the status of the line with regard to a device being
// connected or not. Returns true if a connected device, else returns
// false.
//
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
// checkSof
//
// Method to keep track of sending SOF packets each frame, updating state
// of when the last SOF sent and sending one if the frame period has expired
// (basically at each frame boundary since time 0). An optional idle argument
// specifies a period to wait before instigating the transaction (default 4
// clock periods).
//
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





