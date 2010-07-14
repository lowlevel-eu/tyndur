/*****************************************************************************
* Copyright (c) 2009-2010 Max Reitz                                          *
*                                                                            *
* THE COKE-WARE LICENSE                                                      *
*                                                                            *
* Redistribution  and use  in  source  and  binary  forms,  with or  without *
* modification,  are permitted  provided that the  following  conditions are *
* met:                                                                       *
*                                                                            *
*   1. Redistributions  of  source code  must  retain  the  above  copyright *
*      notice, this list of conditions and the following disclaimer.         *
*   2. Redistribution in  binary form  must reproduce  the  above  copyright *
*      notice  and either  the full  list  of  conditions and  the following *
*      disclaimer or a link to both.                                         *
*   3. If we meet one day and you think this stuff is worth it,  you may buy *
*      me a coke in return.                                                  *
*                                                                            *
* THIS SOFTWARE  IS PROVIDED BY THE REGENTS AND CONTRIBUTORS “AS IS” AND ANY *
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO, THE IMPLIED *
* WARRANTIES  OF MERCHANTABILITY  AND FITNESS  FOR A PARTICULAR PURPOSE  ARE *
* DISCLAIMED.  IN  NO  EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR *
* ANY DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL *
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR *
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER *
* CAUSED  AND  ON  ANY  THEORY OF  LIABILITY,  WHETHER  IN CONTRACT,  STRICT *
* LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY *
* OUT OF THE USE  OF THIS  SOFTWARE,  EVEN IF ADVISED OF THE  POSSIBILITY OF *
* SUCH DAMAGE.                                                               *
*****************************************************************************/

#ifndef USB_STRUCTS_H
#define USB_STRUCTS_H

#include <stdint.h>

#define USB_LOW_SPEED  USB_HUB_LOW_SPEED
#define USB_FULL_SPEED USB_HUB_FULL_SPEED
#define USB_HIGH_SPEED USB_HUB_HIGH_SPEED

enum usb_state {
    USB_ATTACHED = 0,
    USB_POWERED,
    USB_DEFAULT,
    USB_ADDRESS,
    USB_CONFIGURED,
    USB_SUSPENDED
};

struct usb_setup_packet {
    uint8_t type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} __attribute__((packed));

struct usb_descriptor {
    uint8_t length;
    uint8_t desc_type;
} __attribute__((packed));


#define USB_SETUP_OUT     (0 << 7)
#define USB_SETUP_IN      (1 << 7)
#define USB_SETUP_REQ_STD (0 << 5)
#define USB_SETUP_REQ_CLS (1 << 5)
#define USB_SETUP_REQ_VDR (2 << 5)
#define USB_SETUP_REC_DEV 0
#define USB_SETUP_REC_IF  1
#define USB_SETUP_REC_EP  2
#define USB_SETUP_REC_OTH 3

enum usb_setup_requst {
    USB_GET_STATUS = 0,
    USB_CLEAR_FEATURE = 1,
    USB_SET_FEATURE = 3,
    USB_SET_ADDRESS = 5,
    USB_GET_DESCRIPTOR = 6,
    USB_SET_DESCRIPTOR = 7,
    USB_GET_CONFIGURATION = 8,
    USB_SET_CONFIGURATION = 9,
    USB_GET_INTERFACE = 10,
    USB_SET_INTERFACE = 11,
    USB_SYNC_FRAME = 12
};

enum usb_features {
    USB_ENDPOINT_HALT = 0,
    USB_DEVICE_REMOTE_WAKEUP = 1,
    USB_TEST_MODE = 2
};

enum usb_setup_descriptor {
    USB_DESC_DEVICE = 1 << 8,
    USB_DESC_CONFIGURATION = 2 << 8,
    USB_DESC_STRING = 3 << 8,
    USB_DESC_INTERFACE = 4 << 8,
    USB_DESC_ENDPOINT = 5 << 8,
    USB_DESC_DEVICE_QUALIFIER = 6 << 8,
    USB_DESC_OTHER_SPEED_CONFIGURATION = 7 << 8,
    USB_DESC_INTERFACE_POWER = 8 << 8,

    USB_DESC_HUB = 41 << 8
};

struct usb_device_descriptor {
    struct usb_descriptor desc;
    uint16_t bcd_usb;
    uint8_t class, subclass, protocol;
    uint8_t mps0;
    uint16_t vendor_id, device_id;
    uint16_t bcd_device;
    uint8_t i_manufacturer, i_product, i_serial;
    uint8_t num_configurations;
} __attribute__((packed));

struct usb_config_descriptor
{
    struct usb_descriptor desc;
    uint16_t total_length;
    uint8_t num_interfaces;
    uint8_t configuration_value;
    uint8_t i_configuration;
    unsigned rsvd0         : 5;
    unsigned remote_wakeup : 1;
    unsigned self_powered  : 1;
    unsigned rsvd1         : 1;
    uint8_t max_power;
} __attribute__((packed));

struct usb_interface_descriptor
{
    struct usb_descriptor desc;
    uint8_t interface_number;
    uint8_t alternate_setting;
    uint8_t num_endpoints;
    uint8_t class, subclass, protocol;
    uint8_t i_interface;
} __attribute__((packed));

#define USB_EP_CONTROL     0
#define USB_EP_ISOCHRONOUS 1
#define USB_EP_BULK        2
#define USB_EP_INTERRUPT   3

struct usb_endpoint_descriptor {
    struct usb_descriptor desc;
    unsigned number    : 4;
    unsigned rsvd1     : 3;
    unsigned direction : 1;
    unsigned type  : 2;
    unsigned sync  : 2;
    unsigned usage : 2;
    unsigned rsvd2 : 2;
    unsigned mps       : 11;
    unsigned trans_opp : 2;
    unsigned rsvd3     : 3;
    uint8_t interval;
} __attribute__((packed));

struct usb_hub_descriptor {
    struct usb_descriptor desc;
    uint8_t nbr_ports;
    unsigned power_mode : 2;
    unsigned compound   : 1;
    unsigned oc_protect : 2;
    unsigned tt         : 2;
    unsigned pindicator : 1;
    uint8_t rsvd;
    uint8_t pwr_on_2_pwr_good;
    uint8_t hub_contr_current;
    // Die folgenden Felder haben eine variable Größe
} __attribute__((packed));

#endif

