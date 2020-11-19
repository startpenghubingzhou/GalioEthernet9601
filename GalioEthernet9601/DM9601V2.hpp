//
//  DM9601V2.hpp
//  GalioEthernet9601
//
//  Created by PHBZ on 2020/11/17.
//  Copyright © 2020 penghubingzhou. All rights reserved.
//

#ifndef DM9601V2_h
#define DM9601V2_h

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>

#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IOGatedOutputQueue.h>

#include <IOKit/usb/IOUSBHostDevice.h>
#include <IOKit/usb/IOUSBHostInterface.h>
#include <IOKit/usb/IOUSBHostPipe.h>
#include <IOKit/usb/StandardUSB.h>
#include <IOKit/usb/USB.h>

#include "DM9601Struct.h"

#define super IOEthernetController

struct EndpointRequest{
    uint8_t type;
    uint8_t direction;
    uint16_t maxPacketSize;
    uint8_t interval;
}__attribute__((packed));

typedef struct EndpointRequest EndpointRequest;

class DM9601V2 : public super {
    OSDeclareDefaultStructors(DM9601V2)
    
public:
    IOUSBHostDevice* DM9601Device;
public:
    //*********************************
    #pragma mark -
    #pragma mark IOKit Base Methods
    #pragma mark -
    //*********************************
    
    virtual bool init(OSDictionary *properties) APPLE_KEXT_OVERRIDE;
    
    virtual bool start(IOService* provider) APPLE_KEXT_OVERRIDE;
    
    virtual void free(void) APPLE_KEXT_OVERRIDE;
    
    virtual void stop(IOService *provider) APPLE_KEXT_OVERRIDE;
    
    void timeoutOccurred(IOTimerEventSource *timer);
    
    //*********************************
    #pragma mark -
    #pragma mark IOEthernetController Override Methods
    #pragma mark -
    //*********************************

    
    virtual IOReturn enable(IONetworkInterface* netif);
    //virtual IOReturn disable(IONetworkInterface *netif);
    //virtual IOReturn setWakeOnMagicPacket(bool active);
    //virtual IOReturn getPacketFilters(const OSSymbol    *group, uint32_t *filters ) const;
    //virtual IOReturn selectMedium(const IONetworkMedium *medium);
    virtual IOReturn getHardwareAddress(IOEthernetAddress *addr);
    //virtual IOReturn setMulticastMode(IOEnetMulticastMode mode);
    //virtual IOReturn setMulticastList(IOEthernetAddress *addrs, uint32_t count);
    //virtual IOReturn setPromiscuousMode(IOEnetPromiscuousMode mode);
    virtual IOOutputQueue* createOutputQueue();
    //virtual const OSString* newVendorString(void) const;
    //virtual const OSString* newModelString(void) const;
    //virtual const OSString* newRevisionString(void) const;
    //virtual bool configureInterface(IONetworkInterface *netif);
    
private:
    //*********************************
    #pragma mark -
    #pragma mark CDC Private Method
    #pragma mark -
    //*********************************

    IOReturn initDevice(uint8_t numConfigs);
    
    IOReturn configureDevice(uint8_t numConfigs);
    
    void releaseresource();
    
    bool createNetworkInterface();
    
    bool wakeUp();
    
    //void putToSleep();
    
    bool createMediumTables();
    
    bool bufferalloc();
    
    void bufferrelease();
    
    uint32_t outputPacket(mbuf_t pkt, void* param);
    
    bool USBTransmitPacket(mbuf_t packet);
    
   //bool USBSetMulticastFilter(IOEthernetAddress* addrs, uint32_t count);
    
    bool USBSetPacketFilter();
    
    IOReturn clearPipeStall(IOUSBHostPipe* thePipe);
    
    void receivePacket(uint8_t *packet, uint32_t size);
   
    static void commReadComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining);
    
    static void dataReadComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining);
    
    static void dataWriteComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining);
    
    static void merWriteComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining);
    
    static void statsWriteComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining);
    
    //*********************************
    #pragma mark -
    #pragma mark Helper Function
    #pragma mark -
    //*********************************

    IOUSBHostInterface* getinterface(IOUSBHostDevice* udev, uint8_t deviceclass, uint8_t devicesubclass);
    
    IOReturn ReadRegister(uint16_t reg, uint16_t size, uint8_t* buffer);
    
    IOReturn Write1Register(uint16_t reg, uint8_t value);
    
    IOReturn WriteRegister(uint16_t reg, uint16_t size, uint8_t* buffer);
    
    bool getFunctionalDescriptors();
    
    IOReturn GetStringDescriptor(IOUSBHostDevice* udev, char* stringBuffer, uint16_t index, int maxLen);
    
    IOUSBHostPipe* findpipe(IOUSBHostInterface* intf, IOUSBHostPipe* current, EndpointRequest* request);
    
private:
    IOEthernetInterface* fNetworkInterface;
    IOBasicOutputQueue* fTransmitQueue;

    IOLock* kextlock;
    
    IOWorkLoop* fWorkLoop;
    IONetworkStats* fpNetStats;
    IOEthernetStats* fpEtherStats;
    IOTimerEventSource* fTimerSource;
    
    OSDictionary* fMediumDict;
    
    IOUSBHostInterface* fCommInterface;
    IOUSBHostInterface* fDataInterface;
    
    IOUSBHostPipe* fInPipe;
    IOUSBHostPipe* fOutPipe;
    IOUSBHostPipe* fCommPipe;
    
    IOBufferMemoryDescriptor* fCommPipeMDP;
    IOBufferMemoryDescriptor* fPipeInMDP;

    IOUSBHostCompletion fCommCompletionInfo;
    IOUSBHostCompletion fReadCompletionInfo;
    IOUSBHostCompletion fWriteCompletionInfo;
    IOUSBHostCompletion fMERCompletionInfo;
    IOUSBHostCompletion fStatsCompletionInfo;
    
    pipeOutBuffers fPipeOutBuff[kOutBufPool];
    
    uint8_t fbmAttributes; // Device attributes
    uint16_t  fVendorID;
    uint16_t  fProductID;
    
    uint8_t* fCommPipeBuffer;
    uint8_t* fPipeInBuffer;
    
    uint8_t fLinkStatus;
    uint32_t fUpSpeed;
    uint32_t fDownSpeed;
    uint16_t fPacketFilter;
    
    uint8_t fCommInterfaceNumber;
    uint8_t fDataInterfaceNumber;
    uint32_t fCount;
    uint32_t fOutPacketSize;
    
    uint8_t fEaddr[6];
    uint16_t fMax_Block_Size;
    uint16_t fMcFilters;
    uint8_t fEthernetStatistics[4];
    
    uint16_t fCurrStat;
    uint32_t fStatValue;

    //*********************************
    #pragma mark -
    #pragma mark Some state flags
    #pragma mark -
    //*********************************
    
    bool fTerminate; // Are we being terminated (ie the device was unplugged)
    bool fReady;
    bool fNetifEnabled;
    bool fWOL;
    bool fDataDead;
    bool fCommDead;
    bool fStatInProgress;
    bool fInputPktsOK;
    bool fInputErrsOK;
    bool fOutputPktsOK;
    bool fOutputErrsOK;
};

#endif /* DM9601V2_h */
