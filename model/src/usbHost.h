//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 9th November 2023
//
// Contains the code for the usbModel host class header
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

#ifndef _USB_HOST_H_
#define _USB_HOST_H_

#include <cstring>
#include <cmath>

#include "usbCommon.h"
#include "usbPkt.h"
#include "usbPliApi.h"

class usbHost : public usbPliApi, public usbPkt
{
public:

    static const int      PID_NO_CHECK             = usbModel::PID_INVALID;
    static const int      DEFAULTIDLEDELAY         = 4; // 0.33us at 12MHz
    static const int      MAXNAKS                  = 3;

    // ----------------------------------------------------------
    // Constructor
    // ----------------------------------------------------------

    usbHost (int nodeIn, std::string name = std::string(FMT_HOST "HOST" FMT_NORMAL)) :
        usbPliApi(nodeIn, name),
        usbPkt(name),
        connected(false),
        keepalive(true),
        framenum(0),
        epdata0{{true, true}, {true, true}, {true, true}, {true, true},
                {true, true}, {true, true}, {true, true}, {true, true},
                {true, true}, {true, true}, {true, true}, {true, true},
                {true, true}, {true, true}, {true, true}, {true, true}}
    {
    }

    // -------------------------------------------------------------------------
    // Public methods
    // -------------------------------------------------------------------------
public:

    // ----------------------------------------------------------
    // Device sleep method in microseconds
    // ----------------------------------------------------------

    void usbHostSleepUs(const unsigned time_us)
    {
        unsigned ticks         = time_us * usbPliApi::ONE_US;
        unsigned maxidlechunks = usbPliApi::ONE_US;

        // Break up idle into chunks to ensure an SOF is sent
        // within spec if delay argument is large
        do
        {
            // Check if an SOF is needed (if enabled)
            checkSof();

            if (ticks >= maxidlechunks)
            {
                apiSendIdle(maxidlechunks);
                ticks -= maxidlechunks;
            }
            else
            {
                apiSendIdle(ticks);
                ticks = 0;
            }
        }
        while(ticks);
    }

    // ----------------------------------------------------------
    // Get current time
    // ----------------------------------------------------------

    float usbHostGetTimeUs()
    {
        unsigned ticks = apiGetClkCount();
        float timeus = (float)ticks / (float)usbPliApi::ONE_US;

        return timeus;
    }

    // ----------------------------------------------------------
    // End execution of the program
    // ----------------------------------------------------------

    void usbHostEndExecution()
    {
        apiHaltSimulation();
    }

    // ----------------------------------------------------------
    // Wait for a connection on the line
    // ----------------------------------------------------------

    int  usbHostWaitForConnection     (const unsigned polldelay = 10*usbPliApi::ONE_US,
                                       const unsigned timeout   =  3*usbPliApi::ONE_MS);

    // ----------------------------------------------------------
    // Device control methods
    // ----------------------------------------------------------

    int  usbHostSetDeviceAddress      (const uint8_t  addr,      const uint8_t  endp,
                                       const uint16_t devaddr,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetDeviceStatus       (const uint8_t  addr,      const uint8_t  endp,
                                             uint16_t &status,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetDeviceDescriptor   (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t  data[],    const uint16_t reqlen,
                                             uint16_t &rxlen,    const bool     chklen = true,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetDeviceConfig       (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t  &cfgstate,
                                       const uint8_t  index = 1,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetDeviceConfig       (const uint8_t  addr,      const uint8_t  endp,
                                       const uint8_t  index,
                                       const unsigned idle = DEFAULTIDLEDELAY);


    int  usbHostClearDeviceFeature    (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetDeviceFeature      (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t  feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetConfigDescriptor   (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t  data[],    const uint16_t reqlen,
                                             uint16_t &rxlen,    const bool     chklen = true,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetStrDescriptor      (const uint8_t  addr,      const uint8_t  endp,
                                       const uint8_t  stridx,          uint8_t  data[],
                                       const uint16_t reqlen,          uint16_t &rxlen,
                                       const bool     chklen = true,
                                       const uint16_t langid = usbModel::LANGID_ENG_UK,
                                       const unsigned idle   = DEFAULTIDLEDELAY);

    // ----------------------------------------------------------
    // Interface control methods
    // ----------------------------------------------------------

    int  usbHostGetInterfaceStatus    (const uint8_t  addr,      const uint8_t  endp,
                                       const uint16_t ifidx,           uint16_t &status,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostClearInterfaceFeature (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetInterfaceFeature   (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetInterface          (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t index,           uint8_t &altif,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetInterface          (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t index,     const uint8_t altif,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    // ----------------------------------------------------------
    // Endpoint control methods
    // ----------------------------------------------------------

    int  usbHostGetEndpointStatus     (const uint8_t  addr,      const uint8_t  endp,
                                             uint16_t &status,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostClearEndpointFeature  (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostSetEndpointFeature    (const uint8_t  addr,      const uint8_t endp,
                                       const uint16_t feature,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostGetEndpointSynchFrame (const uint8_t  addr,      const uint8_t  endp,
                                             uint16_t &framenum,
                                       const unsigned idle = DEFAULTIDLEDELAY);
    // ----------------------------------------------------------
    // Data transfer methods
    // ----------------------------------------------------------
    
    int  usbHostBulkDataOut           (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t  data[],    const int      len, const int maxpktsize,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostBulkDataIn            (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t* data,      const int      len, const int maxpktsize,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  usbHostIsoDataOut            (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t  data[],    const int      len, const int maxpktsize,
                                       const unsigned idle = DEFAULTIDLEDELAY);
                                       
    int  usbHostIsoDataIn             (const uint8_t  addr,      const uint8_t  endp,
                                             uint8_t* data,      const int      len, const int maxpktsize,
                                       const unsigned idle = DEFAULTIDLEDELAY);
                                       
    // ----------------------------------------------------------
    // Line control
    // ----------------------------------------------------------
    
    void usbHostSuspendDevice         (void) { apiSendIdle(MINSUSPENDCOUNT); }
                                      
    void usbHostResetDevice           (void) { apiSendReset(MINRSTCOUNT); }

    // -------------------------------------------------------------------------
    // Private methods
    // -------------------------------------------------------------------------

private:
    void sendTokenToDevice            (const int      pid,        const uint8_t  addr,    const uint8_t  endp,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    void sendSofToDevice              (const int      pid,       const uint16_t framenum,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  sendDataToDevice             (const int      datatype,   const uint8_t data[],
                                       const int      len,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  getDataFromDevice            (const int      expPID,           uint8_t  data[],
                                             int      &databytes, const bool     noack = false,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  sendStandardRequest          (const uint8_t  addr,      const uint8_t  endp,
                                       const uint8_t  reqtype,   const uint8_t  request,
                                       const uint16_t value = 0, const uint16_t index = 0, const uint16_t length = 0,
                                       const unsigned idle  = DEFAULTIDLEDELAY);

    int  sendDataOut                  (const uint8_t  addr,       const uint8_t  endp,
                                             uint8_t  data[],     const int      len,
                                       const int      maxpktsize, const bool     isochronous,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  getDataIn                    (const uint8_t  addr,       const uint8_t  endp,
                                             uint8_t* data,       const int      len,
                                       const int      maxpktsize, const bool     isochronous,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    int  getStatus                    (const uint8_t  addr,       const uint8_t  endp,
                                       const uint8_t  type,             uint16_t &status,
                                       const uint16_t wValue = 0, const uint16_t wIndex = 0,
                                       const unsigned idle = DEFAULTIDLEDELAY);

    void checkSof                     (const unsigned idle = DEFAULTIDLEDELAY);
    bool checkConnected               (void);

    inline int  epIdx                 (const int endp) {return endp & 0xf;};
    inline bool epDirIn               (const int endp) {return (endp >> 7) & 1;};
    inline int  dataPid               (const int endp) {return epdata0[epIdx(endp)][epDirIn(endp)] ? usbModel::PID_DATA_0 : usbModel::PID_DATA_1;};
    inline int  dataPidUpdate         (const int endp, const bool iso = false)
    {
        int dpid = dataPid(endp);
        if (!iso)
        {
            epdata0[epIdx(endp)][epDirIn(endp)] = !epdata0[epIdx(endp)][epDirIn(endp)];
        }

        return dpid;
    }

    // -------------------------------------------------------------------------
    // Internal private state
    // -------------------------------------------------------------------------
private:
    // Internal buffers for use by class methods
    usbModel::usb_signal_t nrzi   [usbModel::MAXBUFSIZE];
    uint8_t                rxdata [usbModel::MAXBUFSIZE];
    char                   sbuf   [usbModel::ERRBUFSIZE];

    bool                   connected;
    bool                   keepalive;
    uint64_t               framenum;

    bool                   epdata0[usbModel::MAXENDPOINTS][usbModel::NUMEPDIRS];

};


#endif