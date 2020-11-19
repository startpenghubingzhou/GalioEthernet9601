//
//  DM9601V2.cpp
//  GalioEthernet9601
//
//  Created by PHBZ on 2020/11/17.
//  Copyright © 2020 penghubingzhou. All rights reserved.
//

#include "DM9601V2.hpp"


OSDefineMetaClassAndStructors(DM9601V2, super)

bool DM9601V2::init(OSDictionary *properties) {
    uint32_t i;
    
    if (!super::init(properties)) {
        IOLog("%s::init failed!\n", getName());
        return false;
    }
    
    // Set some defaults
    
    fMax_Block_Size = 0x1000;
    fCurrStat = 0;
    fStatInProgress = false;
    fDataDead = false;
    fCommDead = false;
    fPacketFilter = kPACKET_TYPE_DIRECTED | kPACKET_TYPE_BROADCAST | kPACKET_TYPE_MULTICAST;
    
    for (i=0; i<kOutBufPool; i++)
    {
        fPipeOutBuff[i].pipeOutMDP = nullptr;
        fPipeOutBuff[i].pipeOutBuffer = nullptr;
        fPipeOutBuff[i].m = nullptr;
    }
    
    return true;
}

bool DM9601V2::start(IOService* provider) {
    uint8_t configs;
    IOReturn ioret;
    
    if (!super::start(provider)) {
        IOLog("%s::start failed!\n", getName());
        goto start_error;
    }
    
    DM9601Device = OSDynamicCast(IOUSBHostDevice, provider);
    if (!DM9601Device) {
        IOLog("%s::casting provider failed!\n", getName());
        goto start_error;
    }
    
    kextlock = IOLockAlloc();
    if (!kextlock) {
        IOLog("%s::get kext lock failed!\n", getName());
        goto start_error;
    }
    
    IOLockLock(kextlock);
    
    IOLog("%s::Never fear, Galio comes here!\n", getName());
    
    fWorkLoop = getWorkLoop();
    
    if (!fWorkLoop) {
        IOLog("%s::Get workloop failed!\n", getName());
        goto start_error;
    }
    
    DM9601Device->setConfiguration(0);
    
    // Now take control of the device and configure it
    
    if (!DM9601Device->open(this))
    {
        IOLog("%s::unable to open device!\n", getName());
        goto start_error;
    }
    
    // Let's see if we have any configurations to play with
    
    configs = DM9601Device->getDeviceDescriptor()->bNumConfigurations;
    
    if (configs < 1)
    {
        IOLog("%s::no configurations!\n", getName());
        goto start_error;
    }
    
    IOLog("%s::the configuration number is %d!\n", getName(), configs);

    ioret = configureDevice(configs);
    if (ioret != kIOReturnSuccess)
    {
        IOLog("%s::configuring device failed, error code = %d!\n", getName(), ioret);
        goto start_error;
    }

    return true;
    
start_error:
    IOLog("%s::Got an unexpected error when loading! Now the driver will stop...\n", getName());
    stop(provider);
    return false;
}

IOReturn DM9601V2::initDevice(uint8_t numConfigs)
{
    const ConfigurationDescriptor* cd = nullptr;
    const InterfaceDescriptor* intf = nullptr;
    IOReturn ioret;
    uint8_t cval;
    uint8_t config = 0;
    
    //Since DM9601 have only one configuration defined in the protocol, we can simply set configuration number to 1 to get the host interface if not zero.
    ioret = DM9601Device->setConfiguration(1);
    if (ioret != kIOReturnSuccess)
    {
        IOLog("%s::Set configuration error! error code is: %d\n", getName(), ioret);
        return ioret;
    }
    
    cd = DM9601Device->getConfigurationDescriptor();
    
    IOUSBHostInterface* intfcandi = getinterface(DM9601Device, kUSBCompositeClass, kUSBCompositeSubClass);
    if (!intfcandi) {
        IOLog("%s::can't get the interface!\n", getName());
        return kIOReturnNotFound;
    }
    intf = intfcandi->getInterfaceDescriptor();
    if (intf)
    {
        IOLog("%s::Interface descriptor found!\n", getName());
        config = cd->bConfigurationValue;
        ioret = kIOReturnSuccess;
    } else {
        IOLog("%s::That's weird the interface descriptor was null...\n", getName());
        return kIOReturnNotFound;
    }
    
    fbmAttributes = cd->bmAttributes;
    IOLog("%s::The bmAttributes of interface configuration has been saved!\n", getName());
    
    // Save the ID's
    
    fVendorID = DM9601Device->getDeviceDescriptor()->idVendor;
    fProductID = DM9601Device->getDeviceDescriptor()->idProduct;
    
    
    return ioret;
}

IOReturn DM9601V2::configureDevice(uint8_t numConfigs)
{
    IOReturn ioret;
    const InterfaceDescriptor* altInterfaceDesc;
    uint16_t numends = 0;
    uint16_t alt;
    
    ioret = initDevice(numConfigs);
    if (ioret != kIOReturnSuccess)
    {
        IOLog("%s::Init device failed!\n", getName());
        return ioret;
    }
    
    // Get the Comm. Class interface
    fCommInterface = getinterface(DM9601Device, kUSBCompositeClass, kUSBCompositeSubClass);
    if (!fCommInterface)
    {
        IOLog("%s::Finding the first CDC interface failed!\n", getName());
        return kIOReturnError;
    }

#if 1
    uint8_t registerValue = 0;
    
    ioret = ReadRegister(RegNSR, sizeof(registerValue), &registerValue);
    if (ioret != kIOReturnSuccess)
        return ioret;
#else
    if (!getFunctionalDescriptors())
    {
        IOLog("%s::getFunctionalDescriptors failed!\n", getName());
        return false;
    }
#endif
    
    if (!fCommInterface->open(this))
    {
        IOLog("%s::open comm interface failed!\n", getName());
        return kIOReturnError;
    }
    
    fCommInterfaceNumber = fCommInterface->getInterfaceDescriptor()->bInterfaceNumber;
    
    fDataInterface = getinterface(DM9601Device, kUSBCompositeClass, kUSBCompositeSubClass);
    
    if (fDataInterface)
    {
        numends = fDataInterface->getInterfaceDescriptor()->bNumEndpoints;
        if (numends > 1)                    // There must be (at least) two bulk endpoints
        {
            IOLog("%s::Data Class interface found!\n", getName());
        } else {
            altInterfaceDesc = getNextInterfaceDescriptor(fDataInterface->getConfigurationDescriptor(), NULL);
            if (!altInterfaceDesc)
            {
                IOLog("%s::Get next interface failed!\n", getName());
            }
            while (altInterfaceDesc)
            {
                numends = altInterfaceDesc->bNumEndpoints;
                if (numends > 1)
                {
                    if (fDataInterface->open(this))
                    {
                        alt = altInterfaceDesc->bAlternateSetting;
                        IOLog("%s::Data Class interface (alternate) found!\n", getName());
                        ioret = fDataInterface->selectAlternateSetting(alt);
                        if (ioret == kIOReturnSuccess)
                        {
                            IOLog("%s::Alternate set!\n", getName());
                            break;
                        } else {
                            IOLog("%s::selectAlternateSetting failed!\n", getName());
                            numends = 0;
                        }
                    } else {
                        IOLog("%s::open data interface failed.\n", getName());
                        numends = 0;
                    }
                } else {
                    IOLog("%s::No endpoints in this alternate!\n", getName());
                }
                altInterfaceDesc = getNextInterfaceDescriptor(fDataInterface->getConfigurationDescriptor(), altInterfaceDesc);
            }
        }
    } else {
        IOLog("%s::getNextInterfaceDescriptor failed.\n", getName());
    }
    
    if (numends < 2)
    {
        IOLog("%s::Finding a Data Class interface failed.\n", getName());
        fCommInterface->close(this);
        fCommInterface = nullptr;
        return kIOReturnError;
    } else {
        
        fCommInterface->retain();
        fDataInterface->retain();
        
        // Found hqrwqre address
        
        IOLog("%s::Getting Hardware Address.\n", getName());
        
        ioret = ReadRegister(RegPAR, sizeof(fEaddr), fEaddr);
        if (ioret != kIOReturnSuccess)
        {
            IOLog("%s::Getting Hardware Address failed!\n", getName());
            return ioret;
        }
    
        // Found both so now let's publish the interface
    
        if (!createNetworkInterface())
        {
            IOLog("%s::createNetworkInterface failed!\n", getName());
            return kIOReturnInvalid;
        }
    }
    
    return kIOReturnSuccess;
}
    
void DM9601V2::stop(IOService *provider) {
    IOLog("%s::Never fear, Galio will come soon!\n", getName());
    
    releaseresource();
    
    super::stop(provider);
}

void DM9601V2::free(void) {
    super::free();
}

void DM9601V2::releaseresource(){
    if (fNetworkInterface)
    {
        OSSafeReleaseNULL(fNetworkInterface);
    }
    
    
    if (fCommInterface)
    {
        fCommInterface->close(this);
        OSSafeReleaseNULL(fCommInterface);
    }
    
    if (fDataInterface)
    {
        // disable RX
        uint8_t control;
        
        if (ReadRegister(RegRCR, sizeof(control), &control) == kIOReturnSuccess)
        {
            control &= ~RCRRXEnable;
            Write1Register(RegRCR, control);
        }
        
        fDataInterface->close(this);
        OSSafeReleaseNULL(fDataInterface);
    }
    
    if (DM9601Device)
    {
        DM9601Device->close(this);
        OSSafeReleaseNULL(DM9601Device);
    }
    
    if (fMediumDict)
    {
        fMediumDict->release();
        fMediumDict = nullptr;
    }
    
    if (fTimerSource) {
        fTimerSource->disable();
        fWorkLoop->removeEventSource(fTimerSource);
        OSSafeReleaseNULL(fTimerSource);
    }
    
    if (fWorkLoop) {
        OSSafeReleaseNULL(fWorkLoop);
    }
    
    if (kextlock) {
        IOLockUnlock(kextlock);
        IOLockFree(kextlock);
    }
}

IOUSBHostInterface* DM9601V2::getinterface(IOUSBHostDevice* udev, uint8_t deviceclass, uint8_t devicesubclass){
    OSIterator* iterator;
    IOUSBHostInterface* interfaceCandidate;
    OSObject* candidate = NULL;
    
    iterator = udev->getChildIterator(gIOServicePlane);
    
    while(iterator != NULL && (candidate = iterator->getNextObject()) != NULL)
    {
        interfaceCandidate = OSDynamicCast(IOUSBHostInterface, candidate);
        if(interfaceCandidate != NULL && interfaceCandidate->getInterfaceDescriptor()->bInterfaceClass == deviceclass && interfaceCandidate->getInterfaceDescriptor()->bInterfaceSubClass == devicesubclass)
        {
            break;
        }
    }
    OSSafeReleaseNULL(iterator);
    
    return interfaceCandidate;
}

IOReturn DM9601V2::ReadRegister(uint16_t reg, uint16_t size, uint8_t* buffer)
{
    DeviceRequest devreq;
    uint32_t wLenDone = 0; //bytes transferred in the request.
    IOReturn ioret = kIOReturnSuccess;
    
    IOLog("%s::ReadRegister start!\n", getName());
    
    if (size > 255)
        return kIOReturnBadArgument;

    devreq.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBVendor, kUSBDevice);
    devreq.bRequest = kVenReqReadRegister;
    devreq.wValue = 0;
    devreq.wIndex = reg;
    devreq.wLength = size;
    
    ioret = DM9601Device->deviceRequest(DM9601Device, devreq, buffer, wLenDone);
    if (ioret != kIOReturnSuccess)
    {
        IOLog("%s::Device request error!\n", getName());
        if (ioret == kIOUSBPipeStalled)
        {
            // Clear the stall and try it once more
            DM9601Device->abortDeviceRequests(DM9601Device, IOUSBHostIOSource::kAbortSynchronous);
            ioret = DM9601Device->deviceRequest(DM9601Device, devreq, buffer, wLenDone);
            if (ioret != kIOReturnSuccess)
            {
                IOLog("%s::Device request, error a second time!\n", getName());
                return ioret;
            }
        }
    }
    
    if (size != wLenDone)
    {
        IOLog("%s::Size mismatch reading register!\n", getName());
        return kIOReturnUnderrun;
    }
    
    return ioret;
}

IOReturn DM9601V2::WriteRegister(uint16_t reg, uint16_t size, uint8_t* buffer){
    DeviceRequest devreq;
    uint32_t wLenDone = 0;
    IOReturn ioret = kIOReturnSuccess;
    
    IOLog("%s::WriteRegister start!\n", getName());
    
    if (size > 255)
        return kIOReturnBadArgument;
    
    devreq.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
    devreq.bRequest = kVenReqWriteRegister;
    devreq.wValue = 0;
    devreq.wIndex = reg;
    devreq.wLength = size;
    
    ioret = DM9601Device->deviceRequest(DM9601Device, devreq, buffer, wLenDone);
    
    if (kIOReturnSuccess == ioret && wLenDone != size)
    {
        ioret = kIOReturnUnderrun;
    }
    
    if (ioret != kIOReturnSuccess)
    {
        IOLog("%s::Device request error when writereg!\n", getName());
    }
    
    return ioret;
}

IOReturn DM9601V2::Write1Register(uint16_t reg, uint8_t value)
{
    DeviceRequest devreq;
    uint32_t wLenDone = 0;
    IOReturn ioret = kIOReturnSuccess;
    
    IOLog("%s::Write1Register start!\n", getName());
    
    devreq.bmRequestType = USBmakebmRequestType(kUSBOut, kUSBVendor, kUSBDevice);
    devreq.bRequest = kVenReqWriteRegisterByte;
    devreq.wValue = value;
    devreq.wIndex = reg;
    devreq.wLength = 0;
    //devreq.pData = NULL;
    //devreq.wLenDone = 0;
    
    ioret = DM9601Device->deviceRequest(DM9601Device, devreq, (void*)NULL, wLenDone);
    if (ioret != kIOReturnSuccess)
    {
        IOLog("%s::DeviceRequest for write1reg error!\n", getName());
        if (ioret == kIOUSBPipeStalled)
        {
            // Clear the stall and try it once more
            DM9601Device->abortDeviceRequests(DM9601Device, IOUSBHostIOSource::kAbortSynchronous);//GetPipeZero()->ClearPipeStall(false);
            ioret = DM9601Device->deviceRequest(DM9601Device, devreq, (void*)NULL, wLenDone);
            if (ioret != kIOReturnSuccess)
            {
                IOLog("%s::DeviceRequest error a second time when write1reg!\n", getName());
                return ioret;
            }
        }
    }
    
    return ioret;
}

bool DM9601V2::getFunctionalDescriptors()
{
    bool gotDescriptors = false;
    bool configok = true;
    bool enet = false;
    IOReturn ioret;
    const HeaderFunctionalDescriptor* funcDesc = nullptr;
    EnetFunctionalDescriptor* ENETFDesc = nullptr;

    do
    {
        funcDesc = (const HeaderFunctionalDescriptor*)getNextAssociatedDescriptorWithType(fCommInterface->getConfigurationDescriptor(), fCommInterface->getInterfaceDescriptor(), (Descriptor*)funcDesc, CS_INTERFACE);
        if (!funcDesc)
        {
            gotDescriptors = true;
        } else {
            switch (funcDesc->bDescriptorSubtype)
            {
                case Header_FunctionalDescriptor:
                    IOLog("%s::getFunctionalDescriptors - Header Functional Descriptor.\n", getName());
                    break;
                case Enet_Functional_Descriptor:
                    ENETFDesc = (EnetFunctionalDescriptor *)funcDesc;
                    IOLog("%s::Ethernet Functional Descriptor.\n", getName());
                    enet = true;
                    break;
                case Union_FunctionalDescriptor:
                    IOLog("%s::Union Functional Descriptor.\n", getName());
                    break;
                default:
                    IOLog("%s::unknown Functional Descriptor.\n", getName());
                    break;
            }
        }
    } while(!gotDescriptors);
    
    if (!enet)
    {
        configok = false;                    // The Enet Func. Desc.  must be present
    } else {
        
        // Determine who is collecting the input/output network stats.
        
        if (!(ENETFDesc->bmEthernetStatistics[0] & kXMIT_OK))
        {
            fOutputPktsOK = true;
        } else {
            fOutputPktsOK = false;
        }
        if (!(ENETFDesc->bmEthernetStatistics[0] & kRCV_OK))
        {
            fInputPktsOK = true;
        } else {
            fInputPktsOK = false;
        }
        if (!(ENETFDesc->bmEthernetStatistics[0] & kXMIT_ERROR))
        {
            fOutputErrsOK = true;
        } else {
            fOutputErrsOK = false;
        }
        if (!(ENETFDesc->bmEthernetStatistics[0] & kRCV_ERROR))
        {
            fInputErrsOK = true;
        } else {
            fInputErrsOK = false;
        }
        
        // Save the stats (it's bit mapped)
        
        fEthernetStatistics[0] = ENETFDesc->bmEthernetStatistics[0];
        fEthernetStatistics[1] = ENETFDesc->bmEthernetStatistics[1];
        fEthernetStatistics[2] = ENETFDesc->bmEthernetStatistics[2];
        fEthernetStatistics[3] = ENETFDesc->bmEthernetStatistics[3];
        
        // Save the multicast filters (remember it's intel format)
        
        fMcFilters = USBToHostWord((uint16_t)ENETFDesc->wNumberMCFilters[0]);
        
        // Get the Ethernet address
        
        if (ENETFDesc->iMACAddress != 0)
        {
            ioret = GetStringDescriptor(DM9601Device, (char *)&fEaddr, ENETFDesc->iMACAddress, 6);
            if (ioret == kIOReturnSuccess)
            {
                fMax_Block_Size = USBToHostWord((UInt16)ENETFDesc->wMaxSegmentSize[0]);
                IOLog("%s::Maximum segment size.\n", getName());
            } else {
                IOLog("%s::Error retrieving Ethernet address\n", getName());
                configok = false;
            }
        } else {
            configok = false;
        }
    }
    
    return configok;
}

IOReturn DM9601V2::GetStringDescriptor(IOUSBHostDevice* udev, char* buf, uint16_t index, int maxLen)
{
    char stringBuffer[256] = { 0 };
    size_t stringLength = sizeof(stringBuffer);
    const StringDescriptor* stringDescriptor = udev->getStringDescriptor(index);
    IOReturn ioret;
    int i;
    
    if(stringDescriptor != NULL && stringDescriptor->bLength > StandardUSB::kDescriptorSize)
    {
        ioret = StandardUSB::stringDescriptorToUTF8(stringDescriptor, stringBuffer, stringLength);
    }else{
        ioret = kIOReturnNotFound;
    }
    
    for (i = 0; i <= maxLen; i++) {
        buf[i] = stringBuffer[i];
    }
    
    return ioret;
}
bool DM9601V2::createNetworkInterface()
{
    // Allocate memory for buffers etc
    
    fTransmitQueue = (IOGatedOutputQueue*)getOutputQueue();
    if (!fTransmitQueue)
    {
        IOLog("%s::Output queue initialization failed!\n", getName());
        return false;
    }
    fTransmitQueue->retain();
    
    // Allocate Timer event source
    fTimerSource = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &DM9601V2::timeoutOccurred));
    if (fTimerSource == NULL)
    {
        IOLog("%s::Allocate Timer event source failed!\n", getName());
        return false;
    }
    
    if (fWorkLoop->addEventSource(fTimerSource) != kIOReturnSuccess)
    {
        IOLog("%s::Add Timer event source failed!\n", getName());
        return false;
    }
    
    // Attach an IOEthernetInterface client
    
    IOLog("%s::attaching and registering interface...\n", getName());
    
    if (!attachInterface((IONetworkInterface**)&fNetworkInterface, true))
    {
        IOLog("%s::attachInterface failed!\n", getName());
        return false;
    }
    
    // Ready to service interface requests
    
    fNetworkInterface->registerService();
    
    IOLog("%s::create networkinterface succeed!\n", getName());
    
    return true;
    
}
void DM9601V2::timeoutOccurred(IOTimerEventSource* timer)
{
    uint32_t* enetStats;
    uint16_t currStat;
    IOReturn rc;
    DeviceRequest* STREQ;
    bool statOk = false;
    
    IOLog("%s::timeoutOccurred!\n", getName());
    
    enetStats = (uint32_t*)&fEthernetStatistics;
    if (*enetStats == 0)
    {
        IOLog("%s::No Ethernet statistics defined!\n", getName());
        return;
    }
    
    if (fReady == false)
    {
        IOLog("%s::Spurious!\n", getName());
    } else {
        
        // Only do it if it's not already in progress
        
        if (!fStatInProgress)
        {
            
            // Check if the stat we're currently interested in is supported
            
            currStat = stats[fCurrStat++];
            if (fCurrStat >= numStats)
            {
                fCurrStat = 0;
            }
            switch(currStat)
            {
                case kXMIT_OK_REQ:
                    if (fEthernetStatistics[0] & kXMIT_OK)
                    {
                        statOk = true;
                    }
                    break;
                case kRCV_OK_REQ:
                    if (fEthernetStatistics[0] & kRCV_OK)
                    {
                        statOk = true;
                    }
                    break;
                case kXMIT_ERROR_REQ:
                    if (fEthernetStatistics[0] & kXMIT_ERROR_REQ)
                    {
                        statOk = true;
                    }
                    break;
                case kRCV_ERROR_REQ:
                    if (fEthernetStatistics[0] & kRCV_ERROR_REQ)
                    {
                        statOk = true;
                    }
                    break;
                case kRCV_CRC_ERROR_REQ:
                    if (fEthernetStatistics[2] & kRCV_CRC_ERROR)
                    {
                        statOk = true;
                    }
                    break;
                case kRCV_ERROR_ALIGNMENT_REQ:
                    if (fEthernetStatistics[2] & kRCV_ERROR_ALIGNMENT)
                    {
                        statOk = true;
                    }
                    break;
                case kXMIT_ONE_COLLISION_REQ:
                    if (fEthernetStatistics[2] & kXMIT_ONE_COLLISION)
                    {
                        statOk = true;
                    }
                    break;
                case kXMIT_MORE_COLLISIONS_REQ:
                    if (fEthernetStatistics[2] & kXMIT_MORE_COLLISIONS)
                    {
                        statOk = true;
                    }
                    break;
                case kXMIT_DEFERRED_REQ:
                    if (fEthernetStatistics[2] & kXMIT_DEFERRED)
                    {
                        statOk = true;
                    }
                    break;
                case kXMIT_MAX_COLLISION_REQ:
                    if (fEthernetStatistics[2] & kXMIT_MAX_COLLISION)
                    {
                        statOk = true;
                    }
                    break;
                case kRCV_OVERRUN_REQ:
                    if (fEthernetStatistics[3] & kRCV_OVERRUN)
                    {
                        statOk = true;
                    }
                    break;
                case kXMIT_TIMES_CARRIER_LOST_REQ:
                    if (fEthernetStatistics[3] & kXMIT_TIMES_CARRIER_LOST)
                    {
                        statOk = true;
                    }
                    break;
                case kXMIT_LATE_COLLISIONS_REQ:
                    if (fEthernetStatistics[3] & kXMIT_LATE_COLLISIONS)
                    {
                        statOk = true;
                    }
                    break;
                default:
                    break;
            }
        }
        
        if (statOk)
        {
            STREQ = (DeviceRequest*)IOMalloc(sizeof(DeviceRequest));
            if (!STREQ)
            {
                IOLog("%s::allocate STREQ failed!\n", getName());
            } else {
                bzero(STREQ, sizeof(DeviceRequest));
                
                // Now build the Statistics Request
                
                STREQ->bmRequestType = USBmakebmRequestType(kUSBOut, kUSBClass, kUSBInterface);
                STREQ->bRequest = kGet_Ethernet_Statistics;
                STREQ->wValue = currStat;
                STREQ->wIndex = fCommInterfaceNumber;
                STREQ->wLength = 4;
                
                fStatsCompletionInfo.parameter = STREQ;
                
                rc = DM9601Device->deviceRequest(DM9601Device, *STREQ, &fStatValue, &fStatsCompletionInfo);
                if (rc != kIOReturnSuccess)
                {
                    IOLog("%s::Error issueing DeviceRequest!\n", getName());
                    IOFree(STREQ, sizeof(DeviceRequest));
                } else {
                    fStatInProgress = true;
                }
            }
        }
    }
    
    // Restart the watchdog timer
    
    fTimerSource->setTimeoutMS(WATCHDOG_TIMER_MS);
    
}

IOOutputQueue* DM9601V2::createOutputQueue()
{    
    return IOBasicOutputQueue::withTarget(this, TRANSMIT_QUEUE_SIZE);
    
}

IOReturn DM9601V2::getHardwareAddress(IOEthernetAddress *ea)
{
    return kIOReturnSuccess;
}
