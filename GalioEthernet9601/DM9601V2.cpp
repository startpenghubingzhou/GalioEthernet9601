//
//  DM9601V2.cpp
//  GalioEthernet9601
//
//  Created by PHBZ on 2020/11/17.
//  Copyright © 2020 penghubingzhou. All rights reserved.
//

#include "DM9601V2.hpp"


OSDefineMetaClassAndStructors(DM9601V2, super)

//*********************************
#pragma mark -
#pragma mark IOKit Base Methods
#pragma mark -
//*********************************

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
    
void DM9601V2::stop(IOService *provider) {
    IOLog("%s::Never fear, Galio will come soon!\n", getName());
    
    releaseresource();
    
    super::stop(provider);
}

void DM9601V2::free(void) {
    super::free();
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
//*********************************
#pragma mark -
#pragma mark CDC Private Method
#pragma mark -
//*********************************

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
        fTimerSource->cancelTimeout();
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

bool DM9601V2::wakeUp()
{
    IOReturn ioret = kIOReturnSuccess;
    
    IOLog("%s::wakeUp!\n", getName());
    
    fReady = false;
    
    if (fTimerSource)
    {
        fTimerSource->cancelTimeout();
    }
    
    setLinkStatus(0, 0);                // Initialize the link state
    
    /*if (fbmAttributes & kUSBAtrBusPowered)
    {
        rtn = fpDevice->SuspendDevice(false);        // Resume the device
        if (rtn != kIOReturnSuccess)
        {
            return false;
        }
    }*/
    
    IOSleep(50);
    
    if (!bufferalloc())
    {
        return false;
    }
    
    // Initialize RX control register, enable RX
    uint8_t control = 0;
    ioret = ReadRegister(RegRCR, sizeof(control), &control);
    if (ioret != kIOReturnSuccess)
    {
        bufferrelease();
        return false;
    }
    control &= ~RCRPromiscuous;
    control |= RCRDiscardLong | RCRDiscardCRC | RCRRXEnable;
    ioret = Write1Register(RegRCR, control);
    if (ioret != kIOReturnSuccess)
    {
        bufferrelease();
        return false;
    }
    
    // Read the comm interrupt pipe for status:
    
    fCommCompletionInfo.owner= this;
    fCommCompletionInfo.action = commReadComplete;
    fCommCompletionInfo.parameter = NULL;

    if (fCommPipe)
    {
        ioret = fCommPipe->io(fCommPipeMDP, NULL, &fCommCompletionInfo);
    }
    if (ioret == kIOReturnSuccess)
    {
        // Read the data-in bulk pipe:
        
        fReadCompletionInfo.owner = this;
        fReadCompletionInfo.action = dataReadComplete;
        fReadCompletionInfo.parameter = NULL;
        
        if (!fPipeInMDP) {
             IOLog("%s::Got NULL, exiting!\n", getName());
            return false;
        }
        
        //ioret = fCommPipe->io(fPipeInMDP, NULL, &fReadCompletionInfo);
        
        if (ioret == kIOReturnSuccess)
        {
            // Set up the data-out bulk pipe:
            
            fWriteCompletionInfo.owner = this;
            fWriteCompletionInfo.action = dataWriteComplete;
            fWriteCompletionInfo.parameter = NULL;                // for now, filled in with the mbuf address when sent
            
            // Set up the management element request completion routine:
            
            fMERCompletionInfo.owner = this;
            fMERCompletionInfo.action = merWriteComplete;
            fMERCompletionInfo.parameter = NULL;                // for now, filled in with parm block when allocated
            
            // Set up the statistics request completion routine:
            
            fStatsCompletionInfo.owner = this;
            fStatsCompletionInfo.action = statsWriteComplete;
            fStatsCompletionInfo.parameter = NULL;                // for now, filled in with parm block when allocated
        }
    }
    
    if (ioret != kIOReturnSuccess)
    {
        
        // We failed for some reason
        IOLog("%s::Setting up the pipes failed!\n", getName());
        bufferrelease();
        return false;
    } else {
        if (!fMediumDict)
        {
            if (!createMediumTables())
            {
                IOLog("%s::createMediumTables failed!\n", getName());
                bufferrelease();
                return false;
            }
        }
        
        fTimerSource->setTimeoutMS(WATCHDOG_TIMER_MS);
        fReady = true;
    }
    
    return true;
}

bool DM9601V2::createMediumTables()
{
    IONetworkMedium* medium;
    uint64_t maxSpeed;
    uint32_t i;
    
    maxSpeed = 100;
    fMediumDict = OSDictionary::withCapacity(sizeof(mediumTable) / sizeof(mediumTable[0]));
    if (fMediumDict == 0)
    {
        IOLog("%s::create dict failed!\n", getName());
        return false;
    }
    
    for (i = 0; i < sizeof(mediumTable) / sizeof(mediumTable[0]); i++ )
    {
        medium = IONetworkMedium::medium(mediumTable[i].type, mediumTable[i].speed);
        if (medium && (medium->getSpeed() <= maxSpeed))
        {
            IONetworkMedium::addMedium(fMediumDict, medium);
            medium->release();
        }
    }
    
    if (publishMediumDictionary(fMediumDict) != true)
    {
        IOLog("%s::publish dict. failed!\n", getName());
        return false;
    }
    
    medium = IONetworkMedium::getMediumWithType(fMediumDict, kIOMediumEthernetAuto);
    setCurrentMedium(medium);
    
    return true;
    
}/* end createMediumTables */

bool DM9601V2::bufferalloc()
{
    EndpointRequest epReq;        // endPoint request struct on stack
    uint32_t i;
    
    // Open all the end points
    
    epReq.type = kUSBBulk;
    epReq.direction = kUSBIn;
    epReq.maxPacketSize    = 0x40;
    epReq.interval = 0;
    
    fInPipe = findpipe(fDataInterface, NULL, &epReq);
    if (!fInPipe)
    {
        IOLog("%s::no bulk input pipe.\n", getName());
        return false;
    }
    IOLog("%s::bulk input pipe.\n", getName());
    
    epReq.direction = kUSBOut;
    fOutPipe = findpipe(fDataInterface, NULL, &epReq);
    if (!fOutPipe)
    {
        IOLog("%s::no bulk output pipe.\n", getName());
        return false;
    }
    fOutPacketSize = epReq.maxPacketSize;
    IOLog("%s:: bulk output pipe.\n", getName());
    
    // Interrupt pipe - Comm Interface
    
    epReq.type = kUSBInterrupt;
    epReq.direction = kUSBIn;
    fCommPipe = findpipe(fCommInterface, NULL, &epReq);
    if (!fCommPipe)
    {
        IOLog("%s::no interrupt in pipe.\n", getName());
        fCommPipeMDP = NULL;
        fCommPipeBuffer = NULL;
        //        return false;
    } else {
        IOLog("%s::comm pipe.\n", getName());
        
        // Allocate Memory Descriptor Pointer with memory for the Comm pipe:
        
        fCommPipeMDP = IOBufferMemoryDescriptor::withCapacity(COMM_BUFF_SIZE, kIODirectionIn);
        if (!fCommPipeMDP)
            return false;
        
        fCommPipeMDP->setLength(COMM_BUFF_SIZE);
        fCommPipeBuffer = (uint8_t*)fCommPipeMDP->getBytesNoCopy();
        IOLog("%s::comm buffer\n", getName());
    }
    
    // Allocate Memory Descriptor Pointer with memory for the data-in bulk pipe:
    
    fPipeInMDP = IOBufferMemoryDescriptor::withCapacity(fMax_Block_Size, kIODirectionIn);
    if (!fPipeInMDP)
        return false;
    
    fPipeInMDP->setLength(fMax_Block_Size);
    fPipeInBuffer = (uint8_t*)fPipeInMDP->getBytesNoCopy();
    IOLog("%s::input buffer\n", getName());
    
    // Allocate Memory Descriptor Pointers with memory for the data-out bulk pipe pool
    
    for (i=0; i<kOutBufPool; i++)
    {
        fPipeOutBuff[i].pipeOutMDP = IOBufferMemoryDescriptor::withCapacity(fMax_Block_Size, kIODirectionOut);
        if (!fPipeOutBuff[i].pipeOutMDP)
        {
            IOLog("%s::Allocate output descriptor failed!\n", getName());
            return false;
        }
        
        fPipeOutBuff[i].pipeOutMDP->setLength(fMax_Block_Size);
        fPipeOutBuff[i].pipeOutBuffer = (uint8_t*)fPipeOutBuff[i].pipeOutMDP->getBytesNoCopy();
        IOLog("%s::output buffer\n", getName());
    }
    
    return true;
    
}

void DM9601V2::bufferrelease()
{
    uint32_t i;
    
    for (i=0; i<kOutBufPool; i++)
    {
        if (fPipeOutBuff[i].pipeOutMDP)
        {
            fPipeOutBuff[i].pipeOutMDP->release();
            fPipeOutBuff[i].pipeOutMDP = NULL;
        }
    }
    
    if (fPipeInMDP  )
    {
        fPipeInMDP->release();
        fPipeInMDP = 0;
    }
    
    if (fCommPipeMDP)
    {
        fCommPipeMDP->release();
        fCommPipeMDP = 0;
    }
    
}

bool DM9601V2::USBTransmitPacket(mbuf_t packet)
{
    uint32_t numbufs = 0; // number of mbufs for this packet
    mbuf_t m;                // current mbuf
    uint32_t total_pkt_length = 0;
    uint32_t rTotal = 0;
    IOReturn  ioret = kIOReturnSuccess;
    uint32_t poolIndx;
    bool gotBuffer = false;
    uint16_t tryCount = 0;
    
    // Count the number of mbufs in this packet
    
    m = packet;
    while (m)
    {
        total_pkt_length += mbuf_len(m);
        numbufs++;
        m = mbuf_next(m);
    }
    
    IOLog("%s::Total packet length and Number of mbufs: %04h, %04h\n", getName(), total_pkt_length, numbufs);
    
    if (total_pkt_length > fMax_Block_Size)
    {
        IOLog("%s::Bad packet size!\n", getName());    // Note for now and revisit later
        if (fOutputErrsOK)
            fpNetStats->outputErrors++;
        return false;
    }
    
    // Find an ouput buffer in the pool
    
    while (!gotBuffer)
    {
        for (poolIndx=0; poolIndx<kOutBufPool; poolIndx++)
        {
            if (fPipeOutBuff[poolIndx].m == NULL)
            {
                gotBuffer = true;
                break;
            }
        }
        if (gotBuffer)
        {
            IOLog("%s::%d Output buffer found!\n", getName(), poolIndx);
            break;
        } else {
            tryCount++;
            if (tryCount > kOutBuffThreshold)
            {
                IOLog("%s::Exceeded output buffer wait threshold.\n", getName());
                if (fOutputErrsOK)
                    fpNetStats->outputErrors++;
                return false;
            } else {
                IOLog("%s::Waiting for output buffer %d.\n", getName(), tryCount);
                IOSleep(1);
            }
        }
    }
    
    // Start filling in the send buffer
    
    m = packet;                            // start with the first mbuf of the packet
    rTotal = 2;                            // running total
    do
    {
        if (mbuf_len(m) == 0)                    // Ignore zero length mbufs
            continue;
        
        bcopy(mbuf_data(m), &fPipeOutBuff[poolIndx].pipeOutBuffer[rTotal], mbuf_len(m));
        rTotal += mbuf_len(m);
        
    } while ((m = mbuf_next(m)) != 0);
    
    // additional padding byte must be transmitted in case data size
    // to be send is multiple of pipe's max packet size
    if ((rTotal % 0x40) == 0)
    {
        IOLog("%s::Additional padding byte added %d!\n", getName(), rTotal);
        rTotal++;
        fPipeOutBuff[poolIndx].pipeOutBuffer[rTotal] = 0;
    }
    
    IOLog("%s::Filling the send buffer: %04h %04h\n", getName(), total_pkt_length, rTotal);
    
    uint32_t tmp = rTotal-2;
    fPipeOutBuff[poolIndx].pipeOutBuffer[0] = (uint8_t)(tmp & 0xff);
    fPipeOutBuff[poolIndx].pipeOutBuffer[1] = (uint8_t)((tmp >> 8) & 0xff);
    
    
    fPipeOutBuff[poolIndx].m = packet;
    fWriteCompletionInfo.parameter = (void *)poolIndx;
    fPipeOutBuff[poolIndx].pipeOutMDP->setLength(rTotal);
    ioret = fOutPipe->io(fPipeOutBuff[poolIndx].pipeOutMDP, rTotal, &fWriteCompletionInfo);
    //ioret = fOutPipe->Write(fPipeOutBuff[poolIndx].pipeOutMDP, &fWriteCompletionInfo);
    if (ioret != kIOReturnSuccess)
    {
        IOLog("%d::Write failed!\n", getName());
        if (ioret == kIOUSBPipeStalled)
        {
            fOutPipe->clearStall(false);
            ioret = fOutPipe->io(fPipeOutBuff[poolIndx].pipeOutMDP, rTotal,&fWriteCompletionInfo);
            //ioret = fOutPipe->Write(fPipeOutBuff[poolIndx].pipeOutMDP, &fWriteCompletionInfo);
            if (ioret != kIOReturnSuccess)
            {
                IOLog("%s::Write really failed!\n", getName());
                if (fOutputErrsOK)
                    fpNetStats->outputErrors++;
                return false;
            }
        }
    }
    
    IOLog("%s::Write succeeded!\n", getName());
    
    if (fOutputPktsOK)
        fpNetStats->outputPackets++;
    
    return true;
    
}


uint32_t DM9601V2::outputPacket(mbuf_t pkt, void* param)
{
    uint32_t ret = kIOReturnOutputSuccess;
    
    IOLog("%s::outputPacket start!\n", getName());
    
    if (!fLinkStatus)
    {
        IOLog("%s::link is down!\n", getName());
        if (fOutputErrsOK)
            fpNetStats->outputErrors++;
        freePacket(pkt);
    } else {
        if (USBTransmitPacket(pkt) == false)
        {
            ret = kIOReturnOutputStall;
        }
    }
    
    return ret;
}

bool DM9601V2::USBSetPacketFilter()
{
    IOReturn rc;
    uint8_t control = 0;
    
    rc = ReadRegister(RegRCR, sizeof(control), &control);
    if (rc != kIOReturnSuccess)
    {
        IOLog("%s::Error reading control!\n", getName());
        return false;
    }
    
    if (fPacketFilter & kPACKET_TYPE_PROMISCUOUS)
        control |= RCRPromiscuous;
    else
        control &= ~RCRPromiscuous;
    
    if (fPacketFilter & kPACKET_TYPE_ALL_MULTICAST)
        control |= RCRAllMulticast;
    else
        control &= ~RCRAllMulticast;
    
    rc = Write1Register(RegRCR, control);
    if (rc != kIOReturnSuccess)
    {
        IOLog("%s::Error writing control!\n", getName());
        return false;
    }
    
    return true;
}


IOReturn DM9601V2::clearPipeStall(IOUSBHostPipe* thePipe)
{
    uint8_t pipeStatus;
    IOReturn rtn = kIOReturnSuccess;

    //GetStatus will always return 0(aka pipeStatus will be always not equal to kPipeStalled). See MacOSX10.10.sdk\IOUSBPipe.h Line94
    //pipeStatus = thePipe->GetStatus();
    
    //if (pipeStatus == kPipeStalled)
   // {
    
        rtn = thePipe->clearStall(true);
        if (rtn == kIOReturnSuccess)
        {
            IOLog("%s::clear stall successful!\n", getName());
        } else {
            IOLog("%s::clear stall failed!\n", getName());
        }
   // } else {
        //IOLog("%s::Pipe not stalled!\n", getName());
    //}
    
    return rtn;
}

void DM9601V2::receivePacket(uint8_t* packet, uint32_t size)
{
    mbuf_t m;
    uint32_t submit;
    uint32_t length;
    uint8_t* ptr = packet;
    
    if (size > fMax_Block_Size)
    {
        IOLog("%s::Packet size error, packet dropped!\n", getName());
        if (fInputErrsOK)
            fpNetStats->inputErrors++;
        return;
    }
    
    //while (size >0)
    //{
    if (*ptr != 0x40)
    {
        IOLog("%s::Packet size error, packet dropped -2 !\n", getName());
        if (fInputErrsOK)
            fpNetStats->inputErrors++;
        return;
    }
    
    length = ptr[1] | (ptr[2] << 8);
    ptr += 3;
    size -= 3;
    
    IOLog("%s::Packet length: %d\n", getName(), length);
    
    m = allocatePacket(length);
    if (m)
    {
        bcopy(ptr, mbuf_data(m), length);
        submit = fNetworkInterface->inputPacket(m, length);
        IOLog("%s::Packets submitted %d!\n", getName(), submit);
        if (fInputPktsOK)
            fpNetStats->inputPackets++;
    } else {
        IOLog("%s::Buffer allocation failed, packet dropped!\n", getName());
        if (fInputErrsOK)
            fpNetStats->inputErrors++;
    }
    //  size -= length;
    //  ptr += length;
    //}
    
}

void DM9601V2::commReadComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining)
{
    DM9601V2* me = (DM9601V2*)obj;
    IOReturn ior;
    uint8_t notif, status;
    
    IOLog("DM9601V2::commReadComplete\n");
    
    if (ioret == kIOReturnSuccess)    // If operation returned ok
    {
        IOLog("DM9601V2::commReadComplete succeed!\n");
        status = me->fCommPipeBuffer[0];
        if (status & 0x40)
        {
            me->fLinkStatus = 1;
            me->setLinkStatus(0);
        }
        else
        {
            me->fLinkStatus = 0;
            me->setLinkStatus(1);
        }
        notif = me->fCommPipeBuffer[1];
        if (!(notif & kResponse_Available))
        {
            uint8_t control = 0;
            
            control |= RCRDiscardLong | RCRDiscardCRC | RCRRXEnable;
            me->Write1Register(RegRCR, control); // 0x31
            
            control &= ~RCRRXEnable;
            me->Write1Register(RegRCR, control); // 0x30
            
            control |= RCRRXEnable;
            me->Write1Register(RegRCR, control); // 0x31
        }
    }
    else if (ioret == kIOReturnAborted)
    {
        return;
    }
    
    // Queue the next read, only if not aborted
    ior = me->fCommPipe->io(me->fCommPipeMDP, NULL, &me->fCommCompletionInfo);
    if (ior != kIOReturnSuccess)
    {
        IOLog("DM9601V2::Failed to queue next read!\n");
        if (ior == kIOUSBPipeStalled)
        {
            me->fCommPipe->clearStall(false);
            ior = me->fCommPipe->io(me->fCommPipeMDP, NULL, &me->fCommCompletionInfo);//Read(me->fCommPipeMDP, &me->fCommCompletionInfo, NULL);
            if (ior != kIOReturnSuccess)
            {
                IOLog("DM9601V2::Failed, read dead!\n");
                me->fCommDead = true;
            }
        }
    }
    
    return;
    
}

void DM9601V2::dataReadComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining)
{
    DM9601V2* me = (DM9601V2*)obj;
    IOReturn  ior;
    
    IOLog("DM9601V2::dataReadComplete\n");
    if (ioret == kIOReturnSuccess)    // If operation returned ok
    {
        IOLog("DM9601V2::Moving the incoming bytes up the stack %d, %d\n",me->fMax_Block_Size, remaining);
        
        // Move the incoming bytes up the stack
        
        me->receivePacket(me->fPipeInBuffer, me->fMax_Block_Size - remaining);
        
    } else {
        IOLog("DM9601V2::Read completion io err %04h", ioret);
        if (ioret != kIOReturnAborted)
        {
            ioret = me->clearPipeStall(me->fInPipe);
            if (ioret != kIOReturnSuccess)
            {
                IOLog("DM9601V2::clear stall failed (trying to continue...)\n");
            }
        }
    }
    
    // Queue the next read, only if not aborted
    
    if (ioret != kIOReturnAborted)
    {
        ior = me->fInPipe->io(me->fPipeInMDP, NULL, &me->fReadCompletionInfo);
        if (ior != kIOReturnSuccess)
        {
            IOLog("DM9601V2::Failed to queue read!\n");
            if (ior == kIOUSBPipeStalled)
            {
                me->fInPipe->clearStall(false);
                ior = me->fInPipe->io(me->fPipeInMDP, NULL, &me->fReadCompletionInfo);
                if (ior != kIOReturnSuccess)
                {
                    IOLog("DM9601V2:: Failed, read dead!\n");
                    me->fDataDead = true;
                }
            }
        }
    }
    
    return;
    
}

void DM9601V2::dataWriteComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining)
{
    DM9601V2* me = (DM9601V2*)obj;
    mbuf_t m;
    uint32_t pktLen = 0;
    uint32_t numbufs = 0;
    uint32_t poolIndx;
    
    poolIndx = (uintptr_t)param;
    
    if (ioret == kIOReturnSuccess)                        // If operation returned ok
    {
        IOLog("DM9601V2::dataWriteComplete %04h %d\n", ioret, poolIndx);
        if (me->fPipeOutBuff[poolIndx].m != NULL)            // Null means zero length write
        {
            m = me->fPipeOutBuff[poolIndx].m;
            while (m)
            {
                pktLen += mbuf_len(m);

                numbufs++;

                m = mbuf_next(m);
            }
            
            me->freePacket(me->fPipeOutBuff[poolIndx].m);        // Free the mbuf
            me->fPipeOutBuff[poolIndx].m = NULL;
            
            if ((pktLen % me->fOutPacketSize) == 0)            // If it was a multiple of max packet size then we need to do a zero length write
            {
                IOLog("DM9601V2::writing zero length packet..\n");
                me->fPipeOutBuff[poolIndx].pipeOutMDP->setLength(0);
                me->fWriteCompletionInfo.parameter = NULL;
                me->fOutPipe->io(me->fPipeOutBuff[poolIndx].pipeOutMDP, pktLen, &me->fWriteCompletionInfo);
            }
        }
    } else {
        IOLog("DM9601V2::IO err!\n");
        
        if (me->fPipeOutBuff[poolIndx].m != NULL)
        {
            me->freePacket(me->fPipeOutBuff[poolIndx].m);        // Free the mbuf anyway
            me->fPipeOutBuff[poolIndx].m = NULL;
        }
        if (ioret != kIOReturnAborted)
        {
            ioret = me->clearPipeStall(me->fOutPipe);
            if (ioret != kIOReturnSuccess)
            {
                IOLog("DM9601V2::clear stall failed (trying to continue..)\n");
            }
        }
    }
    
    return;
    
}

void DM9601V2::merWriteComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining)
{
    DeviceRequest* MER = (DeviceRequest*)param;
    uint16_t dataLen;
    
    if (MER)
    {
        if (ioret == kIOReturnSuccess)
        {
            IOLog("DM9601V2::merWriteComplete complete!\n");
        } else {
            IOLog("DM9601V2::io err!\n");
        }
        
        dataLen = MER->wLength;
        IOLog("DM9601V2::data length = %d\n", dataLen);

        IOFree(MER, sizeof(DeviceRequest));
        
    } else {
        if (ioret == kIOReturnSuccess)
        {
            IOLog("DM9601V2::request unknown!\n");
        } else {
            IOLog("DM9601V2::request unknown- io err!\n");
        }
    }
    
    return;
    
}

void DM9601V2::statsWriteComplete(void* obj, void* param, IOReturn ioret, uint32_t remaining)
{
    DM9601V2* me = (DM9601V2*)obj;
    DeviceRequest* STREQ = (DeviceRequest*)param;
    uint16_t currStat;
    
    if (STREQ)
    {
        if (ioret == kIOReturnSuccess)
        {
            IOLog("DM9601V2::statsWriteComplete\n");
            currStat = STREQ->wValue;
            switch(currStat)
            {
                case kXMIT_OK_REQ:
                    me->fpNetStats->outputPackets = USBToHostLong(me->fStatValue);
                    break;
                case kRCV_OK_REQ:
                    me->fpNetStats->inputPackets = USBToHostLong(me->fStatValue);
                    break;
                case kXMIT_ERROR_REQ:
                    me->fpNetStats->outputErrors = USBToHostLong(me->fStatValue);
                    break;
                case kRCV_ERROR_REQ:
                    me->fpNetStats->inputErrors = USBToHostLong(me->fStatValue);
                    break;
                case kRCV_CRC_ERROR_REQ:
                    me->fpEtherStats->dot3StatsEntry.fcsErrors = USBToHostLong(me->fStatValue);
                    break;
                case kRCV_ERROR_ALIGNMENT_REQ:
                    me->fpEtherStats->dot3StatsEntry.alignmentErrors = USBToHostLong(me->fStatValue);
                    break;
                case kXMIT_ONE_COLLISION_REQ:
                    me->fpEtherStats->dot3StatsEntry.singleCollisionFrames = USBToHostLong(me->fStatValue);
                    break;
                case kXMIT_MORE_COLLISIONS_REQ:
                    me->fpEtherStats->dot3StatsEntry.multipleCollisionFrames = USBToHostLong(me->fStatValue);
                    break;
                case kXMIT_DEFERRED_REQ:
                    me->fpEtherStats->dot3StatsEntry.deferredTransmissions = USBToHostLong(me->fStatValue);
                    break;
                case kXMIT_MAX_COLLISION_REQ:
                    me->fpNetStats->collisions = USBToHostLong(me->fStatValue);
                    break;
                case kRCV_OVERRUN_REQ:
                    me->fpEtherStats->dot3StatsEntry.frameTooLongs = USBToHostLong(me->fStatValue);
                    break;
                case kXMIT_TIMES_CARRIER_LOST_REQ:
                    me->fpEtherStats->dot3StatsEntry.carrierSenseErrors = USBToHostLong(me->fStatValue);
                    break;
                case kXMIT_LATE_COLLISIONS_REQ:
                    me->fpEtherStats->dot3StatsEntry.lateCollisions = USBToHostLong(me->fStatValue);
                    break;
                default:
                    IOLog("DM9601V2::Invalid stats code %d\n", currStat);
                    break;
            }
            
        } else {
            IOLog("DM9601V2::io err!\n");
        }
        
        IOFree(STREQ, sizeof(IOUSBDevRequest));
    } else {
        if (ioret == kIOReturnSuccess)
        {
            IOLog("DM9601V2::request unknown!\n");
        } else {
            IOLog("DM9601V2::request unknown - io err!\n");
        }
    }
    
    me->fStatValue = 0;
    me->fStatInProgress = false;
    return;
    
}

//*********************************
#pragma mark -
#pragma mark Helper Function
#pragma mark -
//*********************************

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

IOUSBHostPipe* DM9601V2::findpipe(IOUSBHostInterface* intf, IOUSBHostPipe* current, EndpointRequest* request)
{
    const EndpointDescriptor* endpointCandidate = NULL;
    IOUSBHostPipe* foundPipe = NULL;
    IOUSBHostDevice* dev = intf->getDevice();
    uint8_t epDirection;
    uint8_t epType;
    uint16_t maxsize;
    uint8_t interval;
    
    while((endpointCandidate = StandardUSB::getNextEndpointDescriptor(intf->getConfigurationDescriptor(), intf->getInterfaceDescriptor(), endpointCandidate)) != NULL)
    {
        epDirection = StandardUSB::getEndpointDirection(endpointCandidate);
        epType = StandardUSB::getEndpointType(endpointCandidate);
        maxsize = getEndpointMaxPacketSize(dev->getSpeed(), endpointCandidate);
        interval = endpointCandidate->bInterval;
        
        if(epDirection == request->direction && epType == request->type && maxsize == request->maxPacketSize && interval == request->interval)
        {
            foundPipe = intf->copyPipe(StandardUSB::getEndpointAddress(endpointCandidate));
            break;
        }
    }
    return foundPipe;
}
//*********************************
#pragma mark -
#pragma mark IOEthernetController Override Methods
#pragma mark -
//*********************************
IOReturn DM9601V2::enable(IONetworkInterface* netif)
{
    IONetworkMedium* medium;
    IOMediumType mediumType = kIOMediumEthernet100BaseTX | kIOMediumOptionFullDuplex;
    
    IOLog("%s::enable start!\n", getName());
    
    // If an interface client has previously enabled us,
    // and we know there can only be one interface client
    // for this driver, then simply return true.
    
    if (fNetifEnabled)
    {
        IOLog("%s::already enabled!\n", getName());
        return kIOReturnSuccess;
    }
    
    if ((fReady == false) && !wakeUp())
    {
        IOLog("%s::Failed to enable!\n", getName());
        return kIOReturnIOError;
    }
    
    // Mark the controller as enabled by the interface.
    
    fNetifEnabled = true;
    
    // Assume an active link for now
    
    fLinkStatus = 1;
    medium = IONetworkMedium::getMediumWithType(fMediumDict, mediumType);
    IOLog("%s::medium type and pointer!\n", getName());
    setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid, medium, 10 * 1000000);
    IOLog("%s::LinkStatus has been set!\n", getName());
    
    // Start our IOOutputQueue object.
    
    fTransmitQueue->setCapacity(TRANSMIT_QUEUE_SIZE);
    IOLog("%s::capicity set!\n", getName());
    fTransmitQueue->start();
    IOLog("%s::transmit queue started!\n", getName());
    
    USBSetPacketFilter();
    IOLog("%s::packet filter applied!\n", getName());
    
    return kIOReturnSuccess;
    
}

IOOutputQueue* DM9601V2::createOutputQueue()
{    
    return IOBasicOutputQueue::withTarget(this, TRANSMIT_QUEUE_SIZE);
    
}

IOReturn DM9601V2::getHardwareAddress(IOEthernetAddress *ea)
{
    return kIOReturnSuccess;
}
