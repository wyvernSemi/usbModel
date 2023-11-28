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
// runUsbDevice
//
// Public method to start the device being active on the line.
// An optional idle argument (that has a default value) can be
// given to set the delay between responses from the device.
//
// Returns usbModel::USBOK on success, or usbModel::USBERROR
// otherwise.
//
//-------------------------------------------------------------

int usbDevice::usbDeviceRun(const int idle)
{
    int                  error = usbModel::USBOK;
    int                  pid;
    uint32_t             args[usbModel::MAXNUMARGS];
    int                  databytes;

    // Ensure that reset is deasserted
    apiWaitOnNotReset();

    // Connect the device to the line
    apiEnablePullup();

    // Loop forever
    while (error == usbModel::USBOK)
    {
        if (waitForExpectedPacket(usbModel::PID_INVALID, pid, args, rxdata, databytes) != usbModel::USBOK)
        {
             USBDEVDEBUG("<== usbDeviceRun: seen error waiting for a packet\n");
             error = usbModel::USBERROR;
             break;
        }
        else
        {
            USBDEVDEBUG("<== usbDeviceRun: received TOKEN (pid=0x%02x)\n", pid);

            // Process initiating packet types
            switch(pid)
            {
            case usbModel::PID_TOKEN_SETUP:
                if (processControl(args[usbModel::ARGADDRIDX], args[usbModel::ARGENDPIDX], idle) != usbModel::USBOK)
                {
                    USBDEVDEBUG("<== usbDeviceRun: seen error processing control transactions\n");
                    error = usbModel::USBERROR;
                }
                else
                {
                    USBDEVDEBUG("<== usbDeviceRun: received SETUP token, so reset the DATAx to DATA0\n");

                    // When a token received, reset the DATA0/1 to DATA0
                    epdata0[epIdx(args[usbModel::ARGENDPIDX])][epDirIn(args[usbModel::ARGENDPIDX])] = true;
                }
                break;

            case usbModel::PID_TOKEN_IN:

                if (processIn(args, databytes, idle) != usbModel::USBOK)
                {
                    USBDEVDEBUG("<== usbDeviceRun: seen an error processing an IN token\n");

                    error = usbModel::USBERROR;
                }
                else
                {
                    USBDEVDEBUG("<== usbDeviceRun: received IN token\n");
                }
                break;

            case usbModel::PID_TOKEN_OUT:
                if (processOut(args, rxdata, databytes, idle) != usbModel::USBOK)
                {
                    USBDEVDEBUG("<== usbDeviceRun: seen an error processing an OUT token\n");

                    error = usbModel::USBERROR;
                }
                else
                {
                    USBDEVDEBUG("<== usbDeviceRun: received OUT token\n");
                }
                break;

            case usbModel::PID_TOKEN_SOF:
                if (processSOF(args, idle) != usbModel::USBOK)
                {
                    USBDEVDEBUG("<== usbDeviceRun: seen an error processing an SOF token\n");

                    error = usbModel::USBERROR;
                }
                else
                {
                     USBDEVDEBUG("<== usbDeviceRun: received SOF token\n");
                }
                break;

            default:
                USBERRMSG("runUsbDevice: Received unexpected packet ID (0x%x)\n", pid);
                error = usbModel::USBERROR;
                break;
            }
        }
    }

    return error;
}

//-------------------------------------------------------------
// waitForExpectedPacket()
//
// Method to wait for the receipt of a particular PID packet
// type. A packet type of PID_INVALID will not check the type,
// otherwise a STALL acknowledge is sent if a mismatch on
// received PID. Will reset the device is reset detected on
// USB line. Will ignore bad packets by default, but can be
// changed with ignorebadpkts set to false.
//
// The methods will return usbModel::USBOK if successful, else
// usbModel::USBERROR on bad received packets (if not ignoring)
// or unexpected received PIDs (if checking).
//
//-------------------------------------------------------------

int usbDevice::waitForExpectedPacket(const int pktType, int &pid, uint32_t* args, uint8_t* data, int &databytes, const bool ignorebadpkts, const int timeout)
{
    int error = usbModel::USBOK;
    int status;
    int numbits;

    USBDEVDEBUG ("<== waitForExpectedPacket: waiting for a packet (0x%02x)\n", pktType);

    while (true)
    {
        // Wait for a packet
        if ((status = apiWaitForPkt(nrzi, usbPliApi::IS_DEVICE)) == usbModel::USBRESET)
        {
            USBDISPPKT ( "  %s SEEN RESET\n", name.c_str());

            // If a reset seen, reset internal state
            reset();
            continue;
        }
        else if (status == usbModel::USBSUSPEND)
        {
            // If suspension seen, this device model does nothing
            USBDISPPKT ( "  %s SEEN SUSPEND\n", name.c_str());
            continue;
        }

        if (usbPktDecode(nrzi, pid, args, data, databytes) != usbModel::USBOK)
        {

            for(int i = 0; i < databytes; i++)
                USBDEVDEBUG("%02x ", data[i]);
            USBDEVDEBUG("\n");

            // Ignore any packets that have errors
            if (ignorebadpkts)
            {
                USBDEVDEBUG ("<== waitForExpectedPacket: ignoring a bad packet\n");
                continue;
            }
            else
            {
                USBDEVDEBUG ("<== waitForExpectedPacket: seen a bad packet and returning an error\n");
                return usbModel::USBERROR;
            }
        }
        else
        {

            USBDEVDEBUG ("<== waitForExpectedPacket: received a good packet (pid=0x%02x args={%d %d %d} dataytes=%d)\n", pid, args[0], args[1], args[2], databytes);
            break;
        }
    }

    // Check packet is as expected
    if (status == usbModel::USBOK && pktType != PID_NO_CHECK && pid != pktType)
    {
        USBDEVDEBUG("<== waitForExpectedPacket: Received unexpected pid\n");

        // Generate a STALL handshake for the error
        sendPktToHost(usbModel::PID_HSHK_STALL, DEFAULT_IDLE);

        USBERRMSG("waitForExpectedPacket: Received unexpected pid (got 0x%02x, expected 0x%02x)\n", pid, pktType);

        error = usbModel::USBERROR;
    }

    return error;
}

//-------------------------------------------------------------
// sendPktToHost
//
// A set of overloaded methods to send a packet towards the
// host. The type of packet sent is dependent on the argument
// types. All have a pid argument (which is checked as valid
// for the type of packet to be sent).
//
// A DATAx packet is selected with a uint8_t* second argument
// followed by an integer length.
//
// A TOKEN packet is sent with a uint8_t address and endpoint
// index value arguments.
//
// An SOF packet is sent with a uint16_t framenum argument
//
// A HANDSHAKE packet is sent with no packet arguments
//
// All methods have an optional final integer idle argument
// with a default value, which sets the delay before sending
// the packet.
//
// The methods will return usbModel::USBOK if successful, else
// usbModel::USBERROR on bad PIDs.
//
//-------------------------------------------------------------

// Data
int usbDevice::sendPktToHost(const int pid, const uint8_t data[], unsigned datalen, const int idle)
{
    int error = usbModel::USBOK;

    USBDEVDEBUG ("<== sendPktToHost: DATAx (pid=0x%02x, datalen=%d)\n", pid, datalen);

    // Check for valid PID
    if (pid != usbModel::PID_DATA_0 && pid != usbModel::PID_DATA_1)
    {
        USBDEVDEBUG ("<== sendPktToHost: DATAx seen invalid PID\n");

        error = usbModel::USBERROR;
        USBERRMSG("sendPktToHost(DATA): Invalid pid for packet type (0x%02x)", pid);
    }
    else
    {
        USBDEVDEBUG("<== sendPktToHost: sending packet to host\n");

        // Generate packet
        int numbits = usbPktGen(nrzi, pid, data, datalen);

        // Send over the USB line
        apiSendPacket(nrzi, numbits, idle);
    }

    return error;
}

// Token
int usbDevice::sendPktToHost(const int pid, const uint8_t addr, const uint8_t endp, const int idle)
{
    int error = usbModel::USBOK;

    USBDEVDEBUG ("<== sendPktToHost: TOKEN (pid=0x%02x addr=0x%02x, endp=0x%02x)\n", addr, endp);

    // Check for valid PID
    if( pid != usbModel::PID_TOKEN_OUT && pid != usbModel::PID_TOKEN_IN && pid != usbModel::PID_TOKEN_SETUP)
    {
        USBDEVDEBUG ("<== sendPktToHost: TOKEN seen invalid PID\n");

        error = usbModel::USBERROR;
        USBERRMSG("sendPktToHost(TOKEN): Invalid pid for packet type (0x%02x)", pid);
    }
    else
    {
        USBDEVDEBUG("<== sendPktToHost: sending token packet to host\n");

        // Generate packet
        int numbits = usbPktGen(nrzi, pid, addr, endp);

        // Send over the USB line
        apiSendPacket(nrzi, numbits, idle);
    }

    return error;
}

// SOF
int usbDevice::sendPktToHost(const int pid, const uint16_t framenum, const int idle)
{
    int error = usbModel::USBOK;

    USBDEVDEBUG ("<== sendPktToHost: SOF (pid=0x%02x framenum=0x%04x)\n", pid, framenum);

    // Check for valid PID
    if (pid != usbModel::PID_TOKEN_SOF)
    {
        USBDEVDEBUG ("<== sendPktToHost: SOF seen invalid PID\n");

        error = usbModel::USBERROR;
        USBERRMSG("sendPktToHost(SOF): Invalid pid for packet type (0x%02x)", pid);
    }
    else
    {
        USBDEVDEBUG("<== sendPktToHost: sending SOF packet to host\n");

        // Generate packet
        int numbits = usbPktGen(nrzi, pid, framenum);

        // Send over the USB line
        apiSendPacket(nrzi, numbits, idle);
    }

    return error;
}

// Handshake
int usbDevice::sendPktToHost(const int pid, const int idle)
{
    int error = usbModel::USBOK;

    USBDEVDEBUG ("<== sendPktToHost: HANDSHAKE (pid=0x%02x)\n", pid);

    // Check for valid PID
    if (pid != usbModel::PID_HSHK_ACK && pid != usbModel::PID_HSHK_NAK && pid != usbModel::PID_HSHK_STALL)
    {
        USBDEVDEBUG ("<== sendPktToHost: HSHK seen invalid PID\n");
        
        error = usbModel::USBERROR;
        USBERRMSG("sendPktToHost(HANDSHAKE): Invalid pid for packet type (0x%02x)", pid);
    }
    else
    {
        USBDEVDEBUG("<== sendPktToHost: sending HSHK packet to host\n");
        
        // Generate packet
        int numbits = usbPktGen(nrzi, pid);

        // Send over the USB line
        apiSendPacket(nrzi, numbits, idle);
    }

    return error;
}

//-------------------------------------------------------------
// processControl
//
// Method that processes a control transaction (a SETUP token
// received). It loops waiting on receiving a valid DATA0
// packet, which it then ACKS. It decodes the bmRequestType
// fieled of the data and selects the appropriate handler
// method to further process the transaction.
//
// The method will return usbModel::USBOK if successful, else
// usbModel::USBERROR on bad addr/endp arguments, or bad
// received packets.
//
//-------------------------------------------------------------

int usbDevice::processControl(const uint32_t addr, const uint32_t endp, const int idle)
{
    int                  error = usbModel::USBOK;
    int                  pid;
    uint32_t             args[usbModel::MAXNUMARGS];
    int                  databytes;

    USBDEVDEBUG ( "<== processControl (addr = 0x%02x, endp = 0x%02x)\n", addr, endp);

    // Check Address/endp is 0/0 or a previously set address and a valid endpoint
    if (!((addr == 0 && endp == 0) || (addr == devaddr && epvalid[epIdx(endp)][epDirIn(endp)])))
    {
        USBDEVDEBUG ("<== processControl: bad address or endpoint (addr=0x%02x, endp=0x%02x)\n", addr, endp);

        error = usbModel::USBERROR;
        USBERRMSG("processControl: Received bad addr/endp (0x%02x 0x%02x)\n", addr, endp);
    }

    // Wait for DATA0 packet
    USBDEVDEBUG ( "Waiting for DATA0\n");
    if (waitForExpectedPacket(dataPid(endp), pid, args, rxdata, databytes) != usbModel::USBOK)
    {
        USBDEVDEBUG("%s", errbuf);
        error = usbModel::USBERROR;
    }

    // If an error occured on the addr/endp checks or the DATA0 reception, generate a STALL handshake
    if (error == usbModel::USBERROR)
    {
        ephalted[endp & 0xf][endp >> 7 & 1] = true;
        sendPktToHost(usbModel::PID_HSHK_STALL, idle);
    }
    else
    {
        // Map received data over the expected request type
        usbModel::setupRequest* sreq = (usbModel::setupRequest*)rxdata;

        USBDEVDEBUG ( "<== received device request (0x%x)\n", sreq->bmRequestType);

        // A decode request (device, interface, endpoint)
        switch(sreq->bmRequestType)
        {
        // A device request
        case usbModel::USB_DEV_REQTYPE_SET:
        case usbModel::USB_DEV_REQTYPE_GET:
            error = handleDevReq(sreq, endp, idle);
            break;

        // An interface request
        case usbModel::USB_IF_REQTYPE_SET:
        case usbModel::USB_IF_REQTYPE_GET:
            error = handleIfReq(sreq, endp, idle);
            break;

        // An endpoint request
        case usbModel::USB_EP_REQTYPE_SET:
        case usbModel::USB_EP_REQTYPE_GET:
            error = handleEpReq(sreq, endp, idle);
            break;

        // Generate a STALL handshake if an unknown bmRequestType
        default:
            ephalted[endp & 0xf][endp >> 7 & 1] = true;
            sendPktToHost(usbModel::PID_HSHK_STALL, idle);
            error = usbModel::USBERROR;
            break;
        }
    }

    return error;
}

//-------------------------------------------------------------
// processIn
//
// A method that process a data transaction from device to
// host having recived an IN token.
//
// If a user callback function is defined, this is called
// to generate response data which is sent back to the host and
// an ACK returned. The method will return a STALL acknowledgement
// to the host if not a valid address and/or endpoint for
// this device, or if the endpoint is halted. The user callback
// can also return indicating a stall error, which also generates
// a STALL acknowledgement. If the callback indicates a NAK
// condition then a NAK acknowledgement is returned to the host.
//
// The method returns usbModel::USBERROR if an error occurs
// when sending the data packet, otherwise it returns
// usbModel::USBOK
//
//-------------------------------------------------------------

int usbDevice::processIn (const uint32_t args[], int &databytes, const int idle)
{
    int                error  = usbModel::USBOK;

    uint8_t            addr   = args[usbModel::ARGADDRIDX];
    uint8_t            endp   = args[usbModel::ARGENDPIDX] | usbModel::DIRTOHOST;
    int                numbytes;

    dataResponseType_e cbresp = usbDevice::ACK;

    USBDEVDEBUG ("<== processIn (addr = 0x%02x, endp = 0x%02x)\n", addr, endp);

    // Check Address is a previously set address and a valid endpoint
    if (!(addr == devaddr && epvalid[epIdx(endp)][epDirIn(endp)] && !ephalted[epIdx(endp)][epDirIn(endp)]))
    {
        // Generate a STALL handshake for the error
        sendPktToHost(usbModel::PID_HSHK_STALL);

        error = usbModel::USBERROR;
        USBERRMSG("processIn: Received bad addr/endp (0x%02x 0x%02x)\n", addr, endp);
    }
    else
    {
        // If a data callback is set, fetch the data from this function
        if (datacb != NULL)
        {
            cbresp = datacb(endp, rxdata, numbytes);
        }

        if (cbresp == usbDevice::STALL)
        {
            ephalted[epIdx(endp)][epDirIn(endp)] = true;
            sendPktToHost(usbModel::PID_HSHK_STALL);
            error = usbModel::USBERROR;
        }
        else if (cbresp == usbDevice::NAK)
        {
            sendPktToHost(usbModel::PID_HSHK_NAK, idle);
        }
        else
        {
            if (sendInData(rxdata, numbytes, endp, true, idle) != usbModel::USBOK)
            {
                return usbModel::USBERROR;
            }
        }
    }

    return error;
}

//-------------------------------------------------------------
// processOut
//
// A method that process a data transaction from host to
// device, having received an OUT token. The code waits for
// the data then, if a user callback function is defined, this
// is called to process the data sent from the host.
//
// The method will return a STALL acknowledgement
// to the host if not a valid address and/or endpoint for
// this device, or if the endpoint is halted. The user callback
// can also return indicating a stall error, which also generates
// a STALL acknowledgement. If the callback indicates a NAK
// condition then a NAK acknowledgement is returned to the host.
//
// The method returns usbModel::USBERROR if an error occurs
// when sending the data packet, otherwise it returns
// usbModel::USBOK
//
//-------------------------------------------------------------

int usbDevice::processOut (const uint32_t args[], uint8_t data[], int databytes, const int idle)
{
    int                  error    = usbModel::USBOK;

    uint32_t             dargs[usbModel::MAXNUMARGS];
    int                  pid;
    uint8_t              addr     = args[usbModel::ARGADDRIDX];
    uint8_t              endp     = args[usbModel::ARGENDPIDX];
    int                  numbytes;

    int                  epidx    = epIdx(endp);
    int                  epdir    = epDirIn(endp);


    dataResponseType_e   cbresp = usbDevice::ACK;

    USBDEVDEBUG ("<== processOut (addr = 0x%02x, endp = 0x%02x)\n", addr, endp);

    // Check Address is a previously set address and a valid endpoint
    if (!(addr == devaddr && epvalid[epidx][epdir] && !ephalted[epidx][epdir]))
    {
        // Generate a STALL handshake for the error
        sendPktToHost(usbModel::PID_HSHK_STALL);

        error = usbModel::USBERROR;
        USBERRMSG("processOut: Received bad addr/endp (0x%02x 0x%02x)\n", addr, endp);
    }
    else
    {
        // Wait for DATAx packet
        USBDEVDEBUG ( "processOut: Waiting for DATAx\n");
        if (waitForExpectedPacket(dataPid(endp), pid, dargs, data, numbytes) != usbModel::USBOK)
        {
            USBDEVDEBUG("%s", errbuf);
            error = usbModel::USBERROR;
        }

        // If a data callback is set, send the data to this function
        if (datacb != NULL)
        {
             cbresp = datacb(args[usbModel::ARGENDPIDX], data, numbytes);
        }

        if (cbresp == usbDevice::STALL)
        {
            ephalted[endp & 0xf][(endp >> 7) & 1] = true;
            sendPktToHost(usbModel::PID_HSHK_STALL);
            error = usbModel::USBERROR;
        }
        else if (cbresp == usbDevice::NAK)
        {
            sendPktToHost(usbModel::PID_HSHK_NAK, idle);
        }
        else
        {
            sendPktToHost(usbModel::PID_HSHK_ACK, idle);
            dataPidUpdate(endp);
        }
    }

    return error;
}

//-------------------------------------------------------------
// processSOF
//
// Method to process after receiving an SOF token. The frame
// number from the SOF is stored in the framenum state of the
// device.
//
// The method always returns usbModel::USBOK
//
//-------------------------------------------------------------

int  usbDevice::processSOF(const uint32_t args[], const int idle)
{
    USBDISPPKT("  %s RX SOF: FRAME NUMBER 0x%04x\n", name.c_str(), args[usbModel::ARGFRAMEIDX]);

    framenum = args[usbModel::ARGFRAMEIDX] & 0x7ff;

    return usbModel::USBOK;
}


//-------------------------------------------------------------
// handleDevReq
//
// Called if bmRequestType is for a device and handles each
// valid bRequest command:
//
//  GET STATUS
//  CLEAR FEATURE
//  SET FEATURE
//  SET ADDRESS
//  GET DESCRIPTOR for types (in wValue)
//    DEVICE DESCRIPTOR
//    CONFIGURATION DESCRIPTOR
//    STRING DESCRIPTOR
//    INTERFACE DESCRIPTOR
//    ENDPOINT DESCRIPTOR
//    CLASS SPECIFIC (CS) INTERFACE DESCRIPTOR
//  SET DESCRIPTOR
//  GET CONFIGURATION
//  SET CONFIGURATION
//
// All other descriptors are treated as an error.
//
//-------------------------------------------------------------

int usbDevice::handleDevReq(const usbModel::setupRequest* sreq, const uint8_t endp, const int idle)
{
    int                  pid;
    uint32_t             args[usbModel::MAXNUMARGS];
    int                  databytes;
    int                  datasize;
    int                  index;
    bool                 cfgstate;

    uint8_t              desctype;
    uint8_t              descidx;

    USBDEVDEBUG ( "<== handleDevReq (bRequest=0x%x wValue=0x%04x wLength=0x%04x)\n", sreq->bRequest, sreq->wValue, sreq->wLength);

    switch(sreq->bRequest)
    {
    case usbModel::USB_REQ_GET_STATUS:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // Construct status data
        uint8_t buf[2];
        buf[0]   = REMOTE_WAKEUP_STATE | SELF_POWERED_STATE;
        buf[1]   = 0;
        datasize = 2;

        // Construct a formatted output string
        snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET STATUS\n    " FMT_DATA_GREY "remWkup=%d selfPwd=%d" FMT_NORMAL "\n",
                 name.c_str(),
                 REMOTE_WAKEUP_STATE ? 1 : 0,
                 SELF_POWERED_STATE  ? 1 : 0);

        // Send the response to the GET_STATUS command
        return sendGetResp (sreq, buf, datasize, endp, sbuf);
        break;

    case usbModel::USB_REQ_CLEAR_FEATURE:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // No remote wakeup or test mode support, so ignore
        USBDISPPKT("  %s RX DEV REQ: CLEAR FEATURE 0x%04x\n", name.c_str(), sreq->wValue);
        break;

    case usbModel::USB_REQ_SET_FEATURE:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // No remote wakeup or test mode support, so ignore
        USBDISPPKT("  %s RX DEV REQ: SET FEATURE 0x%04x\n", name.c_str(), sreq->wValue);
        break;

    case usbModel::USB_REQ_SET_ADDRESS:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // Extract address
        devaddr = sreq->wValue;

        USBDISPPKT("  %s RX DEV REQ: SET ADDRESS 0x%02x\n", name.c_str(), devaddr);
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

            // Generate an ACK handshake for the SETUP data
            sendPktToHost(usbModel::PID_HSHK_ACK, idle);
            dataPidUpdate(endp);

            // Set returned data size to be the whole of the descriptor if this is smaller than requested length,
            // else return the requested length
            datasize = (sreq->wLength > sizeof(usbModel::deviceDesc)) ? sizeof(usbModel::deviceDesc) : sreq->wLength;

            // Construct a formatted output string
            snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET DEVICE DESCRIPTOR (wLength = %d)\n", name.c_str(), sreq->wLength);

            // Send the response to the GET_DESCRIPTOR (DEVICE) command
            return sendGetResp(sreq, (uint8_t*)&devdesc, datasize, endp, sbuf);
            break;

        case usbModel::CONFIG_DESCRIPTOR_TYPE:

            // Generate an ACK handshake for the SETUP data
            sendPktToHost(usbModel::PID_HSHK_ACK, idle);
            dataPidUpdate(endp);

            // Set returned data size to be the whole of all of the descriptors if this is smaller than requested length,
            // else return the requested length
            datasize = (sreq->wLength > sizeof(configAllDesc)) ? sizeof(configAllDesc) : sreq->wLength;

            // Construct a formatted output string
            snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET CONFIG DESCRIPTOR (wLength = %d)\n", name.c_str(), sreq->wLength);

            // Send the response to the GET_DESCRIPTOR (CONFIG) command
            return sendGetResp(sreq, cfgalldesc.rawbytes, datasize, endp, sbuf);
            break;

        case usbModel::STRING_DESCRIPTOR_TYPE:

            // Generate an ACK handshake for the SETUP data
            sendPktToHost(usbModel::PID_HSHK_ACK, idle);
            dataPidUpdate(endp);

            // Set returned data size to be the whole of all of the descriptors if this is smaller than requested length,
            // else return the requested length
            datasize = (sreq->wLength > strdesc[descidx].bLength) ? strdesc[descidx].bLength : sreq->wLength;

            // Construct a formatted output string
            snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET STRING DESCRIPTOR (wLength = %d)\n", name.c_str(), sreq->wLength);

            // Send the response to the GET_DESCRIPTOR (STRING) command
            return sendGetResp(sreq, (uint8_t*)&strdesc[descidx], datasize, endp, sbuf);
            break;

        case usbModel::IF_DESCRIPTOR_TYPE:

            // Generate an ACK handshake for the SETUP data
            sendPktToHost(usbModel::PID_HSHK_ACK, idle);
            dataPidUpdate(endp);

            break;

        case usbModel::EP_DESCRIPTOR_TYPE:

            // Generate an ACK handshake for the SETUP data
            sendPktToHost(usbModel::PID_HSHK_ACK, idle);
            dataPidUpdate(endp);

            break;

        case usbModel::CS_IF_DESCRIPTOR_TYPE:

            // Generate an ACK handshake for the SETUP data
            sendPktToHost(usbModel::PID_HSHK_ACK, idle);
            dataPidUpdate(endp);

            break;

        default:
            USBERRMSG("handleDevReq: Received unexpected wValue descriptor type (0x%02x)\n", sreq->wValue);

            // Generate an STALL handshake for the SETUP data
            ephalted[usbModel::CONTROL_EP & 0xf][(usbModel::CONTROL_EP >> 7) & 1] = true;
            sendPktToHost(usbModel::PID_HSHK_STALL, idle);
            return usbModel::USBERROR;

            break;
        }

    case usbModel::USB_REQ_SET_DESCRIPTOR:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // Ignoring SET_DESCRIPTOR requests, so do nothing
        USBDISPPKT("  %s RX DEV REQ: SET DESCRIPTOR\n", name.c_str());
        break;

    // Return deviceConfigured state
    case usbModel::USB_REQ_GET_CONFIG:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        datasize = 1;

        // Only one configuration (index at 1), so return false for all other indexes
        cfgstate = ((sreq->wValue & 0xff) == 1) ? deviceConfigured : false;

        // Construct a formatted output string
        snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX DEV REQ: GET DEVICE CONFIGURATION (index=%d)\n", name.c_str(), (sreq->wValue & 0xff));

        return sendGetResp(sreq, (uint8_t*)&cfgstate, datasize, endp, sbuf);

        break;

    case usbModel::USB_REQ_SET_CONFIG:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // Set the configuration if index is 1, clear if index is 0 and leave unchanged for all other indexes
        deviceConfigured = ((sreq->wValue & 0xff) == 1) ? true  :
                           ((sreq->wValue & 0xff) == 0) ? false :
                           deviceConfigured;

        USBDISPPKT("  %s RX DEV REQ: SET CONFIGURATION (index %d)\n", name.c_str(), sreq->wValue & 0xff);

       break;

    default:
        // Generate a STALL handshake if an unknown bRequest
        ephalted[usbModel::CONTROL_EP & 0xf][(usbModel::CONTROL_EP >> 7) & 1] = true;
        sendPktToHost(usbModel::PID_HSHK_STALL, idle);
        break;
    }

    return usbModel::USBOK;
}

//-------------------------------------------------------------
// handleIfReq
//
// Called if bmRequestType is for an intersface and handles each
// valid bRequest command:
//
//  GET STATUS
//  CLEAR FEATURE
//  SET FEATURE
//  GET INTERFACE
//  SET INTERFACE
//
// All other descriptors are treated as an error. The interface
// has no features or alternative interfaces, so ignores sets
// and clears, and returns 0 on gets.
//
//-------------------------------------------------------------

int usbDevice::handleIfReq(const usbModel::setupRequest* sreq, const uint8_t endp, const int idle)
{
    int datasize;
    uint8_t buf[2];

    USBDEVDEBUG("handleIfReq: bRequest=0x%02x\n", sreq->bRequest);

    switch(sreq->bRequest)
    {
    case usbModel::USB_REQ_GET_STATUS:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // No interface statuses, so return 0x0000

        // Construct status data
        buf[0]   = 0;
        buf[1]   = 0;
        datasize = 2;

        // Construct a formatted output string
        snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX IF REQ: GET STATUS\n", name.c_str());

        // Send the response to the GET_STATUS command
        return sendGetResp (sreq, buf, datasize, endp, sbuf);
        break;

    case usbModel::USB_REQ_CLEAR_FEATURE:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // No interface features, so ignore
        USBDISPPKT("  %s RX IF REQ: CLEAR FEATURE (wIndex=0x%04x)\n", name.c_str(), sreq->wIndex);
        break;

    case usbModel::USB_REQ_SET_FEATURE:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // No interface features, so ignore
        USBDISPPKT("  %s RX IF REQ: SET FEATURE (wIndex=0x%04x)\n", name.c_str(), sreq->wIndex);
        break;

    case usbModel::USB_REQ_GET_INTERFACE:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // No alternative interfaces, so return 0x00

        // Construct status data
        buf[0]   = 0;
        datasize = 1;

        // Construct a formatted output string
        snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX IF REQ: GET INTERFACE (wIndex=0x%04x)\n", name.c_str(), sreq->wIndex);

        // Send the response to the GET_STATUS command
        return sendGetResp (sreq, buf, datasize, endp, sbuf);

        break;

    case usbModel::USB_REQ_SET_INTERFACE:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // No alternative interfaces, so ignore

        USBDISPPKT("  %s RX IF REQ: SET INTERFACE (wIndex=0x%04x)\n", name.c_str(), sreq->wIndex);
        break;

    default:
        // Generate a STALL handshake if an unknown bRequest
        sendPktToHost(usbModel::PID_HSHK_STALL, idle);
        break;
    }

    return usbModel::USBOK;
}

//-------------------------------------------------------------
// handleEpReq
//
// Called if bmRequestType is for an endpoint and handles each
// valid bRequest command:
//
//  GET STATUS
//  CLEAR FEATURE
//  SET FEATURE
//  SYNCH FRAME
//
// All other descriptors are treated as an error. The endpoints
// have a halted status for GET STATUS and SET/CLEAR FEATURE.
// The SYNCH FRAME request alwyas generates a 0 value to return.
//
//-------------------------------------------------------------

int usbDevice::handleEpReq(const usbModel::setupRequest* sreq, const uint8_t endp, const int idle)
{
    int datasize;
    uint8_t buf[2];
    int     epidx;
    int     epdir;

    USBDEVDEBUG("handleEpReq: bRequest=0x%02x\n", sreq->bRequest);

    epidx = sreq->wIndex & 0x000f;
    epdir = (sreq->wIndex >> 7) & 0x0001;

    switch(sreq->bRequest)
    {
    case usbModel::USB_REQ_GET_STATUS:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // Send back halted status
        buf[0]   = ephalted[epidx][epdir] ? 1 : 0;
        buf[1]   = 0;

        datasize = 2;

        // Construct a formatted output string
        snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX EP REQ: GET STATUS\n", name.c_str());

        // Send the response to the GET_STATUS command
        return sendGetResp (sreq, buf, datasize, endp, sbuf);

        break;

    case usbModel::USB_REQ_CLEAR_FEATURE:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        USBDISPPKT("  %s RX EP REQ: CLEAR FEATURE (wIndex=0x%04x)\n", name.c_str(), sreq->wIndex);

        // Clear halted status
        ephalted[epidx][epdir] = false;

        break;

    case usbModel::USB_REQ_SET_FEATURE:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // Set halted status
        ephalted[epidx][epdir] = true;

        USBDISPPKT("  %s RX EP REQ: SET FEATURE (wIndex=0x%04x)\n", name.c_str(), sreq->wIndex);
        break;

    case usbModel::USB_REQ_SYNCH_FRAME:

        // Generate an ACK handshake for the SETUP data
        sendPktToHost(usbModel::PID_HSHK_ACK, idle);
        dataPidUpdate(endp);

        // Not suppotred so send back 0
        buf[0]   = 0;
        buf[1]   = 0;

        datasize = 2;

        // Construct a formatted output string
        snprintf(sbuf, usbModel::ERRBUFSIZE,"  %s RX EP REQ: SYNCH FRAME\n", name.c_str());

        // Send the response to the GET_STATUS command
        return sendGetResp (sreq, buf, datasize, endp, sbuf);

        break;

    // Generate a STALL handshake if an unknown or unsupported bRequest
    default:
        ephalted[epidx][epdir] = true;
        sendPktToHost(usbModel::PID_HSHK_STALL, idle);
        break;
    }

    return usbModel::USBOK;
}

//-------------------------------------------------------------
// sendGetResp
//
// Returns a data packet (in data[], length data bytes) to host
// as a response to get commands.
//
// The method will return usbModel::USBERROR if an incorrect
// bmRequestType, or if an error when returning data to the
// host.
//
//-------------------------------------------------------------

int usbDevice::sendGetResp (const usbModel::setupRequest* sreq, const uint8_t data[], const int databytes, const uint8_t endp, const char* fmtstr, const int idle)
{
    int                  pid;
    uint32_t             args[usbModel::MAXNUMARGS];
    int                  datasize;
    int                  numbytes;

    int datasent       = 0;

    USBDEVDEBUG("<== sendGetResp: databytes=%d\n", databytes);

    // Check request type is a "device get" type
    if (sreq->bmRequestType != usbModel::USB_DEV_REQTYPE_GET &&
        sreq->bmRequestType != usbModel::USB_IF_REQTYPE_GET  &&
        sreq->bmRequestType != usbModel::USB_EP_REQTYPE_GET)
    {
        USBERRMSG("getResp: Received unexpected bmRequestType with a GET command (0x%02x)\n", sreq->bmRequestType);
        return usbModel::USBERROR;
    }

    USBDISPPKT("%s", fmtstr);

    if (sendInData(data, databytes, endp, false, idle) != usbModel::USBOK)
    {
        return usbModel::USBERROR;
    }

    return usbModel::USBOK;
}

//-------------------------------------------------------------
// sendInData
//
// Method to transfer data (in data[], length databytes) to the
// host. Breaks data up into chunks of the maximum supported
// size, waithing for IN tokesm sending packet and waiting
// for aknowledgement. Will resend on NAKs up to a limit when
// an error generated. The method can be called after an
// IN token already received, so the intial wait for IN can be
// skipped with skipfirstIN set to true.
//
// The method will return usbModel::USBERROR if an error
// occured when waiting for the IN token or the acknowledgement
// after sending the data, or if it received too many NAKs
// from the host. Otherwise it returns usbModel::USBOK.
//
//-------------------------------------------------------------

int usbDevice::sendInData(const uint8_t data[], const int databytes, const uint8_t endp, bool skipfirstIN, const int idle)
{
    int                  pid;
    uint32_t             args[usbModel::MAXNUMARGS];
    int                  datasize;
    int                  numbytes;
    int                  numnaks  = 0;
    int                  datasent = 0;

    USBDEVDEBUG("<== sendInData: databytes=%d endp=0x%02x, skipfirstIN=%d\n", databytes, endp, skipfirstIN);

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

        // Wait for an IN token (unless skipping on first iteration as one already received)
        if (!skipfirstIN || datasent)
        {
            USBDEVDEBUG ("<== sendInData: waiting for IN token\n");
            if (waitForExpectedPacket(usbModel::PID_TOKEN_IN, pid, args, rxdata, numbytes) != usbModel::USBOK)
            {
                return usbModel::USBERROR;
            }
        }

        USBDEVDEBUG ("<== sendInData: sending IN data to host\n");

        // Send DATAx packet
        sendPktToHost(dataPid(endp), &data[datasent], datasize, idle);

        USBDEVDEBUG ("<== sendInData: waiting for ACK/NAK token\n");

        // Wait for acknowledge (either ACK or NAK)
        if (waitForExpectedPacket(PID_NO_CHECK, pid, args, rxdata, numbytes) != usbModel::USBOK)
        {
            USBERRMSG ("sendInData: unexpected error wait for ACK/NAK\n");
            return usbModel::USBERROR;
        }

        // If ACK then end of transaction if no more bytes
        if (pid == usbModel::PID_HSHK_ACK)
        {
            USBDEVDEBUG("<== sendInData: seen ACK for DATAx\n");

            dataPidUpdate(endp);

            datasent += datasize;

            if ((databytes - datasent) == 0)
            {
                USBDEVDEBUG("<== sendInData: remaining_data = %d\n", databytes - datasent);
                break;
            }
        }
        // Unexpected PID if not a NAK. NAK causes loop to send again, so no action.
        else if (pid == usbModel::PID_HSHK_NAK)
        {
            numnaks++;

            if (numnaks > MAXNAKS)
            {
                USBERRMSG ("sendInData: seen too many NAKs\n");
                return usbModel::USBERROR;
            }
        }
        else
        {
            USBDEVDEBUG("<== sendInData: bad pid (0x%02x) waiting for ACK\n", pid);
            return usbModel::USBERROR;
        }
    }

    return usbModel::USBOK;
}

