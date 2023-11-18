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
    int fmtDevDescriptor(char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        deviceDesc *desc = (deviceDesc*)rawdata;

        return snprintf(sbuf, maxstrsize,
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
    int fmtCfgDescriptor(char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        configDesc *desc = (configDesc*)rawdata;

        return snprintf(sbuf, maxstrsize,
                        FMT_DATA_GREY
                        "  bLength                = %d\n"
                        "  bDescriptorType        = %s\n"
                        "  wTotalLength           = 0x%04x\n"
                        "  bNumInterfaces         = 0x%02x\n"
                        "  bConfigurationValue    = 0x%02x\n"
                        "  iConfiguration         = 0x%02x\n"
                        "  bmAttributes           = 0x%02x\n"
                        "  bMaxPower              = 0x%02x\n"
                        FMT_NORMAL,
                        desc->bLength, fmtDecriptorType(desc->bDescriptorType), desc->wTotalLength,
                        desc->bNumInterfaces, desc->bConfigurationValue,
                        desc->iConfiguration, desc->bmAttributes,
                        desc->bMaxPower);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtIfDescriptor(char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        interfaceDesc *desc = (interfaceDesc*)rawdata;

        return snprintf(sbuf, maxstrsize,
                        FMT_DATA_GREY
                        "  bLength                = %d\n"
                        "  bDescriptorType        = %s\n"
                        "  bAlternateSetting      = %02x\n"
                        "  bNumEndpoints          = %02x\n"
                        "  bInterfaceClass        = %02x\n"
                        "  bInterfaceSubClass     = %02x\n"
                        "  bInterfaceProtocol     = %02x\n"
                        "  iInterface             = %02x\n"
                        FMT_NORMAL,
                        desc->bLength, fmtDecriptorType(desc->bDescriptorType),
                        desc->bAlternateSetting,
                        desc->bNumEndpoints,
                        desc->bInterfaceClass,
                        desc->bInterfaceSubClass,
                        desc->bInterfaceProtocol,
                        desc->iInterface);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------
    int fmtEpDescriptor  (char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        endpointDesc *desc = (endpointDesc*)rawdata;

        return snprintf(sbuf, maxstrsize,
                        FMT_DATA_GREY
                        "  bLength                = %d\n"
                        "  bDescriptorType        = %s\n"
                        "  bEndpointAddress       = %02x\n"
                        "  bmAttributes           = %02x\n"
                        "  wMaxPacketSize         = %04x\n"
                        "  bInterval              = %02x\n"
                        FMT_NORMAL,
                        desc->bLength, fmtDecriptorType(desc->bDescriptorType),
                        desc->bEndpointAddress,
                        desc->bmAttributes,
                        desc->wMaxPacketSize,
                        desc->bInterval);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtHdrFuncDescriptor (char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        headerFuncDesc *desc = (headerFuncDesc*)rawdata;

        return snprintf(sbuf, maxstrsize,
                        FMT_DATA_GREY
                        "  bLength                = %d\n"
                        "  bDescriptorType        = %s\n"
                        "  bDescriptorSubType     = %s\n"
                        "  bcdCDC                 = %02x\n"
                        FMT_NORMAL,
                        desc->bLength, fmtDecriptorType(desc->bDescriptorType),
                        fmtFuncDescSubtype(desc->bDescriptorSubType),
                        desc->bcdCDC);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtAcmFuncDescriptor (char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        acmFuncDesc *desc = (acmFuncDesc*)rawdata;

        return snprintf(sbuf, maxstrsize,
                        FMT_DATA_GREY
                        "  bLength                = %d\n"
                        "  bDescriptorType        = %s\n"
                        "  bDescriptorSubType     = %s\n"
                        "  bmCapabilities         = %02x\n"
                        FMT_NORMAL,
                        desc->bLength, fmtDecriptorType(desc->bDescriptorType),
                        fmtFuncDescSubtype(desc->bDescriptorSubType),
                        desc->bmCapabilities);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtUnionFuncDescriptor (char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        unionFuncDesc *desc = (unionFuncDesc*)rawdata;

        return snprintf(sbuf, maxstrsize,
                        FMT_DATA_GREY
                        "  bLength                = %d\n"
                        "  bDescriptorType        = %s\n"
                        "  bDescriptorSubType     = %s\n"
                        "  bControlInterface      = %02x\n"
                        "  bSubordinateInterface0 = %02x\n"
                        FMT_NORMAL,
                        desc->bLength, fmtDecriptorType(desc->bDescriptorType),
                        fmtFuncDescSubtype(desc->bDescriptorSubType),
                        desc->bControlInterface,
                        desc->bSubordinateInterface0);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtCallMgmtFuncDescriptor (char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        callMgmtFuncDesc *desc = (callMgmtFuncDesc*)rawdata;

        return snprintf(sbuf, maxstrsize,
                        FMT_DATA_GREY
                        "  bLength               = %d\n"
                        "  bDescriptorType       = %s\n"
                        "  bDescriptorSubType    = %s\n"
                        "  bmCapabilities        = %02x\n"
                        "  bmDataInterface       = %02x\n"
                        FMT_NORMAL,
                        desc->bLength, fmtDecriptorType(desc->bDescriptorType),
                        fmtFuncDescSubtype(desc->bDescriptorSubType),
                        desc->bmCapabilities,
                        desc->bmDataInterface);
    }

    // -------------------------------------------------------------------------
    // -------------------------------------------------------------------------

    int fmtCfgAllDescriptor (char sbuf[], const uint8_t rawdata[], const int maxstrsize)
    {
        int     soffset        = 0;
        int     roffset        = 0;
        uint8_t subtype;

        int totallen = ((configDesc*)rawdata)->wTotalLength;

        do
        {
            uint8_t desctype = rawdata[roffset+1];

            USBDEVDEBUG(stderr, "==> fmtCfgAllDescriptor desctype=0x%02x totallen=%d roffset=%d soffset=%d\n", desctype, totallen, roffset, soffset);

            switch(desctype)
            {
            case CONFIG_DESCRIPTOR_TYPE:
                soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\nConfiguration Descriptor:\n\n");
                soffset += fmtCfgDescriptor(&sbuf[soffset], &rawdata[roffset], maxstrsize-soffset); roffset += rawdata[roffset];
                break;
            case IF_DESCRIPTOR_TYPE:
                soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\nInterface Descriptor:\n\n");
                soffset += fmtIfDescriptor(&sbuf[soffset], &rawdata[roffset], maxstrsize-soffset); roffset+= rawdata[roffset];
                break;
            case EP_DESCRIPTOR_TYPE:
                soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\nEndpoint Descriptor:\n\n");
                soffset += fmtEpDescriptor(&sbuf[soffset], &rawdata[roffset], maxstrsize-soffset); roffset+= rawdata[roffset];
                break;
            case CS_IF_DESCRIPTOR_TYPE:
                subtype = rawdata[roffset+2];
                switch(subtype)
                {
                case HEADER_SUBTYPE:
                    soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\nHeader Function Descriptor:\n\n");
                    soffset += fmtHdrFuncDescriptor(&sbuf[soffset], &rawdata[roffset], maxstrsize-soffset); roffset+= rawdata[roffset];
                    break;
                case CALL_MGMT_SUBTYPE:
                    soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\nCall Management Function Descriptor:\n\n");
                    soffset += fmtCallMgmtFuncDescriptor(&sbuf[soffset], &rawdata[roffset], maxstrsize-soffset); roffset+= rawdata[roffset];
                    break;
                case ACM_SUBTYPE:
                    soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\nAbstract Control Management Functional Descriptor:\n\n");
                    soffset += fmtAcmFuncDescriptor(&sbuf[soffset], &rawdata[roffset], maxstrsize-soffset); roffset+= rawdata[roffset];
                    break;
                case UNION_SUBTYPE:
                    soffset += snprintf(&sbuf[soffset], maxstrsize-soffset,  "\nUnion Function Descriptor:\n\n");
                    soffset += fmtUnionFuncDescriptor(&sbuf[soffset], &rawdata[roffset], maxstrsize-soffset); roffset+= rawdata[roffset];
                    break;
                default:
                    soffset += snprintf(&sbuf[soffset],  maxstrsize-soffset, "\nUNKNOWN descriptor subtype (0x%02x)\n\n", subtype);
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
    const char* fmtLineState(const unsigned linestate)
    {
        return (linestate == usbModel::USB_K)   ? "K" :
               (linestate == usbModel::USB_J)   ? "J" :
               (linestate == usbModel::USB_SE0) ? "SE0" :
               (linestate == usbModel::USB_SE1) ? "SE1" : "?";
    }
}