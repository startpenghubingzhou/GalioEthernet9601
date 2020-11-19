//
//  DM9601Struct.h
//  GalioEthernet9601
//
//  Created by PHBZ on 2020/11/17.
//  Copyright © 2020 penghubingzhou. All rights reserved.
//

#ifndef DM9601Struct_h
#define DM9601Struct_h

//*********************************
#pragma mark -
#pragma mark macros_defs
#pragma mark -
//*********************************

#define TRANSMIT_QUEUE_SIZE     256                // How does this relate to MAX_BLOCK_SIZE?
#define WATCHDOG_TIMER_MS       1000

#define MAX_BLOCK_SIZE        PAGE_SIZE
#define COMM_BUFF_SIZE        16

#define nameLength        32                // Arbitrary length
#define defaultName        "USB Ethernet"

#define kFiltersSupportedMask    0xefff
#define kPipeStalled        1

#define kOutBufPool        6
#define kOutBuffThreshold    100

#define kEthernetControlModel    6 // USB CDC Definitions (Ethernet Control Model)

#define    numStats    13

//*********************************
#pragma mark -
#pragma mark enum_defs
#pragma mark -
//*********************************

enum DM9601Registers {
    RegNCR    = 0x00,    // Network Control Register
    NCRExtPHY    = 0x80,    // Select External PHY
    NCRFullDX    = 0x08,    // Full duplex
    NCRLoopback    = 0x06,    // Internal PHY analog loopback
    
    RegNSR    = 0x01,    // Network Status Register
    NSRSpeed10    = 0x80,    // 0 = 100MBps, 1 = 10MBps (internal PHY)
    NSRLinkUp    = 0x40,    // 1 = link up (internal PHY)
    NSRTXFull    = 0x10,    // TX FIFO full
    NSRRXOver    = 0x08,    // RX FIFO overflow
    
    RegRCR    = 0x05,    // RX Control Register
    RCRDiscardLong    = 0x20,    // Discard long packet (over 1522 bytes)
    RCRDiscardCRC    = 0x10,    // Discard CRC error packet
    RCRAllMulticast    = 0x08,    // Pass all multicast
    RCRPromiscuous    = 0x02,    // Promiscuous
    RCRRXEnable        = 0x01,    // RX enable
    
    RegEPCR    = 0x0b,    // EEPROM & PHY Control Register
    EPCROpSelect    = 0x08,    // EEPROM or PHY Operation Select
    EPCRRegRead        = 0x04,    // EEPROM or PHY Register Read Command
    EPCRRegWrite    = 0x02,    // EEPROM or PHY Register Write Command
    
    RegEPAR    = 0x0c,    // EEPROM & PHY Address Register
    EPARIntPHY        = 0x40,    // [7:6] force to 01 if Internal PHY is selected
    EPARMask        = 0x1f,    // mask [0:5]
    
    RegEPDRL = 0x0d, // EEPROM & PHY Low Byte Data Register
    
    RegEPDRH = 0x0e, // EEPROM & PHY Low Byte Data Register
    
    RegPAR    = 0x10,    // [0x10 - 0x15] Physical Address Register
    
    RegMAR    = 0x16,    // [0x16 - 0x1d] Multicast Address Register
    
    RegGPCR    = 0x1E,    // General Purpose Control Register
    GPCRPowerDown    = 0x01,    // [0:6] Define in/out direction of GPCR
    // GPIO0 - is output for Power Down function
    
    RegGPR    = 0x1F,    // General Purpose Register
    GPRPowerDownInPHY = 0x01,    // Power down Internal PHY
    
    RegUSBC    = 0xf4, // USB Control Register
    USBCIntAck        = 0x20,    // ACK with 8-bytes of data on interrupt EP
    USBCIntNAck        = 0x10,    // Supress ACK on interrupt EP
    
};

// Stats of interest in bmEthernetStatistics (bit definitions)
enum
{
    kXMIT_OK =            0x01,        // Byte 1
    kRCV_OK =            0x02,
    kXMIT_ERROR =        0x04,
    kRCV_ERROR =        0x08,
    
    kRCV_CRC_ERROR =        0x02,        // Byte 3
    kRCV_ERROR_ALIGNMENT =    0x08,
    kXMIT_ONE_COLLISION =    0x10,
    kXMIT_MORE_COLLISIONS =    0x20,
    kXMIT_DEFERRED =        0x40,
    kXMIT_MAX_COLLISION =    0x80,
    
    kRCV_OVERRUN =        0x01,        // Byte 4
    kXMIT_TIMES_CARRIER_LOST =    0x08,
    kXMIT_LATE_COLLISIONS =    0x10
};

// Vendor Requests
enum {
    kVenReqReadRegister            = 0,
    kVenReqWriteRegister        = 1,
    kVenReqWriteRegisterByte    = 3,
};

// Packet Filter definitions
enum
{
    kPACKET_TYPE_PROMISCUOUS =        0x0001,
    kPACKET_TYPE_ALL_MULTICAST =    0x0002,
    kPACKET_TYPE_DIRECTED =        0x0004,
    kPACKET_TYPE_BROADCAST =        0x0008,
    kPACKET_TYPE_MULTICAST =        0x0010
};

enum
{
    CS_INTERFACE            = 0x24,
    
    Header_FunctionalDescriptor        = 0x00,
    CM_FunctionalDescriptor        = 0x01,
    Union_FunctionalDescriptor        = 0x06,
    CS_FunctionalDescriptor        = 0x07,
    Enet_Functional_Descriptor        = 0x0f,
    
    CM_ManagementData            = 0x01,
    CM_ManagementOnData            = 0x02
};

//    Requests

enum
{
    kSend_Encapsulated_Command        = 0,
    kGet_Encapsulated_Response        = 1,
    kSet_Ethernet_Multicast_Filter    = 0x40,
    kSet_Ethernet_PM_Packet_Filter    = 0x41,
    kGet_Ethernet_PM_Packet_Filter    = 0x42,
    kSet_Ethernet_Packet_Filter        = 0x43,
    kGet_Ethernet_Statistics        = 0x44,
    kGet_AUX_Inputs            = 4,
    kSet_AUX_Outputs            = 5,
    kSet_Temp_MAC            = 6,
    kGet_Temp_MAC            = 7,
    kSet_URB_Size            = 8,
    kSet_SOFS_To_Wait            = 9,
    kSet_Even_Packets            = 10,
    kScan                = 0xFF
};

// Notifications

enum
{
    kNetwork_Connection            = 0,
    kResponse_Available            = 1,
    kConnection_Speed_Change        = 0x2A
};
//*********************************
#pragma mark -
#pragma mark struct_defs
#pragma mark -
//*********************************

typedef struct
{
    IOBufferMemoryDescriptor* pipeOutMDP;
    uint8_t* pipeOutBuffer;
    mbuf_t m;
} pipeOutBuffers;

typedef struct
{
    uint8_t    bFunctionLength;
    uint8_t     bDescriptorType;
    uint8_t     bDescriptorSubtype;
} HeaderFunctionalDescriptor;

typedef struct
{
    uint8_t     bFunctionLength;
    uint8_t     bDescriptorType;
    uint8_t     bDescriptorSubtype;
    uint8_t     iMACAddress;
    uint8_t     bmEthernetStatistics[4];
    uint8_t     wMaxSegmentSize[2];
    uint8_t     wNumberMCFilters[2];
    uint8_t     bNumberPowerFilters;
} EnetFunctionalDescriptor;

// Stats request definitions

enum
{
    kXMIT_OK_REQ =            0x0001,
    kRCV_OK_REQ =            0x0002,
    kXMIT_ERROR_REQ =            0x0003,
    kRCV_ERROR_REQ =            0x0004,
    
    kRCV_CRC_ERROR_REQ =        0x0012,
    kRCV_ERROR_ALIGNMENT_REQ =        0x0014,
    kXMIT_ONE_COLLISION_REQ =        0x0015,
    kXMIT_MORE_COLLISIONS_REQ =        0x0016,
    kXMIT_DEFERRED_REQ =        0x0017,
    kXMIT_MAX_COLLISION_REQ =        0x0018,
    
    kRCV_OVERRUN_REQ =            0x0019,
    kXMIT_TIMES_CARRIER_LOST_REQ =    0x001c,
    kXMIT_LATE_COLLISIONS_REQ =        0x001d
};


uint16_t stats[13] = {
    kXMIT_OK_REQ,
    kRCV_OK_REQ,
    kXMIT_ERROR_REQ,
    kRCV_ERROR_REQ,
    kRCV_CRC_ERROR_REQ,
    kRCV_ERROR_ALIGNMENT_REQ,
    kXMIT_ONE_COLLISION_REQ,
    kXMIT_MORE_COLLISIONS_REQ,
    kXMIT_DEFERRED_REQ,
    kXMIT_MAX_COLLISION_REQ,
    kRCV_OVERRUN_REQ,
    kXMIT_TIMES_CARRIER_LOST_REQ,
    kXMIT_LATE_COLLISIONS_REQ
};

static struct MediumTable
{
    uint32_t    type;
    uint32_t    speed;
}

mediumTable[] =
{
    {kIOMediumEthernetNone,                                                0},
    {kIOMediumEthernetAuto,                                                0},
    {kIOMediumEthernet10BaseT      | kIOMediumOptionHalfDuplex,                                10},
    {kIOMediumEthernet10BaseT      | kIOMediumOptionFullDuplex,                                10},
    {kIOMediumEthernet100BaseTX  | kIOMediumOptionHalfDuplex,                                100},
    {kIOMediumEthernet100BaseTX  | kIOMediumOptionFullDuplex,                                100}
};



#endif /* DM9601Struct_h */
