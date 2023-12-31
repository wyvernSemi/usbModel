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

#include "usbCommon.h"
#include "usbPkt.h"
#include "usbPliApi.h"

class usbDevice : public usbPliApi, public usbPkt
{
public:

    //-------------------------------------------------------------
    // Public type definitions
    //-------------------------------------------------------------

    enum dataResponseType_e
    {
        ACK,
        NAK,
        STALL
    };

    typedef dataResponseType_e (*usbDeviceDataCallback_t) (const uint8_t endp, uint8_t* data, int &numbytes);

    //-------------------------------------------------------------
    // Public constant definitions
    //-------------------------------------------------------------

    static const unsigned SLEEP_FOREVER            = 0;

private:

    //-------------------------------------------------------------
    // Local constant definitions
    //-------------------------------------------------------------

    // PID value to not check for an expected PID type
    static const int      PID_NO_CHECK             = usbModel::PID_INVALID;

    // Default idle ticks before responses
    static const int      DEFAULT_IDLE             = 4;

    // Define the number of endpoints for each interface for this device
    static const int      NUMIF0EPS                = 1;
    static const int      NUMIF1EPS                = 2;
    static const int      TOTALNUMEPS              = NUMIF0EPS + NUMIF1EPS;

    // This devices feature set values
    static const uint8_t  REMOTE_WAKEUP_STATE      = usbModel::USB_REMOTE_WAKEUP_OFF;
    static const uint8_t  SELF_POWERED_STATE       = usbModel::USB_NOT_SELF_POWERED;

    // Maximum number of received NAKs before generating an error
    static const int      MAXNAKS                  = 3;

public:

    //-------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------

    usbDevice (int nodeIn, usbDeviceDataCallback_t datacbIn = NULL, std::string name = std::string(FMT_DEVICE "DEV " FMT_NORMAL)) :
        usbPliApi(nodeIn, name),
        usbPkt(name),
        deviceConfigured(false),
        ephalted{{false}},
        epvalid{{true,  false}, {true,  true},  {false, false}, {false, false},
                {false, false}, {false, false}, {false, false}, {false, false},
                {false, true},  {false, false}, {false, false}, {false, false},
                {false, false}, {false, false}, {false, false}, {false, false}},
        framenum(0),
        suspended(false),
        datacb(datacbIn)
    {
        strdesc[0].bLength    = 6; // bLength + bDescriptorType bytes plus two wLANGID entries (2 bytes each)
        strdesc[0].bString[0] = usbModel::LANGID_ENG_UK; // English UK
        strdesc[0].bString[1] = usbModel::LANGID_ENG_US; // English US

        strdesc[1].bLength = 2;  // bLength + bDescriptorType bytes
        strdesc[1].bLength += usbModel::fmtStrToUnicode(strdesc[1].bString, "github.com/wyvernSemi");

        strdesc[2].bLength = 2;  // bLength + bDescriptorType bytes
        strdesc[2].bLength += usbModel::fmtStrToUnicode(strdesc[2].bString, "usbModel");

        reset();
    };

    //-------------------------------------------------------------
    // Get current time
    //-------------------------------------------------------------

    float usbDeviceGetTimeUs()
    {
        unsigned ticks = apiGetClkCount();

        return (float)ticks * 1.0/(float)usbPliApi::ONE_US;
    }

    //-------------------------------------------------------------
    // Device sleep method in microseconds
    //-------------------------------------------------------------

    void usbDeviceSleepUs(const unsigned time_us)
    {
        unsigned ticks = time_us * usbPliApi::ONE_US;

        apiSendIdle(ticks);
    }

    //-------------------------------------------------------------
    // Method to disconnect the device
    //-------------------------------------------------------------

    void usbDeviceDisconnect()
    {
        apiDisablePullup();
    }

    //-------------------------------------------------------------
    // Method to reconnect the device
    //-------------------------------------------------------------

    void usbDeviceReconnect()
    {
        apiEnablePullup();
    }

    //-------------------------------------------------------------
    // User entry method to start the USB device model
    //-------------------------------------------------------------

    int  usbDeviceRun (const int idle = DEFAULT_IDLE);

    //-------------------------------------------------------------
    // End execution of the program
    //-------------------------------------------------------------

    void usbDeviceEndExecution()
    {
        apiHaltSimulation();
    }

private:

    //-------------------------------------------------------------
    // Configuration structure
    //-------------------------------------------------------------

    class configAllDesc
    {
    public:
        struct usbModel::configDesc       cfgdesc0;
        struct usbModel::headerFuncDesc   hdrfuncdesc;
        struct usbModel::acmFuncDesc      acmfuncdesc;
        struct usbModel::unionFuncDesc    unionfuncdesc;
        struct usbModel::callMgmtFuncDesc callmgmtfuncdesc;
        struct usbModel::interfaceDesc    ifdesc0;
        struct usbModel::endpointDesc     epdesc0_0;
        struct usbModel::interfaceDesc    ifdesc1;
        struct usbModel::endpointDesc     epdesc1_0;
        struct usbModel::endpointDesc     epdesc1_1;

        configAllDesc() : cfgdesc0(sizeof(configAllDesc)),
                          ifdesc0(0, NUMIF0EPS),
                          epdesc0_0(0x82, 0x03, 0xff),
                          ifdesc1(1, NUMIF1EPS, 0x0a, 0, 0),
                          epdesc1_0(0x81, 02),
                          epdesc1_1(0x01, 02)
        {
        }
    };

    // Union between configuration structure and a raw bytes array
    union cfgAllBuf
    {
        configAllDesc cfgall;
        uint8_t       rawbytes[sizeof(configAllDesc)];

        cfgAllBuf() : cfgall()
        {
        }
    };

    //-------------------------------------------------------------
    // Reset method, called on detecting a reset state on the line
    //-------------------------------------------------------------

    void reset(void)
    {
        // Call reset method of base classes
        usbPliApi::apiReset();
        usbPkt::reset();

        // Reset the device address to be unassigned
        devaddr   = usbModel::USB_NO_ASSIGNED_ADDR;

        // Set the device to be unconfigured
        deviceConfigured = false;

        // Reset the frame number
        framenum = 0;
        
        // Clear suspension state
        suspended = false;


        for (int edx = 0; edx < usbModel::MAXENDPOINTS; edx++)
        {
            ephalted[edx][0] = false;
            ephalted[edx][1] = false;

            epdata0[edx][0]  = true;
            epdata0[edx][1]  = true;
        }
    }

    //-------------------------------------------------------------
    // Methods for sending packets back towards the host
    //-------------------------------------------------------------

    int          sendGetResp           (const usbModel::setupRequest* sreq,
                                        const uint8_t data[], const int databytes,
                                        const uint8_t endp,   const char* fmtstr,
                                        const int     idle = DEFAULT_IDLE);

    int          sendInData            (const uint8_t data[], const int  databytes,
                                        const uint8_t endp,         bool skipfirstIN = false,
                                        const int     idle = DEFAULT_IDLE);

    int          sendPktToHost         (const int pid, const uint8_t  data[],   unsigned      datalen, const int idle = DEFAULT_IDLE);   // DATA
    int          sendPktToHost         (const int pid, const uint8_t  addr,     const uint8_t endp,    const int idle = DEFAULT_IDLE);   // Token
    int          sendPktToHost         (const int pid, const uint16_t framenum, const int     idle = DEFAULT_IDLE);                      // SOF
    int          sendPktToHost         (const int pid, const int      idle = DEFAULT_IDLE);                                              // Handshake

    int          controlStatusStage    (const bool instatus, const uint8_t endp);

    //-------------------------------------------------------------
    // Method to wait for the receipt of a particular packet type
    //-------------------------------------------------------------

    int          waitForExpectedPacket (const int  pktType, int &pid, uint32_t* args, uint8_t* data, int &databytes,
                                        const bool ignorebadpkts = true, const int timeout = usbModel::NOTIMEOUT);

    //-------------------------------------------------------------
    // Methods for processing different packet types
    //-------------------------------------------------------------

    int          processControl        (const uint32_t addr,   const uint32_t endp,       const int idle = DEFAULT_IDLE);
    int          processIn             (const uint32_t args[],       int      &databytes, const int idle = DEFAULT_IDLE);
    int          processOut            (const uint32_t args[],       uint8_t  data[],     const int databytes, const int idle = DEFAULT_IDLE);
    int          processSOF            (const uint32_t args[], const int      idle = DEFAULT_IDLE);

    //-------------------------------------------------------------
    // Methods for handling requests
    //-------------------------------------------------------------

    int          handleDevReq          (const usbModel::setupRequest* sreq, const uint8_t endp, const int idle = DEFAULT_IDLE);
    int          handleIfReq           (const usbModel::setupRequest* sreq, const uint8_t endp, const int idle = DEFAULT_IDLE);
    int          handleEpReq           (const usbModel::setupRequest* sreq, const uint8_t endp, const int idle = DEFAULT_IDLE);

    //-------------------------------------------------------------
    // Methods for handling endpoint data0/1
    //-------------------------------------------------------------

    inline int   epIdx                 (const int endp) {return endp & 0xf;};
    inline bool  epDirIn               (const int endp) {return (endp >> 7) & 1;};
    inline int   dataPid               (const int endp) {return epdata0[epIdx(endp)][epDirIn(endp)] ? usbModel::PID_DATA_0 : usbModel::PID_DATA_1;};
    inline int   dataPidUpdate         (const int endp, const bool iso = false)
    {
        int dpid = dataPid(endp);
        if (!iso)
        {
            epdata0[epIdx(endp)][epDirIn(endp)] = !epdata0[epIdx(endp)][epDirIn(endp)];
        }

        return dpid;
    }

    //-------------------------------------------------------------
    // Internal device state
    //-------------------------------------------------------------

    // Assigned device address
    int                     devaddr;

    // Device configured status
    bool                    deviceConfigured;

    // Endpoint statuses for each possible endpoints and the directions
    bool                    ephalted [usbModel::MAXENDPOINTS][usbModel::NUMEPDIRS];
    bool                    epvalid  [usbModel::MAXENDPOINTS][usbModel::NUMEPDIRS];
    bool                    epdata0  [usbModel::MAXENDPOINTS][usbModel::NUMEPDIRS];

    // Internal buffers for use by class methods
    uint8_t                 rxdata   [usbModel::MAXBUFSIZE];
    usbModel::usb_signal_t  nrzi     [usbModel::MAXBUFSIZE];
    char                    sbuf     [usbModel::ERRBUFSIZE];

    // Device's descriptors
    usbModel::deviceDesc    devdesc;
    usbModel::stringDesc    strdesc[3];
    cfgAllBuf               cfgalldesc;

    // Data callback function pointer
    usbDeviceDataCallback_t datacb;

    // Last SOF frame number
    uint16_t                framenum;
    
    bool                    suspended;


};