/*
 * Copyright (c) 2010 Eduard Burtescu
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <processor/Processor.h>
#include <utilities/assert.h>
#include <utilities/PointerGuard.h>
#include <usb/Usb.h>
#include <usb/UsbDevice.h>
#include <usb/UsbHub.h>
#include <usb/UsbConstants.h>
#include <usb/UsbDescriptors.h>

UsbDevice::UsbDevice(uint8_t nPort, UsbSpeed speed) :
    m_nAddress(0), m_nPort(nPort), m_Speed(speed), m_UsbState(Connected)
{
}

UsbDevice::UsbDevice(UsbDevice *pDev) :
    m_nAddress(pDev->m_nAddress), m_nPort(pDev->m_nPort), m_Speed(pDev->m_Speed), m_UsbState(Connected),
    m_pDescriptor(pDev->m_pDescriptor), m_pConfiguration(pDev->m_pConfiguration), m_pInterface(pDev->m_pInterface)
{
}

UsbDevice::~UsbDevice()
{
}

UsbDevice::DeviceDescriptor::DeviceDescriptor(UsbDeviceDescriptor *pDescriptor)
{
    nBcdUsbRelease = pDescriptor->nBcdUsbRelease;
    nClass = pDescriptor->nClass;
    nSubclass = pDescriptor->nSubclass;
    nProtocol = pDescriptor->nProtocol;
    nMaxControlPacketSize = pDescriptor->nMaxControlPacketSize;
    nVendorId = pDescriptor->nVendorId;
    nProductId = pDescriptor->nProductId;
    nBcdDeviceRelease = pDescriptor->nBcdDeviceRelease;
    nVendorString = pDescriptor->nVendorString;
    nProductString = pDescriptor->nProductString;
    nSerialString = pDescriptor->nSerialString;
    nConfigurations = pDescriptor->nConfigurations;

    delete pDescriptor;
}

UsbDevice::DeviceDescriptor::~DeviceDescriptor()
{
    for(size_t i = 0; i < configList.count(); i++)
        delete configList[i];
}

UsbDevice::ConfigDescriptor::ConfigDescriptor(void *pConfigBuffer, size_t nConfigLength)
{
    UsbConfigurationDescriptor *pDescriptor = static_cast<UsbConfigurationDescriptor*>(pConfigBuffer);
    nConfig = pDescriptor->nConfig;
    nString = pDescriptor->nString;

    uint8_t *pBuffer = static_cast<uint8_t*>(pConfigBuffer);
    size_t nOffset = pBuffer[0];
    Interface *pCurrentInterface = 0;

    while(nOffset < nConfigLength)
    {
        size_t nLength = pBuffer[nOffset];
        uint8_t nType = pBuffer[nOffset + 1];
        if(nType == UsbDescriptor::Interface)
        {
            Interface *pNewInterface = new Interface(reinterpret_cast<UsbInterfaceDescriptor*>(&pBuffer[nOffset]));
            if(!pNewInterface->nAlternateSetting)
            {
                assert(interfaceList.count() < pDescriptor->nInterfaces);
                pCurrentInterface = pNewInterface;
                interfaceList.pushBack(pCurrentInterface);
            }
        }
        else if(pCurrentInterface)
        {
            if(nType == UsbDescriptor::Endpoint)
                pCurrentInterface->endpointList.pushBack(new Endpoint(reinterpret_cast<UsbEndpointDescriptor*>(&pBuffer[nOffset])));
            else
                pCurrentInterface->otherDescriptorList.pushBack(new UnknownDescriptor(&pBuffer[nOffset], nType, nLength));
        }
        else
            otherDescriptorList.pushBack(new UnknownDescriptor(&pBuffer[nOffset], nType, nLength));
        nOffset += nLength;
    }
    assert(interfaceList.count());

    delete pDescriptor;
}

UsbDevice::ConfigDescriptor::~ConfigDescriptor()
{
    for(size_t i = 0; i < interfaceList.count(); i++)
        delete interfaceList[i];
    for(size_t i = 0; i < otherDescriptorList.count(); i++)
        delete otherDescriptorList[i];
}

UsbDevice::Interface::Interface(UsbInterfaceDescriptor *pDescriptor)
{
    nInterface = pDescriptor->nInterface;
    nAlternateSetting = pDescriptor->nAlternateSetting;
    nClass = pDescriptor->nClass;
    nSubclass = pDescriptor->nSubclass;
    nProtocol = pDescriptor->nProtocol;
    nString = pDescriptor->nString;
}

UsbDevice::Interface::~Interface()
{
    for(size_t i = 0; i < endpointList.count(); i++)
        delete endpointList[i];
    for(size_t i = 0; i < otherDescriptorList.count(); i++)
        delete otherDescriptorList[i];
}

UsbDevice::Endpoint::Endpoint(UsbEndpointDescriptor *pDescriptor) : bDataToggle(false)
{
    nEndpoint = pDescriptor->nEndpoint;
    bIn = pDescriptor->bDirection;
    bOut = !bIn;
    nTransferType = pDescriptor->nTransferType;
    nMaxPacketSize = pDescriptor->nMaxPacketSize;
}

void UsbDevice::initialise(uint8_t nAddress)
{
    // Check for late calls
    if(m_UsbState > Connected)
    {
        ERROR("USB: UsbDevice::initialise called, but this device is already initialised!");
        return;
    }

    // Assign the given address to this device
    if(!controlRequest(0, UsbRequest::SetAddress, nAddress, 0))
        return;
    m_nAddress = nAddress;
    m_UsbState = Addressed; // We now have an address

    // Get the device descriptor
    void *pDeviceDescriptor = getDescriptor(UsbDescriptor::Device, 0, getDescriptorLength(UsbDescriptor::Device, 0));
    if(!pDeviceDescriptor)
        return;
    m_pDescriptor = new DeviceDescriptor(static_cast<UsbDeviceDescriptor*>(pDeviceDescriptor));
    m_UsbState = HasDescriptors; // We now have the device descriptor

    // Debug dump of the device descriptor
#ifdef USB_VERBOSE_DEBUG
    DEBUG_LOG("USB version: " << Dec << (m_pDescriptor->nBcdUsbRelease >> 8) << "." << (m_pDescriptor->nBcdUsbRelease & 0xFF) << ".");
    DEBUG_LOG("Device class/subclass/protocol: " << m_pDescriptor->nClass << "/" << m_pDescriptor->nSubclass << "/" << m_pDescriptor->nProtocol);
    DEBUG_LOG("Maximum control packet size is " << Dec << m_pDescriptor->nMaxControlPacketSize << Hex << " bytes.");
    DEBUG_LOG("Vendor and product IDs: " << m_pDescriptor->nVendorId << ":" << m_pDescriptor->nProductId << ".");
    DEBUG_LOG("Device version: " << Dec << (m_pDescriptor->nBcdDeviceRelease >> 8) << "." << (m_pDescriptor->nBcdDeviceRelease & 0xFF) << ".");
#endif

    // Descriptor number for the configuration descriptor
    uint8_t nConfigDescriptor = UsbDescriptor::Configuration;

    // Handle high-speed capable devices running at full-speed:
    // If the device works at full-speed and it has a device qualifier descriptor,
    // it means that the normal configuration descriptor is for high-speed,
    // and we need to use the other speed configuration descriptor
    if(m_Speed == FullSpeed && m_pDescriptor->nBcdUsbRelease > ((1 << 8) + 19) && getDescriptorLength(UsbDescriptor::DeviceQualifier, 0))
        nConfigDescriptor = UsbDescriptor::OtherSpeedConfiguration;

    // Get the vendor, product and serial strings
    m_pDescriptor->sVendor = getString(m_pDescriptor->nVendorString);
    m_pDescriptor->sProduct = getString(m_pDescriptor->nProductString);
    m_pDescriptor->sSerial = getString(m_pDescriptor->nSerialString);

    // Grab each configuration from the device
    for(size_t i = 0; i < m_pDescriptor->nConfigurations; i++)
    {
        // Skip extra configurations
        if(i)
        {
            WARNING("USB: Found a device with multiple configurations!");
            break;
        }

        // Get the total size of this configuration descriptor
        uint16_t *pPartialConfig = static_cast<uint16_t*>(getDescriptor(nConfigDescriptor, i, 4));
        if(!pPartialConfig)
            return;
        uint16_t configLength = pPartialConfig[1];
        delete pPartialConfig;

        // Get our configuration descriptor
        ConfigDescriptor *pConfig = new ConfigDescriptor(getDescriptor(nConfigDescriptor, i, configLength), configLength);

        // Get the associated string
        pConfig->sString = getString(pConfig->nString);

        // Go through the interface list
        for(size_t j = 0; j < pConfig->interfaceList.count(); j++)
        {
             // Get this interface, for minor adjustments
            Interface *pInterface = pConfig->interfaceList[j];

            // Just in case the class numbers are in the device descriptor
            if(pConfig->interfaceList.count() == 1 && m_pDescriptor->nClass && !pInterface->nClass)
            {
                pInterface->nClass = m_pDescriptor->nClass;
                pInterface->nSubclass = m_pDescriptor->nSubclass;
                pInterface->nProtocol = m_pDescriptor->nProtocol;
            }

            // Again, get the associated string
            pInterface->sString = getString(pInterface->nString);
        }

        // Make sure it's not empty, then add it to our list of configurations
        assert(pConfig->interfaceList.count());
        m_pDescriptor->configList.pushBack(pConfig);
    }

    // Make sure we ended up with at least a configuration
    assert(m_pDescriptor->configList.count());

    // Use the first configuration
    /// \todo support more configurations (how?)
    useConfiguration(0);
}

ssize_t UsbDevice::doSync(UsbDevice::Endpoint *pEndpoint, UsbPid pid, uintptr_t pBuffer, size_t nBytes, size_t timeout)
{
    if(!pEndpoint)
    {
        ERROR("USB: UsbDevice::doSync called with invalid endpoint");
        return -TransactionError;
    }

    UsbHub *pParentHub = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParentHub)
    {
        ERROR("USB: Orphaned UsbDevice!");
        return -TransactionError;
    }

    if(!nBytes)
        return 0;

    if(pBuffer & 0xF)
    {
        ERROR("USB: Input pointer wasn't properly aligned [" << pBuffer << ", " << nBytes << "]");
        return -TransactionError;
    }

    UsbEndpoint endpointInfo(m_nAddress, m_nPort, pEndpoint->nEndpoint, m_Speed, pEndpoint->nMaxPacketSize);
    uintptr_t nTransaction = pParentHub->createTransaction(endpointInfo);
    if(nTransaction == static_cast<uintptr_t>(-1))
    {
        ERROR("UsbDevice: couldn't get a valid transaction to work with from the parent hub");
        return -TransactionError;
    }

    size_t byteOffset = 0;
    while(nBytes)
    {
        size_t nBytesThisTransaction = nBytes > pEndpoint->nMaxPacketSize ? pEndpoint->nMaxPacketSize : nBytes;

        pParentHub->addTransferToTransaction(nTransaction, pEndpoint->bDataToggle, pid, pBuffer + byteOffset, nBytesThisTransaction);
        byteOffset += nBytesThisTransaction;
        nBytes -= nBytesThisTransaction;

        pEndpoint->bDataToggle = !pEndpoint->bDataToggle;
    }

    return pParentHub->doSync(nTransaction, timeout);
}

ssize_t UsbDevice::syncIn(Endpoint *pEndpoint, uintptr_t pBuffer, size_t nBytes, size_t timeout)
{
    return doSync(pEndpoint, UsbPidIn, pBuffer, nBytes, timeout);
}

ssize_t UsbDevice::syncOut(Endpoint *pEndpoint, uintptr_t pBuffer, size_t nBytes, size_t timeout)
{
    return doSync(pEndpoint, UsbPidOut, pBuffer, nBytes, timeout);
}

void UsbDevice::addInterruptInHandler(Endpoint *pEndpoint, uintptr_t pBuffer, uint16_t nBytes, void (*pCallback)(uintptr_t, ssize_t), uintptr_t pParam)
{
    if(!pEndpoint)
    {
        ERROR("USB: UsbDevice::addInterruptInHandler called with invalid endpoint");
        return;
    }

    UsbHub *pParentHub = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParentHub)
    {
        ERROR("USB: Orphaned UsbDevice!");
        return;
    }

    if(!nBytes)
        return;

    if(pBuffer & 0xF)
    {
        ERROR("USB: Input pointer wasn't properly aligned [" << pBuffer << ", " << nBytes << "]");
        return;
    }

    UsbEndpoint endpointInfo(m_nAddress, m_nPort, pEndpoint->nEndpoint, m_Speed, pEndpoint->nMaxPacketSize);
    pParentHub->addInterruptInHandler(endpointInfo, pBuffer, nBytes, pCallback, pParam);
}

bool UsbDevice::controlRequest(uint8_t nRequestType, uint8_t nRequest, uint16_t nValue, uint16_t nIndex, uint16_t nLength, uintptr_t pBuffer)
{
    // Setup structure - holds request details
    Setup *pSetup = new Setup(nRequestType, nRequest, nValue, nIndex, nLength);
    PointerGuard<Setup> guard(pSetup);

    UsbHub *pParentHub = dynamic_cast<UsbHub*>(m_pParent);
    if(!pParentHub)
    {
        ERROR("USB: Orphaned UsbDevice!");
        return -1;
    }

    UsbEndpoint endpointInfo(m_nAddress, m_nPort, 0, m_Speed, 64);

    uintptr_t nTransaction = pParentHub->createTransaction(endpointInfo);
    if(nTransaction == static_cast<uintptr_t>(-1))
    {
        ERROR("UsbDevice: couldn't get a valid transaction to work with from the parent hub");
        return -1;
    }

    // Setup Transfer - handles the SETUP phase of the transfer
    pParentHub->addTransferToTransaction(nTransaction, false, UsbPidSetup, reinterpret_cast<uintptr_t>(pSetup), sizeof(Setup));

    // Data Transfer - handles data transfer
    if(nLength)
        pParentHub->addTransferToTransaction(nTransaction, true, nRequestType & UsbRequestDirection::In ? UsbPidIn : UsbPidOut, pBuffer, nLength);

    // Handshake Transfer - IN when we send data to the device, OUT when we receive. Zero-length.
    pParentHub->addTransferToTransaction(nTransaction, true, nRequestType & UsbRequestDirection::In ? UsbPidOut : UsbPidIn, 0, 0);

    // Wait for the transaction to complete
    ssize_t nResult = pParentHub->doSync(nTransaction);
    // Return false if we had an error, true otherwise
    return nResult >= 0; // >= sizeof(Setup) + nLength;
}

uint16_t UsbDevice::getStatus()
{
    uint16_t *nStatus = new uint16_t(0);
    PointerGuard<uint16_t> guard(nStatus);
    controlRequest(UsbRequestDirection::In, UsbRequest::GetStatus, 0, 0, 2, reinterpret_cast<uintptr_t>(nStatus));
    return *nStatus;
}

bool UsbDevice::clearEndpointHalt(Endpoint *pEndpoint)
{
    return controlRequest(UsbRequestRecipient::Endpoint, UsbRequest::ClearFeature, 0, pEndpoint->nEndpoint);
}

void UsbDevice::useConfiguration(uint8_t nConfig)
{
    m_pConfiguration = m_pDescriptor->configList[nConfig];
    if(!controlRequest(0, UsbRequest::SetConfiguration, m_pConfiguration->nConfig, 0))
        return;
    m_UsbState = Configured; // We now are configured
}

void UsbDevice::useInterface(uint8_t nInterface)
{
    m_pInterface = m_pConfiguration->interfaceList[nInterface];
    if(m_pInterface->nAlternateSetting && !controlRequest(UsbRequestRecipient::Interface, UsbRequest::SetInterface, m_pInterface->nAlternateSetting, 0))
       return;
    m_UsbState = HasInterface; // We now have an interface
}

void *UsbDevice::getDescriptor(uint8_t nDescriptor, uint8_t nSubDescriptor, uint16_t nBytes, uint8_t requestType)
{
    uint8_t *pBuffer = new uint8_t[nBytes];
    uint16_t nIndex = requestType & UsbRequestRecipient::Interface ? m_pInterface->nInterface : 0;

    /// \todo Proper language ID handling!
    if(nDescriptor == UsbDescriptor::String)
        nIndex = 0x0409; // English (US)

    if(!controlRequest(UsbRequestDirection::In | requestType, UsbRequest::GetDescriptor, (nDescriptor << 8) | nSubDescriptor, nIndex, nBytes, reinterpret_cast<uintptr_t>(pBuffer)))
    {
        delete [] pBuffer;
        return 0;
    }
    return pBuffer;
}

uint8_t UsbDevice::getDescriptorLength(uint8_t nDescriptor, uint8_t nSubDescriptor, uint8_t requestType)
{
    uint8_t *length = new uint8_t(0);
    PointerGuard<uint8_t> guard(length);
    uint16_t nIndex = requestType & UsbRequestRecipient::Interface ? m_pInterface->nInterface : 0;

    /// \todo Proper language ID handling
    if(nDescriptor == UsbDescriptor::String)
        nIndex = 0x0409; // English (US)

    controlRequest(UsbRequestDirection::In | requestType, UsbRequest::GetDescriptor, (nDescriptor << 8) | nSubDescriptor, nIndex, 1, reinterpret_cast<uintptr_t>(length));
    return *length;
}

String UsbDevice::getString(uint8_t nString)
{
    // A value of zero means there's no string
    if(!nString)
        return String("");

    uint8_t descriptorLength = getDescriptorLength(UsbDescriptor::String, nString);
    if(!descriptorLength)
        return String("");

    char *pBuffer = static_cast<char*>(getDescriptor(UsbDescriptor::String, nString, descriptorLength));
    if(!pBuffer)
        return String("");

    // Get the number of characters in the string and allocate a new buffer for the string
    size_t nStrLength = (descriptorLength - 2) / 2;
    char *pString = new char[nStrLength + 1];

    // For each character, get the lower part of the UTF-16 value
    /// \todo UTF-8 support of some kind
    for(size_t i = 0; i < nStrLength; i++)
        pString[i] = pBuffer[2 + i * 2];

    // Set the last byte of the string to 0, delete the old buffer and return the string
    pString[nStrLength] = 0;
    delete pBuffer;
    return String(pString);
}
