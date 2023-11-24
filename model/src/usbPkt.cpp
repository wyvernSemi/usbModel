//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 19th October 2023
//
// Contains the code for the USB raw packet encoding
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

#include "usbPkt.h"

// -------------------------------------------------------------------------
// bitrev()
//
// An efficient bit reverse, up to 32 bits.
//
// -------------------------------------------------------------------------

uint32_t usbPkt::bitrev(const uint32_t Data, const int bits)
{
    unsigned long result = Data;
    int i;

    // Compare each of the bottom bits with reflected top bits
    for (i = 0; i < bits/2; i++)
    {
        // If top and bottom bits different, invert both bits
        if (((result >> (bits-1-i*2)) ^ result) & (1 << i))
        {
            result ^= (1 << i) | (1 << (bits-1-i));
        }
    }

    return result & ((1 << bits) - 1);
}

// -------------------------------------------------------------------------
// usbcrc16
//
// Sixteen bit CRC generation for polynomial:
//   crc16 = x^16 + x^15 + x^2 + 1
//
// Data for CRC is pass in as a usb_signal_t, but only dp used, with
// the length of the data (in bytes) specified with len. The
// length is expected to be byte aligned. The CRC is initialised with
// crcinit before calculation. The result is bit reversed
// before returning value.
//
// -------------------------------------------------------------------------

int usbPkt::usbcrc16 (const usbModel::usb_signal_t data[], const unsigned len, const unsigned crcinit)
{
    unsigned crc = crcinit;

    for (unsigned byte = 0; byte < len; byte++)
    {
        for (int i = 0; i < 8; i++)
        {
            crc = (crc << 1UL) ^ ((((crc & usbModel::BIT16) ? 1 : 0) ^ ((data[byte].dp >> i) & 1)) ? usbModel::POLY16 : 0);
        }
    }

    return bitrev(~crc & 0xffff, 16);
}

// -------------------------------------------------------------------------
// usbcrc5
//
// Five bit CRC generation for polynomial:
//   crc5  =  x^5 +  x^2 + 1
//
// Data for CRC is pass in as a usb_signal_t, but only dp used, with
// the length of the data (in bytes) specified with len. The
// data may not be byte aligned, so the number of trailing bits is 
// specified in endbits. The CRC is initialised with crcinit before
// calculation. The result is bit reversed before returning value.
//
// -------------------------------------------------------------------------

int usbPkt::usbcrc5(const usbModel::usb_signal_t data[], const unsigned len, const int endbits, const unsigned crcinit )
{
    unsigned crc = crcinit;

    for (unsigned bytes = 0; bytes < len; bytes++)
    {
        int bits = (bytes == (len - 1)) ? endbits : 8;
        for (int i = 0; i < bits; i++)
        {
            crc = (crc << 1UL) ^ ((((crc & usbModel::BIT5) ? 1 : 0) ^ ((data[bytes].dp >> i) & 1)) ? usbModel::POLY16 : 0);
        }
    }

    return bitrev(~crc & 0x1f, 5);
}

// -------------------------------------------------------------------------
// nrziEnc
//
// NRZI encoding of raw byte data (raw[]), with result placed in nrzi[].
// The length (in bytes) of data to encode is specified in len, and the
// state of the line (J or K) prior to the start of this encoding is
// given in start. Bit stuffing is performed by inserting a virtual 0 in the
// input data when 6 consecutive 1s are seen.
// 
// The method returns the number of bits generated in the encoding.
// 
// -------------------------------------------------------------------------

int usbPkt::nrziEnc(const usbModel::usb_signal_t raw[], usbModel::usb_signal_t nrzi[], const unsigned len, const int start)
{
    int      state = start;
    uint32_t outputp = 0;
    uint32_t outputm = 0;
    int      onescnt = 0;
    int      obit    = 0;
    int      bitcnt  = 0;
    int      obyte   = 0;

    // Run through each byte in the buffer
    for (unsigned byte = 0; byte < len; byte++)
    {

        // Run through each bit in the byte
        for (int bit = 0; bit < usbModel::NRZI_BITSPERBYTE; bit++)
        {
            // A zero is a state change
            if (!(raw[byte].dp & (usbModel::NRZI_BYTELSBMASK << bit)))
            {
                state = ~state & usbModel::NRZI_BYTELSBMASK;
                onescnt = 0;
            }
            // A one is no state change
            else
            {
                // Keep count of the number of consecutive 1s
                onescnt++;
            }

            // Output NRZI bit
            outputp |= ( state & usbModel::NRZI_BYTELSBMASK) << obit;
            outputm |= (~state & usbModel::NRZI_BYTELSBMASK) << obit;
            obit++;
            bitcnt++;

            // If seen 6 ones, stuff an extra 0 in the output (state change)
            if (onescnt == usbModel::MAXONESLENGTH)
            {
                USBDEVDEBUG("==> nrziEnc: stuffing bit (%d)\n", obit);
                state = ~state;
                onescnt = 0;

                outputp |= ( state & usbModel::NRZI_BYTELSBMASK) << obit;
                outputm |= (~state & usbModel::NRZI_BYTELSBMASK) << obit;
                obit++;
                bitcnt++;
            }

            // If output shift has a whole byte or more, send to output
            while (obit >= 8)
            {
                nrzi[obyte].dp = (uint8_t)outputp;
                nrzi[obyte].dm = (uint8_t)outputm;
                obyte++;

                // Remember any residue bits
                outputp >>= 8;
                outputm >>= 8;
                obit -= 8;
            }
        }
    }

    // Add EOP
    outputp |= usbModel::SE0P << obit;
    outputm |= usbModel::SE0M << obit;
    obit += 3;
    bitcnt += 3;

    // Flush any residue bits out
    while (obit > 0)
    {
        nrzi[obyte].dp = (uint8_t)outputp;
        nrzi[obyte].dm = (uint8_t)outputm;
        obyte++;
        outputp >>= 8;
        outputm >>= 8;
        obit -= 8;
    }

    return bitcnt;
}

// -------------------------------------------------------------------------
// nrziDec
//
// Decodes NRZI data from nrzi[] and places decoded bytes in raw[]. The
// state of the line before the decode is specified in start. The method
// will remove stuffed bits (the bit following 6 consectived decoded
// 1s).
//
// The method checks errors on the input for:
//
//   SE1
//   Bad SE0
//   Bad EOP sequence
//
// The bit count of the decoded data is returned if there are no errors,
// else usbModel::USBERROR is returned.
//
// -------------------------------------------------------------------------

int usbPkt::nrziDec(const usbModel::usb_signal_t nrzi[], usbModel::usb_signal_t raw[], const int start)
{
    int      ibyte     = 0;
    int      eofactive = 0;
    int      bitcount  = 0;
    uint8_t  lastbit   = start;

    int      obyte     = 0;
    int      obit      = 0;
    uint32_t output    = 0;
    int      onecnt    = 0;

    usbModel::usb_signal_t currbit;

    while (true)
    {
        for (int bit = 0; bit < 8; bit++)
        {
            currbit.dp      = (nrzi[ibyte].dp >> bit) & 1;
            currbit.dm      = (nrzi[ibyte].dm >> bit) & 1;

            uint8_t se      = (currbit.dp == currbit.dm);

            // Always an error for SE1
            if (se && currbit.dp)
            {
                USBERRMSG("nrziDec: seen SE1\n");
                return usbModel::USBERROR;
            }

            // If receiving a potential EOP...
            if (eofactive)
            {
                // If seen one SE0, check this bit is also SE0
                if (eofactive == 1 && !se)
                {
                    USBERRMSG("nrziDec: Bad EOP. SE0 not followed by another SE0\n");
                    return usbModel::USBERROR;
                }

                // If seen two SE0s, check this bit is a J
                if (eofactive == 2)
                {
                    // If current bit is a J, then flush any remaining output bits and return bit count
                    if (currbit.dp & !currbit.dm)
                    {
                        // Residue bits to flush
                        // TODO: check if flushing needed. Maybe an error not to be byte aligned
                        if (obit)
                        {
                            raw[obyte].dp = output & 0xff;
                            bitcount += obit;
                        }

                        return bitcount;
                    }
                    // Anything other than a J at this point is an error
                    else
                    {
                        USBERRMSG("nrziDec: Bad EOP. two SE0s not followed by a J (D+ = %d D- = %d)\n", currbit.dp, currbit.dm);
                        return usbModel::USBERROR;
                    }
                }
            }

            // If SE0 increment EOF active state
            if (se && !currbit.dp)
            {
                eofactive++;
            }

            // If not processing an EOF...
            if (!eofactive)
            {
                if (onecnt < usbModel::MAXONESLENGTH)
                {
                    // Add decoded current bit to output: change = 0, same = 1
                    output |= ((lastbit ^ currbit.dp) ? 0 : 1) << obit;

                    // Increment count of bits on output
                    obit++;
                    bitcount++;
                }

                // If a 0, reset the onecnt
                if (lastbit ^ currbit.dp)
                {
                    onecnt = 0;
                }
                else
                {
                    onecnt++;
                }

                // If enough bits for a byte on the output, place in the output buffer
                while (obit >= 8)
                {
                    raw[obyte].dp = output & 0xff;
                    output >>= 8;
                    obit -= 8;
                    obyte++;
                }

                lastbit = currbit.dp;
            }
        }

        ibyte++;
    }
    
    // Should never reach here.
    return usbModel::USBERROR;
}

// -------------------------------------------------------------------------
// usbPktGen (for handshake/preamble)
// 
// Generates a handshake or preamble token packet, as specified by pid,
// and places it in buf[]. It will return usbModel::USBERROR if the
// pid is not a valid type for this packet, or if NRZI encoding
// failed.
//
// -------------------------------------------------------------------------

int usbPkt::usbPktGen(usbModel::usb_signal_t buf[], const int pid)
{
    int idx = 0;

    // Validate PID
    switch (pid)
    {
    case usbModel::PID_HSHK_ACK:
    case usbModel::PID_HSHK_NAK:
    case usbModel::PID_HSHK_NYET:
    case usbModel::PID_HSHK_STALL:
    case usbModel::PID_SPCL_PREAMB:
        break;
    default:
        USBERRMSG("genUsbPkt: Bad PID (0x%x) seen for handshake generation.\n", pid);
        return usbModel::USBERROR;
    }

    // SOP/Sync
    rawbuf[idx].dp = usbModel::SYNC;
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // PID
    rawbuf[idx].dp = pid | ((~pid & 0xf) << 4);
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // NRZI encode with bit stuffing and EOP
    return nrziEnc(rawbuf, buf, idx);
}

// -------------------------------------------------------------------------
// usbPktGen (for SOF)
// 
// Generates a token packet (not SOF), as specified by pid,
// and places it in buf[]. It will return usbModel::USBERROR if the
// pid is not a valid type for this packet, or if NRZI encoding
// failed.
//
// -------------------------------------------------------------------------

int usbPkt::usbPktGen(usbModel::usb_signal_t buf[], const int pid, const uint8_t addr, const uint8_t endp)
{
    int idx = 0;
    unsigned crc;

    // Validate PID for this type of packet
    switch (pid)
    {
    case usbModel::PID_TOKEN_IN:
    case usbModel::PID_TOKEN_OUT:
    case usbModel::PID_TOKEN_SETUP:
        break;
    default:
        USBERRMSG("genUsbPkt: Bad PID (0x%x) seen for token generation.\n", pid);
        return usbModel::USBERROR;
    }

    // Validate the address (0 to 127)
    if (addr > usbModel::MAXDEVADDR)
    {
        USBERRMSG("genUsbPkt: Invalid token address (0x%x\n", addr);
        return usbModel::USBERROR;
    }

    // Validate the endpoint
    if (endp > usbModel::MAXENDP)
    {
        USBERRMSG("genUsbPkt: Invalid token end point (0x%x\n", endp);
        return usbModel::USBERROR;
    }

    // SOP/Sync
    rawbuf[idx].dp = usbModel::SYNC;
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // PID
    rawbuf[idx].dp = pid | ((~pid & 0xf) << 4);
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // Payload
    rawbuf[idx].dp = (addr & 0x7f) | ((endp & 0x1) << 7);
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    rawbuf[idx].dp   = endp >> 1;

    // CRC5 over ADDR and ENDP
    crc = usbcrc5(&rawbuf[idx - 1], 2, 3);

    rawbuf[idx].dp |= crc << 3;
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // NRZI encode with bit stuffing and EOP
    return nrziEnc(rawbuf, buf, idx);
}

// -------------------------------------------------------------------------
// usbPktGen (for SOF token)
// 
// Generates an AOF token packet, as specified by pid,
// and places it in buf[]. It will return usbModel::USBERROR if the
// pid is not a valid type for this packet, or if NRZI encoding
// failed.
//
// -------------------------------------------------------------------------

int usbPkt::usbPktGen(usbModel::usb_signal_t buf[], const int pid, const uint16_t framenum)
{
    int idx = 0;
    unsigned crc;

    // Validate PID for this type of packet
    switch (pid)
    {
    case usbModel::PID_TOKEN_SOF:
        break;
    default:
        USBERRMSG("genUsbPkt: Bad PID (0x%x) seen for SOF generation.\n", pid);
        return usbModel::USBERROR;
    }

    // Validate frame number
    if (framenum > usbModel::MAXFRAMENUM)
    {
        USBERRMSG("genUsbPkt: Ivalid frame number (%d)\n", framenum);
        return usbModel::USBERROR;
    }

    // SOP/Sync
    rawbuf[idx].dp = usbModel::SYNC;
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // PID
    rawbuf[idx].dp = pid | ((~pid & 0xf) << 4);
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // Payload
    rawbuf[idx].dp = framenum & 0xff;
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    rawbuf[idx].dp = (framenum >> 8) & 0x7;

    // CRC5 over frame number
    crc = usbcrc5(&rawbuf[idx - 1], 2, 3);

    rawbuf[idx].dp |= crc << 3;
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // NRZI encode with bit stuffing and EOP
    return nrziEnc(rawbuf, buf, idx);
}

// -------------------------------------------------------------------------
// usbPktGen (for DATAx)
// 
// Generates a DATAx packet, as specified by pid,
// and places it in buf[]. It will return usbModel::USBERROR if the
// pid is not a valid type for this packet, or if NRZI encoding
// failed.
//
// -------------------------------------------------------------------------

int usbPkt::usbPktGen(usbModel::usb_signal_t buf[], const int pid, const uint8_t data[], const unsigned len)
{
    int idx = 0;

    USBDEVDEBUG("==> genUsbPkt: pid=0x%x len=%d\n", pid, len);

    // Validate PID for this type of packet
    switch (pid)
    {
    case usbModel::PID_DATA_0:
    case usbModel::PID_DATA_1:
    case usbModel::PID_DATA_2:
    case usbModel::PID_DATA_M:
        break;
    default:
        USBERRMSG("genUsbPkt: Bad PID (0x%x) seen for data packet generation.\n", pid);
        return usbModel::USBERROR;
    }

    // Validate length
    switch (currspeed)
    {
    case usbModel::usb_speed_e::LS:
        if (len > 8)
        {
            USBERRMSG("genUsbPkt: Invalid data length for low speed (%d).\n", len);
            return usbModel::USBERROR;
        }
        break;
    case usbModel::usb_speed_e::FS:
        if (len > 64)
        {
            USBERRMSG("genUsbPkt: Invalid data length for fast speed (%d).\n", len);
            return usbModel::USBERROR;
        }
        break;
    case usbModel::usb_speed_e::HS:
        if (len > 512)
        {
            USBERRMSG("genUsbPkt: Invalid data length for high speed (%d).\n", len);
            return usbModel::USBERROR;
        }
        break;
    }

    // SOP/Sync
    rawbuf[idx].dp = usbModel::SYNC;
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // PID
    rawbuf[idx].dp = pid | ((~pid & 0xf) << 4);
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    // Payload
    for (unsigned byte = 0; byte < len; byte++)
    {
        rawbuf[idx].dp = data[byte];
        rawbuf[idx].dm = ~rawbuf[idx].dp;
        idx++;
    }

    // CRC16 over data
    unsigned crc = usbcrc16(&rawbuf[2], len);

    USBDEVDEBUG("    ");
    for (int i = 0; i < len; i++)
        USBDEVDEBUG("%02x ", rawbuf[i+2].dp);
    USBDEVDEBUG("\n    crc=0x%04x\n", crc);

    rawbuf[idx].dp = crc & 0xff;
    rawbuf[idx].dm = ~rawbuf[idx].dp;
    idx++;

    rawbuf[idx].dp = (crc >> 8) & 0xff;
    rawbuf[idx].dm = ~buf[idx].dp;
    idx++;

    // NRZI encode with bit stuffing and EOP
    return nrziEnc(rawbuf, buf, idx);
}


// -------------------------------------------------------------------------
// usbPktDecode
//
// Decodes a received NRZI packet (in nrzibuf[]), returing the PID in pid,
// and other extracted values in args[] (the number of arguments dependant
// on the packet type. The data (if any) is placed in data[] and the length
// (in bytes) of this data returned in databytes.
// 
// The method returns usbModel::USBERROR if NRZI decoding fails, if
// the PID field upper bits not inverse of lower bits, CRC checks fail,
// or the PID is invalid. If the PID is valid but not yet supported, then
// usbModel::USBUNSUPPORTED is returned. If decoding was error free then
// usbModel::USBOK is returned. 
//  
// -------------------------------------------------------------------------

int usbPkt::usbPktDecode(const usbModel::usb_signal_t nrzibuf[], int& pid, uint32_t args[], uint8_t data[], int &databytes)
{
    int crc;
    int idx;
    
    // Default data length is zero
    databytes = 0;

    // NRZI decode
    int bitcnt = nrziDec(nrzibuf, rawbuf);

    if (bitcnt < usbModel::MINPKTSIZEBITS)
    {
        USBERRMSG("%sdecodePkt: Invalid bit count returned from nrziDec (%d).\n", errbuf, bitcnt);

        return usbModel::USBERROR;
    }

    // Extract PID
    pid = rawbuf[usbModel::PIDBYTEOFFSET].dp & 0xf;
    uint8_t pidchk = (~rawbuf[usbModel::PIDBYTEOFFSET].dp >> 4) & 0xf;

    // Check PID inverse is in top bits
    if (pid != pidchk)
    {
        USBERRMSG("decodePkt: Invalid PID. Top nibble is not the inverse of bottom nibble (0x%02x).\n", rawbuf[usbModel::PIDBYTEOFFSET].dp);
        return usbModel::USBERROR;
    }

    switch (pid)
    {
    case usbModel::PID_HSHK_ACK:
        USBDISPPKT("  %s RX HNDSHK:  ACK\n", name.c_str());
        break;
    case usbModel::PID_HSHK_NAK:
        USBDISPPKT("  %s RX HNDSHK:  NAK\n", name.c_str());
        break;
    case usbModel::PID_HSHK_STALL:
        USBDISPPKT("  %s RX HNDSHK:  STALL\n", name.c_str());
        break;
    case usbModel::PID_HSHK_NYET:
        USBDISPPKT("  %s RX HNDSHK:  NYET\n", name.c_str());
        break;

    case usbModel::PID_TOKEN_OUT:
    case usbModel::PID_TOKEN_IN:
    case usbModel::PID_TOKEN_SETUP:
        args[usbModel::ARGADDRIDX]    = rawbuf[usbModel::ADDRBYTEOFFSET].dp & 0x7f;                                                 // Address
        args[usbModel::ARGENDPIDX]    = (rawbuf[usbModel::ENDPBYTEOFFSET].dp >> 7) | ((rawbuf[usbModel::ENDPBYTEOFFSET+1].dp & 0x7) << 1);    // ENDP
        args[usbModel::ARGTKNCRC5IDX] = rawbuf[usbModel::CRC5BYTEOFFSET].dp >> 3;                                                   // CRC5

        crc = usbcrc5(&rawbuf[usbModel::ADDRBYTEOFFSET], 2, 3);

        if (args[usbModel::ARGTKNCRC5IDX] != crc)
        {
            USBERRMSG("decodePkt: Bad CRC5 for token. Got 0x%x, expected 0x%x.\n",
                args[usbModel::ARGTKNCRC5IDX], crc);
            return usbModel::USBERROR;
        }
        else if (pid == usbModel::PID_TOKEN_OUT)
        {
            USBDISPPKT("  %s RX TOKEN:   OUT\n    " FMT_DATA_GREY "addr=%d endp=%d" FMT_NORMAL "\n",
                name.c_str(), args[usbModel::ARGADDRIDX], args[usbModel::ARGENDPIDX]);
        }
        else if (pid == usbModel::PID_TOKEN_IN)
        {
            USBDISPPKT("  %s RX TOKEN:   IN\n    " FMT_DATA_GREY "addr=%d endp=%d" FMT_NORMAL "\n",
                name.c_str(), args[usbModel::ARGADDRIDX], args[usbModel::ARGENDPIDX]);
        }
        else
        {
            USBDISPPKT("  %s RX TOKEN:   SETUP\n    " FMT_DATA_GREY "addr=%d endp=%d" FMT_NORMAL "\n",
                name.c_str(), args[usbModel::ARGADDRIDX], args[usbModel::ARGENDPIDX]);
        }
        break;

    case usbModel::PID_TOKEN_SOF:
        args[usbModel::ARGFRAMEIDX]   = rawbuf[usbModel::FRAMEBYTEOFFSET].dp | (rawbuf[usbModel::FRAMEBYTEOFFSET+1].dp & 0x7) << 8;           // Frame number
        args[usbModel::ARGSOFCRC5IDX] = rawbuf[usbModel::CRC5BYTEOFFSET].dp >> 3;                                                   // CRC5

        crc = usbcrc5(&rawbuf[usbModel::FRAMEBYTEOFFSET], 2, 3);

        if (args[usbModel::ARGSOFCRC5IDX] != crc)
        {
            USBERRMSG("decodePkt: Bad CRC5 for SOF. Got 0x%x, expected 0x%x.\n", args[usbModel::ARGTKNCRC5IDX], crc);
            return usbModel::USBERROR;
        }

        USBDISPPKT("  %s RX TOKEN:   SOF\n    " FMT_DATA_GREY "frame=%d" FMT_NORMAL "\n",
            name.c_str(), args[usbModel::ARGFRAMEIDX]);

        break;

    case usbModel::PID_DATA_0:
    case usbModel::PID_DATA_1:
        // Calulate the size of the data packet payload (total size in bytes minus SYNC, PID and CRC16)
        databytes = (bitcnt / 8) - usbModel::DATABYTEOFFSET - 2;

        // Extract CRC16
        args[usbModel::ARGCRC16IDX] = rawbuf[databytes + usbModel::DATABYTEOFFSET].dp | (rawbuf[databytes + usbModel::DATABYTEOFFSET + 1].dp << 8); // CRC16

        // Calculate CRC16
        crc = usbcrc16(&rawbuf[usbModel::DATABYTEOFFSET], databytes);

        // Check CRCs match
        if (crc != args[usbModel::ARGCRC16IDX])
        {
            USBERRMSG("decodePkt: Bad CRC16 for data packet. Got 0x%04x, expected 0x%04x.\n", args[usbModel::ARGCRC16IDX], crc);
            
            USBDEVDEBUG("    \n");
            for (int i = 0; i < databytes+2; i++)
                USBDEVDEBUG("%02x ", rawbuf[usbModel::DATABYTEOFFSET+i].dp);
            USBDEVDEBUG("\n");
            
            return usbModel::USBERROR;
        }

        // Copy validated memory to output buffer
        USBDISPPKT("  %s RX DATA:    %s", name.c_str(), pid == usbModel::PID_DATA_0 ? "DATA0" : "DATA1");

        for (idx = 0; idx < databytes; idx++)
        {
            data[idx] = rawbuf[usbModel::DATABYTEOFFSET+idx].dp;

            if ((idx % 16) == 0)
            {
                USBDISPPKT(FMT_DATA_GREY "\n   ");
            }
            USBDISPPKT(" %02x", data[idx]);
        }

        if (idx % 16 != 1)
        {
            USBDISPPKT(FMT_NORMAL "\n");
        }
        else
        {
            USBDISPPKT(FMT_NORMAL);
        }

        break;

    // Unsupported
    case usbModel::PID_TOKEN_ERR:
    case usbModel::PID_TOKEN_SPLIT:
    case usbModel::PID_TOKEN_PING:
    case usbModel::PID_DATA_2:
    case usbModel::PID_DATA_M:
        USBERRMSG("decodePkt: Unsupported packet type (0x%x)\n", pid);
        return usbModel::USBUNSUPPORTED;
        break;

    default:
        USBERRMSG("decodePkt: Unrecognised packet type (0x%x)\n", pid);
        return usbModel::USBERROR;
        break;
    }

    return usbModel::USBOK;
}
