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

// crc5  =  x^5 +  x^2 + 1
// crc16 = x^16 + x^15 + x^2 + 1

#include <cstring>
#include <stdio.h>
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
// -------------------------------------------------------------------------

int usbPkt::usbcrc16(const usb_signal_t data[], const unsigned len, const unsigned crcinit)
{
    unsigned crc = crcinit;

    for (unsigned byte = 0; byte < len; byte++)
    {
        for (int i = 0; i < 8; i++)
        {
            crc = (crc << 1UL) ^ ((((crc & BIT16) ? 1 : 0) ^ ((data[byte].dp >> i) & 1)) ? POLY16 : 0);
        }
    }

    return bitrev(~crc & 0xffff, 16);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

int usbPkt::usbcrc5(const usb_signal_t data[], const unsigned len, const int endbits, const unsigned crcinit )
{
    unsigned crc = crcinit;

    for (unsigned bytes = 0; bytes < len; bytes++)
    {
        int bits = (bytes == (len - 1)) ? endbits : 8;
        for (int i = 0; i < bits; i++)
        {
            crc = (crc << 1UL) ^ ((((crc & BIT5) ? 1 : 0) ^ ((data[bytes].dp >> i) & 1)) ? POLY16 : 0);
        }
    }

    return bitrev(~crc & 0x1f, 5);
}

// -------------------------------------------------------------------------
// NRZI encoding
// -------------------------------------------------------------------------

int usbPkt::nrziEnc(const usbPkt::usb_signal_t raw[], usbPkt::usb_signal_t nrzi[], const unsigned len, const int start)
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
        for (int bit = 0; bit < NRZI_BITSPERBYTE; bit++)
        {
            // A zero is a state change
            if (!(raw[byte].dp & (NRZI_BYTELSBMASK << bit)))
            {
                state = ~state & NRZI_BYTELSBMASK;
                onescnt = 0;
            }
            // A one is no state change
            else
            {
                // Keep count of the number of consecutive 1s
                onescnt++;

                // If about to output a seventh 1, stuff an extra 0 in the output (state change)
                if (onescnt == 7)
                {
                    state = ~state;
                    outputp |= ( state & NRZI_BYTELSBMASK) << obit;
                    outputm |= (~state & NRZI_BYTELSBMASK) << obit;
                    obit++;
                    bitcnt++;
                }
            }

            // Output NRZI bit
            outputp |= ( state & NRZI_BYTELSBMASK) << obit;
            outputm |= (~state & NRZI_BYTELSBMASK) << obit;
            obit++;
            bitcnt++;

            // If output shift has a whole byte or more, send to output
            if (obit >= 8)
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
    outputp |= SE0P << obit;
    outputm |= SE0M << obit;
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
// NRZI decode with bit-stuffing removal. Checks errors for:
//
//  SE1
//  Bad SE0
//  Bad EOP sequence
//
// -------------------------------------------------------------------------

int usbPkt::nrziDec(const usb_signal_t nrzi[], usb_signal_t raw[], const int start)
{
    int      ibyte     = 0;
    int      eofactive = 0;
    int      bitcount  = 0;
    uint8_t  lastbit   = start;

    int      obyte     = 0;
    int      obit      = 0;
    uint32_t output    = 0;
    int      onecnt    = 0;

    usb_signal_t currbit;

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
                return USBERROR;
            }

            // If receiving a potential EOP...
            if (eofactive)
            {
                // If seen one SE0, check this bit is also SE0
                if (eofactive == 1 && !se)
                {
                    USBERRMSG("nrziDec: Bad EOP. SE0 not followed by another SE0\n");
                    return USBERROR;
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
                        return USBERROR;
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
                // Add decoded current bit to output: change = 0, same = 1
                output |= ((lastbit ^ currbit.dp) ? 0 : 1) << obit;

                // If not at a bit-stuffed point ...
                if (onecnt != 6)
                {
                    // Increment count of bits on output
                    obit++;
                    bitcount++;

                    // If a 1, increment onecnt
                    if (currbit.dp)
                    {
                        onecnt++;
                    }
                    // If a zero, clear onecnt
                    else
                    {
                        onecnt = 0;
                    }
                }
                // At bit stuff point, clear onecnt. Not incrementing obit and bitcount deletes the stuffed bit
                else
                {
                    onecnt = 0;
                }

                // If enough bits for a byte on the output, place in the output buffer
                if (obit >= 8)
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
}

// -------------------------------------------------------------------------
// Handshake and preamble packet generation
// -------------------------------------------------------------------------

int usbPkt::genPkt(usb_signal_t buf[], const int pid)
{
    int idx = 0;

    // Validate PID
    switch (pid)
    {
    case PID_HSHK_ACK:
    case PID_HSHK_NAK:
    case PID_HSHK_NYET:
    case PID_HSHK_STALL:
    case PID_SPCL_PREAMB:
        break;
    default:
        USBERRMSG("genPkt: Bad PID (0x%x) seen for handshake generation.\n", pid);
        return USBERROR;
    }

    // SOP/Sync
    rawbuf[idx].dp = SYNC;
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
// Token generation (not SOF)
// -------------------------------------------------------------------------

int usbPkt::genPkt(usb_signal_t buf[], const int pid, const uint8_t addr, const uint8_t endp)
{
    int idx = 0;
    unsigned crc;

    // Validate PID for this type of packet
    switch (pid)
    {
    case PID_TOKEN_IN:
    case PID_TOKEN_OUT:
    case PID_TOKEN_SETUP:
        break;
    default:
        USBERRMSG("genPkt: Bad PID (0x%x) seen for token generation.\n", pid);
        return USBERROR;
    }

    // Validate the address (0 to 127)
    if (addr > MAXDEVADDR)
    {
        USBERRMSG("genPkt: Invalid token address (0x%x\n", addr);
        return USBERROR;
    }

    // Validate the endpoint
    if (endp > MAXENDP)
    {
        USBERRMSG("genPkt: Invalid token end poubt (0x%x\n", endp);
        return USBERROR;
    }

    // SOP/Sync
    rawbuf[idx].dp = SYNC;
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
// SOF token generation
// -------------------------------------------------------------------------

int usbPkt::genPkt(usb_signal_t buf[], const int pid, const uint16_t framenum)
{
    int idx = 0;
    unsigned crc;

    // Validate PID for this type of packet
    switch (pid)
    {
    case PID_TOKEN_SOF:
        break;
    default:
        USBERRMSG("genPkt: Bad PID (0x%x) seen for SOF generation.\n", pid);
        return USBERROR;
    }

    // Validate frame number
    if (framenum > MAXFRAMENUM)
    {
        USBERRMSG("genPkt: Ivalid frame number (%d)\n", framenum);
        return USBERROR;
    }

    // SOP/Sync
    rawbuf[idx].dp = SYNC;
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
// Data Packet generation
// -------------------------------------------------------------------------

int usbPkt::genPkt(usb_signal_t buf[], const int pid, const uint8_t data[], const unsigned len)
{
    int idx = 0;

    // Validate PID for this type of packet
    switch (pid)
    {
    case PID_DATA_0:
    case PID_DATA_1:
    case PID_DATA_2:
    case PID_DATA_M:
        break;
    default:
        USBERRMSG("genPkt: Bad PID (0x%x) seen for data packet generation.\n", pid);
        return USBERROR;
    }

    // Validate length
    switch (currspeed)
    {
    case usb_speed_e::LS:
        if (len > 8)
        {
            USBERRMSG("genPkt: Invalid data length for low speed (%d).\n", len);
            return USBERROR;
        }
        break;
    case usb_speed_e::FS:
        if (len > 64)
        {
            USBERRMSG("genPkt: Invalid data length for fast speed (%d).\n", len);
            return USBERROR;
        }
        break;
    case usb_speed_e::HS:
        if (len > 512)
        {
            USBERRMSG("genPkt: Invalid data length for high speed (%d).\n", len);
            return USBERROR;
        }
        break;
    }

    // SOP/Sync
    rawbuf[idx].dp = SYNC;
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
// -------------------------------------------------------------------------

int usbPkt::decodePkt(const usb_signal_t nrzibuf[], int& pid, uint32_t args[], uint8_t data[], int &databytes)
{
    databytes = 0;

    int crc;

    // NRZI decode
    int bitcnt = nrziDec(nrzibuf, rawbuf);

    if (bitcnt < MINPKTSIZEBITS)
    {
        USBERRMSG("%sdecodePkt: Invalid bit count returned from nrziDec (%d).\n", errbuf, bitcnt);

        return USBERROR;
    }

    // Extract PID
    pid = rawbuf[PIDBYTEOFFSET].dp & 0xf;
    uint8_t pidchk = (~rawbuf[PIDBYTEOFFSET].dp >> 4) & 0xf;

    // Check PID inverse is in top bits
    if (pid != pidchk)
    {
        USBERRMSG("decodePkt: Invalid PID. Top nibble is not the inverse of bottom nibble (%d).\n", rawbuf[PIDBYTEOFFSET].dp);
        return USBERROR;
    }

    switch (pid)
    {
    case PID_HSHK_ACK:
        USBDISPPKT("  RX HNDSHK: ACK\n");
        break;
    case PID_HSHK_NAK:
        USBDISPPKT("  RX HNDSHK: NAK\n");
        break;
    case PID_HSHK_STALL:
        USBDISPPKT("  RX HNDSHK: STALL\n");
        break;
    case PID_HSHK_NYET:
        USBDISPPKT("  RX HNDSHK: NYET\n");
        break;

    case PID_TOKEN_OUT:
    case PID_TOKEN_IN:
    case PID_TOKEN_SETUP:
        args[ARGADDRIDX]    = rawbuf[ADDRBYTEOFFSET].dp & 0x7f;                                                 // Address
        args[ARGENDPIDX]    = (rawbuf[ENDPBYTEOFFSET].dp >> 7) | ((rawbuf[ENDPBYTEOFFSET+1].dp & 0x7) << 1);    // ENDP
        args[ARGTKNCRC5IDX] = rawbuf[CRC5BYTEOFFSET].dp >> 3;                                                   // CRC5

        crc = usbcrc5(&rawbuf[ADDRBYTEOFFSET], 2, 3);

        if (args[ARGTKNCRC5IDX] != crc)
        {
            USBERRMSG("decodePkt: Bad CRC5 for token. Got 0x%x, expected 0x%x.\n", args[ARGTKNCRC5IDX], crc);
            return USBERROR;
        }
        else if (pid == PID_TOKEN_OUT)
        {
            USBDISPPKT("  RX TOKEN: OUT   (ADDR=%d ENDP=%d)\n", args[ARGADDRIDX], args[ARGENDPIDX]);
        }
        else if (pid == PID_TOKEN_IN)
        {
            USBDISPPKT("  RX TOKEN: IN    (ADDR=%d ENDP=%d)\n", args[ARGADDRIDX], args[ARGENDPIDX]);
        }
        else
        {
            USBDISPPKT("  RX TOKEN: SETUP (ADDR=%d ENDP=%d)\n", args[ARGADDRIDX], args[ARGENDPIDX]);
        }
        break;

    case PID_TOKEN_SOF:
        args[ARGFRAMEIDX]   = rawbuf[FRAMEBYTEOFFSET].dp | (rawbuf[FRAMEBYTEOFFSET+1].dp & 0x7) << 8;           // Frame number
        args[ARGSOFCRC5IDX] = rawbuf[CRC5BYTEOFFSET].dp >> 3;                                                   // CRC5

        crc = usbcrc5(&rawbuf[FRAMEBYTEOFFSET], 2, 3);

        if (args[ARGSOFCRC5IDX] != crc)
        {
            USBERRMSG("decodePkt: Bad CRC5 for SOF. Got 0x%x, expected 0x%x.\n", args[ARGTKNCRC5IDX], crc);
            return USBERROR;
        }

        USBDISPPKT("  RX TOKEN: SOF   (FRAME= %d)\n", args[ARGFRAMEIDX]);

        break;

    case PID_DATA_0:
    case PID_DATA_1:
        // Calulate the size of the data packet payload (total size in bytes minus SYNC, PID and CRC16)
        databytes = (bitcnt / 8) - DATABYTEOFFSET - 2;

        // Extract CRC16
        args[ARGCRC16IDX] = rawbuf[databytes + DATABYTEOFFSET].dp | (rawbuf[databytes + DATABYTEOFFSET + 1].dp << 8); // CRC16

        // Calculate CRC16
        crc = usbcrc16(&rawbuf[DATABYTEOFFSET], databytes);

        // Check CRCs match
        if (crc != args[ARGCRC16IDX])
        {
            USBERRMSG("decodePkt: Bad CRC16 for data packet. Got 0x%04x, expected 0x%04x.\n", args[ARGCRC16IDX], crc);
            return USBERROR;
        }

        // Copy validated memory to output buffer
        USBDISPPKT("  RX DATA: %s:", pid == PID_DATA_0 ? "DATA0" : "DATA1");

        for (int idx = 0; idx < databytes; idx++)
        {
            data[idx] = rawbuf[DATABYTEOFFSET+idx].dp;

            if ((idx % 16) == 0)
            {
                USBDISPPKT("\n   ");
            }
            USBDISPPKT(" %02x", data[idx]);
        }

        if (databytes % 16)
        {
            USBDISPPKT("\n");
        }

        break;

    // Unsupported
    case PID_TOKEN_ERR:
    case PID_TOKEN_SPLIT:
    case PID_TOKEN_PING:
    case PID_DATA_2:
    case PID_DATA_M:
        USBERRMSG("decodePkt: Unsupported packet type (0x%x)\n", pid);
        return USBUNSUPPORTED;
        break;

    default:
        USBERRMSG("decodePkt: Unrecognised packet type (0x%x)\n", pid);
        return USBERROR;
        break;
    }

    USBERRMSG("");

    return USBOK;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

#define TEST

#ifdef TEST

int main(int argc, char * argv[])
{
    int idx;
    usbPkt pkt;

    uint8_t decbuf[1024];

#ifdef TESTCRC
    unsigned crc;

    usbPkt::usb_signal_t data[4] = { {0x15, 0x00}, {0x07, 0x00}, {0x00, 0x00}, {0x00, 0x00} };

    crc = pkt.usbcrc5(data, 2, 3);

    printf("crc = 0x%04x (0x%04x)\n", crc, pkt.bitrev(crc, 5));

    usbPkt::usb_signal_t data1[4] = { {0x23, 0x00}, {0x45, 0x00}, {0x067, 0x00}, {0x89, 0x00} };

    crc = pkt.usbcrc16(data1, 4);

    printf("crc = 0x%04x (0x%04x)\n", crc, pkt.bitrev(crc, 16));
#endif

    usbPkt::usb_signal_t data2[1024];

    // Generate NAK handshake
    int numbits = pkt.genPkt(data2, usbPkt::PID_HSHK_NAK);

    for (idx = 0; idx < numbits / 8; idx++)
    {
        for (int bits = 0; bits < 8; bits++)
            printf("%c", usbPkt::bitenc(data2[idx], bits));
        printf("%c", '_');
    }

    for (int bits = 0; bits < numbits % 8; bits++)
        printf("%c", usbPkt::bitenc(data2[idx], bits));

    printf("\n");

    int      pid;
    uint32_t args[4];
    char errstr[usbPkt::ERRBUFSIZE];
    int databytes;

    if (pkt.decodePkt(data2, pid, args, NULL, databytes) != usbPkt::USBOK)
    {
        fprintf(stderr, "***ERROR: Decoded bad handshake packet:\n    ");
        pkt.getUsbErrMsg(errstr);
        fprintf(stderr, "%s\n", errstr);
    }

    // Generate SETUP token
    numbits = pkt.genPkt(data2, usbPkt::PID_TOKEN_SETUP, 0x15, 0xe);

    for (idx = 0; idx < numbits / 8; idx++)
    {
        for (int bits = 0; bits < 8; bits++)
            printf("%c", usbPkt::bitenc(data2[idx], bits));
        printf("%c", '_');
    }

    for (int bits = 0; bits < numbits % 8; bits++)
        printf("%c", usbPkt::bitenc(data2[idx], bits));

    printf("\n");

    if (pkt.decodePkt(data2, pid, args, NULL, databytes))
    {
        fprintf(stderr, "***ERROR: Decoded bad token packet:\n    ");
        pkt.getUsbErrMsg(errstr);
        fprintf(stderr, "%s\n", errstr);
    }

    // Generate DATA1 packet
    uint8_t payload[4] = { 0x23, 0x45, 0x67, 0x89 };
    numbits = pkt.genPkt(data2, usbPkt::PID_DATA_1, payload, 4);

    for (idx = 0; idx < numbits / 8; idx++)
    {
        for (int bits = 0; bits < 8; bits++)
            printf("%c", usbPkt::bitenc(data2[idx], bits));

        printf("%c", '_');
    }

    for (int bits = 0; bits < numbits % 8; bits++)
        printf("%c", usbPkt::bitenc(data2[idx], bits));

    printf("\n");

    if (pkt.decodePkt(data2, pid, args, decbuf, databytes))
    {
        fprintf(stderr, "***ERROR: Decoded bad token packet:\n    ");
        pkt.getUsbErrMsg(errstr);
        fprintf(stderr, "%s\n", errstr);
    }
}

#endif