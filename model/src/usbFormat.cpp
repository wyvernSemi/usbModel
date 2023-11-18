//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 16th Novenmber 2023
//
// Contains the usbModel formatting functions
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

#include <stdio.h>
#include <stdint.h>

#include "usbFormat.h"
#include "usbCommon.h"

namespace usbModel
{
    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    const char* fmtDecriptorType(const uint8_t desc)
    {
        switch(desc)
        {
        case USB_DEV_DESCRIPTOR_TYPE : return "USB_DEV_DESCRIPTOR_TYPE";
        case USB_CFG_DESCRIPTOR_TYPE : return "USB_CFG_DESCRIPTOR_TYPE";
        case USB_STR_DESCRIPTOR_TYPE : return "USB_STR_DESCRIPTOR_TYPE";
        case USB_IF_DESCRIPTOR_TYPE  : return "USB_IF_DESCRIPTOR_TYPE";
        case USB_EP_DESCRIPTOR_TYPE  : return "USB_EP_DESCRIPTOR_TYPE";
        case USB_FUNC_DESCRIPTOR_TYPE: return "USB_FUNC_DESCRIPTOR_TYPE";
        default:                       return "UNKNOWN";
        }
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    const char* fmtFuncDescSubtype(const uint8_t subtype)
    {
        switch(subtype)
        {
        case HEADER_SUBTYPE   : return "HEADER";
        case CALL_MGMT_SUBTYPE: return "CALL MANAGMENT";
        case ACM_SUBTYPE      : return "ABSTRACT CONTROL MANAGEMENT";
        case UNION_SUBTYPE    : return "UNION";
        default:                      return "UNKNOWN";
        }
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    int fmtDevDescriptor(char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        deviceDesc *desc = (deviceDesc*)rawdata;

        for (int idx = 0; idx < indent; idx++)
        {
            sbuf[idx] = ' ';
        }

        return snprintf(&sbuf[indent], maxstrsize-indent,
                        FMT_DATA_GREY
                        "  bLength                = %d\n"
                        "  bDescriptorType        = %s\n"
                        "  bcdUSB                 = 0x%04x\n"
                        "  bDeviceClass           = 0x%02x\n"
                        "  bDeviceSubClass        = 0x%02x\n"
                        "  bDeviceProtocol        = 0x%02x\n"
                        "  bMaxPacketSize         = 0x%02x\n"
                        "  idVendor               = 0x%04x\n"
                        "  idProduct              = 0x%04x\n"
                        "  bcdDevice              = 0x%04x\n"
                        "  iManufacturer          = 0x%02x\n"
                        "  iProduct               = 0x%02x\n"
                        "  iSerialNumber          = 0x%02x\n"
                        "  bNumConfigurations     = 0x%02x\n\n"
                        FMT_NORMAL,
                        desc->bLength, fmtDecriptorType(desc->bDescriptorType), desc->bcdUSB, desc->bDeviceClass,
                        desc->bDeviceSubClass, desc->bDeviceProtocol, desc->bMaxPacketSize, desc->idVendor,
                        desc->idProduct, desc->bcdDevice, desc->iManufacturer, desc->iProduct,
                        desc->iSerialNumber, desc->bNumConfigurations);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    int fmtCfgDescriptor(char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        configDesc *desc = (configDesc*)rawdata;
        char ibuf[100];

        for (int idx = 0; idx < indent && idx < 100-1; idx++)
        {
            ibuf[idx] = ' ';
        }
        ibuf[indent] = 0;

        return snprintf(sbuf, maxstrsize-indent,
                        FMT_DATA_GREY
                        "%s  bLength                = %d\n"
                        "%s  bDescriptorType        = %s\n"
                        "%s  wTotalLength           = 0x%04x\n"
                        "%s  bNumInterfaces         = 0x%02x\n"
                        "%s  bConfigurationValue    = 0x%02x\n"
                        "%s  iConfiguration         = 0x%02x\n"
                        "%s  bmAttributes           = 0x%02x\n"
                        "%s  bMaxPower              = 0x%02x\n"
                        FMT_NORMAL,
                        ibuf, desc->bLength,
                        ibuf, fmtDecriptorType(desc->bDescriptorType),
                        ibuf, desc->wTotalLength,
                        ibuf, desc->bNumInterfaces,
                        ibuf, desc->bConfigurationValue,
                        ibuf, desc->iConfiguration,
                        ibuf, desc->bmAttributes,
                        ibuf, desc->bMaxPower);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtIfDescriptor(char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        interfaceDesc *desc = (interfaceDesc*)rawdata;
        char ibuf[100];

        for (int idx = 0; idx < indent && idx < 100-1; idx++)
        {
            ibuf[idx] = ' ';
        }
        ibuf[indent] = 0;

        return snprintf(sbuf, maxstrsize-indent,
                        FMT_DATA_GREY
                        "%s  bLength                = %d\n"
                        "%s  bDescriptorType        = %s\n"
                        "%s  bAlternateSetting      = %02x\n"
                        "%s  bNumEndpoints          = %02x\n"
                        "%s  bInterfaceClass        = %02x\n"
                        "%s  bInterfaceSubClass     = %02x\n"
                        "%s  bInterfaceProtocol     = %02x\n"
                        "%s  iInterface             = %02x\n"
                        FMT_NORMAL,
                        ibuf, desc->bLength,
                        ibuf, fmtDecriptorType(desc->bDescriptorType),
                        ibuf, desc->bAlternateSetting,
                        ibuf, desc->bNumEndpoints,
                        ibuf, desc->bInterfaceClass,
                        ibuf, desc->bInterfaceSubClass,
                        ibuf, desc->bInterfaceProtocol,
                        ibuf, desc->iInterface);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    int fmtEpDescriptor  (char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        endpointDesc *desc = (endpointDesc*)rawdata;
        char ibuf[100];

        for (int idx = 0; idx < indent && idx < 100-1; idx++)
        {
            ibuf[idx] = ' ';
        }
        ibuf[indent] = 0;

        return snprintf(sbuf, maxstrsize-indent,
                        FMT_DATA_GREY
                        "%s  bLength                = %d\n"
                        "%s  bDescriptorType        = %s\n"
                        "%s  bEndpointAddress       = %02x\n"
                        "%s  bmAttributes           = %02x\n"
                        "%s  wMaxPacketSize         = %04x\n"
                        "%s  bInterval              = %02x\n"
                        FMT_NORMAL,
                        ibuf, desc->bLength,
                        ibuf, fmtDecriptorType(desc->bDescriptorType),
                        ibuf, desc->bEndpointAddress,
                        ibuf, desc->bmAttributes,
                        ibuf, desc->wMaxPacketSize,
                        ibuf, desc->bInterval);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtHdrFuncDescriptor (char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        headerFuncDesc *desc = (headerFuncDesc*)rawdata;
        char ibuf[100];

        for (int idx = 0; idx < indent; idx++)
        {
            ibuf[idx] = ' ';
        }
        ibuf[indent] = 0;

        return snprintf(sbuf, maxstrsize-indent,
                        FMT_DATA_GREY
                        "%s  bLength                = %d\n"
                        "%s  bDescriptorType        = %s\n"
                        "%s  bDescriptorSubType     = %s\n"
                        "%s  bcdCDC                 = %02x\n"
                        FMT_NORMAL,
                        ibuf, desc->bLength,
                        ibuf, fmtDecriptorType(desc->bDescriptorType),
                        ibuf, fmtFuncDescSubtype(desc->bDescriptorSubType),
                        ibuf, desc->bcdCDC);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtAcmFuncDescriptor (char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        acmFuncDesc *desc = (acmFuncDesc*)rawdata;
        char ibuf[100];

        for (int idx = 0; idx < indent && idx < 100-1; idx++)
        {
            ibuf[idx] = ' ';
        }
        ibuf[indent] = 0;

        return snprintf(sbuf, maxstrsize-indent,
                        FMT_DATA_GREY
                        "%s  bLength                = %d\n"
                        "%s  bDescriptorType        = %s\n"
                        "%s  bDescriptorSubType     = %s\n"
                        "%s  bmCapabilities         = %02x\n"
                        FMT_NORMAL,
                        ibuf, desc->bLength,
                        ibuf, fmtDecriptorType(desc->bDescriptorType),
                        ibuf, fmtFuncDescSubtype(desc->bDescriptorSubType),
                        ibuf, desc->bmCapabilities);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtUnionFuncDescriptor (char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        unionFuncDesc *desc = (unionFuncDesc*)rawdata;
        char ibuf[100];

        for (int idx = 0; idx < indent && idx < 100-1; idx++)
        {
            ibuf[idx] = ' ';
        }
        ibuf[indent] = 0;

        return snprintf(sbuf, maxstrsize-indent,
                        FMT_DATA_GREY
                        "%s  bLength                = %d\n"
                        "%s  bDescriptorType        = %s\n"
                        "%s  bDescriptorSubType     = %s\n"
                        "%s  bControlInterface      = %02x\n"
                        "%s  bSubordinateInterface0 = %02x\n"
                        FMT_NORMAL,
                        ibuf, desc->bLength,
                        ibuf, fmtDecriptorType(desc->bDescriptorType),
                        ibuf, fmtFuncDescSubtype(desc->bDescriptorSubType),
                        ibuf, desc->bControlInterface,
                        ibuf, desc->bSubordinateInterface0);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtCallMgmtFuncDescriptor (char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        callMgmtFuncDesc *desc = (callMgmtFuncDesc*)rawdata;
        char ibuf[100];

        for (int idx = 0; idx < indent && idx < 100-1; idx++)
        {
            ibuf[idx] = ' ' ;
        }
        ibuf[indent] = 0;

        return snprintf(sbuf, maxstrsize-indent,
                        FMT_DATA_GREY
                        "%s  bLength               = %d\n"
                        "%s  bDescriptorType       = %s\n"
                        "%s  bDescriptorSubType    = %s\n"
                        "%s  bmCapabilities        = %02x\n"
                        "%s  bmDataInterface       = %02x\n"
                        FMT_NORMAL,
                        ibuf, desc->bLength,
                        ibuf, fmtDecriptorType(desc->bDescriptorType),
                        ibuf, fmtFuncDescSubtype(desc->bDescriptorSubType),
                        ibuf, desc->bmCapabilities,
                        ibuf, desc->bmDataInterface);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtCfgAllDescriptor (char sbuf[], const uint8_t rawdata[], const unsigned indent, const int maxstrsize)
    {
        int     soffset        = 0;
        int     roffset        = 0;
        uint8_t subtype;
        int     subdescindent  = indent + 2;

        int      totallen      = ((configDesc*)rawdata)->wTotalLength;

        do
        {
            uint8_t desctype = rawdata[roffset+1];

            USBDEVDEBUG("==> fmtCfgAllDescriptor desctype=0x%02x totallen=%d roffset=%d soffset=%d\n", desctype, totallen, roffset, soffset);

            for (int idx = 0; idx < indent; idx++)
            {
                sbuf[soffset] = ' ';
                soffset++;
            }

            switch(desctype)
            {
            case CONFIG_DESCRIPTOR_TYPE:
                soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\nConfiguration Descriptor:\n\n");
                soffset += fmtCfgDescriptor(&sbuf[soffset], &rawdata[roffset], 0, maxstrsize-soffset); roffset += rawdata[roffset];
                break;
            case IF_DESCRIPTOR_TYPE:
                soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\n..Interface Descriptor:\n\n");
                soffset += fmtIfDescriptor(&sbuf[soffset], &rawdata[roffset], subdescindent, maxstrsize-soffset); roffset+= rawdata[roffset];
                break;
            case EP_DESCRIPTOR_TYPE:
                soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\n....Endpoint Descriptor:\n\n");
                soffset += fmtEpDescriptor(&sbuf[soffset], &rawdata[roffset], subdescindent+2, maxstrsize-soffset); roffset+= rawdata[roffset];
                break;
            case CS_IF_DESCRIPTOR_TYPE:
                subtype = rawdata[roffset+2];
                switch(subtype)
                {
                case HEADER_SUBTYPE:
                    soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\n..Header Function Descriptor:\n\n");
                    soffset += fmtHdrFuncDescriptor(&sbuf[soffset], &rawdata[roffset], subdescindent, maxstrsize-soffset); roffset+= rawdata[roffset];
                    break;
                case CALL_MGMT_SUBTYPE:
                    soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\n..Call Management Function Descriptor:\n\n");
                    soffset += fmtCallMgmtFuncDescriptor(&sbuf[soffset], &rawdata[roffset], subdescindent, maxstrsize-soffset); roffset+= rawdata[roffset];
                    break;
                case ACM_SUBTYPE:
                    soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\n..Abstract Control Management Functional Descriptor:\n\n");
                    soffset += fmtAcmFuncDescriptor(&sbuf[soffset], &rawdata[roffset], subdescindent, maxstrsize-soffset); roffset+= rawdata[roffset];
                    break;
                case UNION_SUBTYPE:
                    soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\n..Union Function Descriptor:\n\n");
                    soffset += fmtUnionFuncDescriptor(&sbuf[soffset], &rawdata[roffset], subdescindent, maxstrsize-soffset); roffset+= rawdata[roffset];
                    break;
                default:
                    soffset += snprintf(&sbuf[soffset],  maxstrsize-soffset, "\n  UNKNOWN descriptor subtype (0x%02x)\n\n", subtype);
                    return USBERROR;
                    break;
                }
                break;
            default:
                soffset += snprintf(&sbuf[soffset], maxstrsize-soffset, "\nUNKNOWN descriptor type (0x%02x)\n\n", desctype);
                return USBERROR;
                break;
            }
        } while (roffset < totallen && soffset < maxstrsize);

        snprintf(&sbuf[soffset], maxstrsize-soffset, "\n");

        return USBOK;
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int strToUnicode (uint16_t* dst, const char* src, const int maxstrsize)
    {
        int idx;
        for (idx = 0; idx < maxstrsize && src[idx] != 0; idx++)
        {
            dst[idx]     = (uint16_t)src[idx];
        }

        return idx * 2;
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int UnicodeToStr (char* dst, const uint16_t* src, const int length, const int maxstrsize)
    {
        int idx;
        for (idx = 0; idx < maxstrsize && idx < length; idx++)
        {
            dst[idx]     = (uint8_t)(src[idx] & 0xff);
        }

        dst[idx] = 0;

        return idx;
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    const char* fmtLineState(const unsigned linestate)
    {
        return (linestate == usbModel::USB_K)   ? "K" :
               (linestate == usbModel::USB_J)   ? "J" :
               (linestate == usbModel::USB_SE0) ? "SE0" :
               (linestate == usbModel::USB_SE1) ? "SE1" : "?";
    }
}