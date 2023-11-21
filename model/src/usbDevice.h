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
private:

    //-------------------------------------------------------------
    // Local constant definitions
    //-------------------------------------------------------------

    static const int      PID_NO_CHECK             = usbModel::PID_INVALID;
    static const int      DEFAULT_IDLE             = 36;
    
    static const int      NUMIF0EPS                = 1;
    static const int      NUMIF1EPS                = 2;
    static const int      TOTALNUMEPS              = NUMIF0EPS + NUMIF1EPS;
    static const int      MAXNUMEPS                = 16;
    
    static const uint8_t  REMOTE_WAKEUP_STATE      = usbModel::USB_REMOTE_WAKEUP_OFF;
    static const uint8_t  SELF_POWERED_STATE       = usbModel::USB_NOT_SELF_POWERED;

public:

    //-------------------------------------------------------------
    // Constructor
    //-------------------------------------------------------------

    usbDevice (int nodeIn, std::string name = std::string(FMT_DEVICE "DEV " FMT_NORMAL)) :
        usbPliApi(nodeIn, name),
        usbPkt(name),
        deviceConfigured(false),
        ephalted{{false}},
        epvalid{{true,  false}, {true,  true},  {false, false}, {false, false},
                {false, false}, {false, false}, {false, false}, {false, false},
                {false, true},  {false, false}, {false, false}, {false, false},
                {false, false}, {false, false}, {false, false}, {false, false}}
    {
        strdesc[0].bLength    = 6; // bLength + bDescriptorType bytes plus two wLANGID entries (2 bytes each)
        strdesc[0].bString[0] = 0x0809; // English UK
        strdesc[0].bString[1] = 0x0409; // English US

        strdesc[1].bLength = 2;  // bLength + bDescriptorType bytes
        strdesc[1].bLength += usbModel::fmtStrToUnicode(strdesc[1].bString, "github.com/wyvernSemi");

        strdesc[2].bLength = 2;  // bLength + bDescriptorType bytes
        strdesc[2].bLength += usbModel::fmtStrToUnicode(strdesc[2].bString, "usbModel");
        reset();
    };

    //-------------------------------------------------------------
    // User entry method to start the USB device model
    //-------------------------------------------------------------

    int usbDeviceRun(const int idle = DEFAULT_IDLE);

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
                          epdesc0_0(0x88, 0x03, 0xff),
                          ifdesc1(1, NUMIF1EPS),
                          epdesc1_0(0x81, 02),
                          epdesc1_1(0x01, 02)
        {
        }
    };

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
    void reset()
    {
        usbPliApi::apiReset();
        devaddr   = usbModel::USB_NO_ASSIGNED_ADDR;
    }

    int          sendGetResp           (const usbModel::setupRequest* sreq,
                                        const uint8_t data[], const int databytes,
                                        const char* fmtstr,
                                        const int idle = DEFAULT_IDLE);

    //-------------------------------------------------------------
    // Methods for sending packets back towards the host
    //-------------------------------------------------------------

    int          sendPktToHost         (const int pid, const uint8_t  data[],   unsigned      datalen, const int idle = DEFAULT_IDLE);   // DATA
    int          sendPktToHost         (const int pid, const uint8_t  addr,     const uint8_t endp,    const int idle = DEFAULT_IDLE);   // Token
    int          sendPktToHost         (const int pid, const uint16_t framenum, const int     idle = DEFAULT_IDLE);                      // SOF
    int          sendPktToHost         (const int pid, const int      idle = DEFAULT_IDLE);                                              // Handshake

    //-------------------------------------------------------------
    // Method to wait for the receipt of a particular packet type
    //-------------------------------------------------------------

    int          waitForExpectedPacket (const int pktType, int &pid, uint32_t args[], uint8_t data[], int databytes, bool ignorebadpkts = true);

    //-------------------------------------------------------------
    // Methods for processing different packet types
    //-------------------------------------------------------------

    int          processControl        (const uint32_t addr,   const uint32_t endp,   const int idle = DEFAULT_IDLE);
    int          processIn             (const uint32_t args[], const uint8_t  data[], const int databytes, const int idle = DEFAULT_IDLE);
    int          processOut            (const uint32_t args[],       uint8_t  data[], const int databytes, const int idle = DEFAULT_IDLE);
    int          processSOF            (const uint32_t args[], const int      idle = DEFAULT_IDLE);

    //-------------------------------------------------------------
    // Methods for handling requests
    //-------------------------------------------------------------

    int          handleDevReq          (const usbModel::setupRequest* sreq, const int idle = DEFAULT_IDLE);
    int          handleIfReq           (const usbModel::setupRequest* sreq, const int idle = DEFAULT_IDLE);
    int          handleEpReq           (const usbModel::setupRequest* sreq, const int idle = DEFAULT_IDLE);

    //-------------------------------------------------------------
    // Internal device state
    //-------------------------------------------------------------

    // Assigned device address
    int                    devaddr;
    bool                   deviceConfigured;
    bool                   ephalted[MAXNUMEPS][2];
    bool                   epvalid[MAXNUMEPS][2];

    // Internal buffers for use by class methods
    uint8_t                rxdata [usbModel::MAXBUFSIZE];
    usbModel::usb_signal_t nrzi   [usbModel::MAXBUFSIZE];
    char                   sbuf   [usbModel::ERRBUFSIZE];

    // Device's descriptors
    usbModel::deviceDesc   devdesc;
    usbModel::stringDesc   strdesc[3];
    cfgAllBuf              cfgalldesc;


};