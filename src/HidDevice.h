//
// HidDevice.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include "Buffer.h"
#include "../firmware/src/configuration.h"
#include "../firmware/src/usb_reports.h"

class HidDevice
{
public:
    HidDevice() = default;
    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;
    HidDevice(HidDevice&&) = default;
    HidDevice& operator=(HidDevice&&) = default;

    ~HidDevice()
    {
        Close();
    }

    HRESULT Open(LPCTSTR pszDevicePath)
    {
        DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        HANDLE hHidDevice = CreateFile(pszDevicePath, dwDesiredAccess, dwShareMode, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hHidDevice == INVALID_HANDLE_VALUE)
            return AtlHresultFromLastError();

        m_hDevice = hHidDevice;

        HRESULT hr = GetProperties();
        if (FAILED(hr))
            return hr;

        hr = CheckDeviceMatch();
        if (FAILED(hr))
            return hr;

        HidD_SetNumInputBuffers(m_hDevice, 2);

        return S_OK;
    }

    void Close()
    {
        if (m_preparsedData != nullptr)
        {
            HidD_FreePreparsedData(m_preparsedData);
            m_preparsedData = nullptr;
        }

        if (m_hDevice != nullptr)
        {
            CloseHandle(m_hDevice);
            m_hDevice = nullptr;
        }
    }

    std::wstring GetProduct() const { return m_product; }
    std::wstring GetManufacturer() const { return m_manufacturer; }

public:
    Configuration* GetConfiguration() { return &m_configuration; }

    void ReadReport(UsbReport& report)
    {
        auto buffer = Read();
        std::memcpy(&report, buffer.GetData(), sizeof(report));
    }

    void ReadEnhancedReport(UsbEnhancedReport& report)
    {
        auto buffer = GetFeatureReport(UsbEnhancedReportId);
        std::memcpy(&report, buffer.GetData(), sizeof(report));
    }

    void ReadConfiguration()
    {
        auto buffer = GetFeatureReport(ConfigurationReportId);
        std::memcpy(&m_configuration, buffer.GetData(), sizeof(m_configuration));
    }

    void WriteConfiguration()
    {
        Buffer<uint8_t> buffer(reinterpret_cast<const uint8_t*>(&m_configuration), sizeof(m_configuration));
        SetFeatureReport(ConfigurationReportId, buffer);
    }

    void LoadDefaultConfiguration()
    {
        SetFeatureReport(LoadConfigurationDefaultsId, Buffer<uint8_t>());
    }

    void ReadConfigurationFromEeprom()
    {
        SetFeatureReport(ReadConfigurationFromEepromId, Buffer<uint8_t>());
    }

    void WriteConfigurationToEeprom()
    {
        SetFeatureReport(WriteConfigurationToEepromId, Buffer<uint8_t>());
    }

    void JumpToBootloader()
    {
        SetFeatureReport(JumpToBootloaderId, Buffer<uint8_t>());
    }

    Buffer<uint8_t> Read()
    {
        Buffer<uint8_t> buffer(m_caps.InputReportByteLength);

        DWORD dwBytesRead = 0;
        if (!ReadFile(m_hDevice, buffer.GetData(), static_cast<DWORD>(buffer.GetSize()), &dwBytesRead, nullptr))
            throw std::runtime_error("ReadFile failed");

        return buffer;
    }

    Buffer<uint8_t> GetFeatureReport(uint8_t index)
    {
        Buffer<uint8_t> buffer(m_caps.FeatureReportByteLength);

        buffer[0] = index;
        if (!HidD_GetFeature(m_hDevice, buffer.GetData(), m_caps.FeatureReportByteLength))
            throw std::runtime_error("HidD_GetFeature failed");

        return buffer;
    }

    void SetFeatureReport(uint8_t index, const Buffer<uint8_t>& buffer)
    {
        if (buffer.GetSize() > m_caps.FeatureReportByteLength)
            throw std::runtime_error("Buffer too big");

        Buffer<uint8_t> report(m_caps.FeatureReportByteLength);

        std::memcpy(report.GetData(), buffer.GetData(), buffer.GetSize());
        std::memset(report.GetData() + buffer.GetSize(), 0, m_caps.FeatureReportByteLength - buffer.GetSize());
        report[0] = index;

        if (!HidD_SetFeature(m_hDevice, report.GetData(), m_caps.FeatureReportByteLength))
            throw std::runtime_error("HidD_SetFeature failed");
    }

private:
    HRESULT GetProperties()
    {
        if (!HidD_GetPreparsedData(m_hDevice, &m_preparsedData))
            return AtlHresultFromLastError();

        if (!HidP_GetCaps(m_preparsedData, &m_caps))
            return AtlHresultFromLastError();

        WCHAR product[128] = {};
        if (!HidD_GetProductString(m_hDevice, product, sizeof(product)))
            return AtlHresultFromLastError();

        m_product = product;

        WCHAR manufacturer[128] = {};
        if (!HidD_GetManufacturerString(m_hDevice, manufacturer, sizeof(manufacturer)))
            return AtlHresultFromLastError();

        m_manufacturer = manufacturer;

        WCHAR serialNumber[128] = {};
        if (HidD_GetSerialNumberString(m_hDevice, serialNumber, sizeof(serialNumber)))
        {
            m_serialNumber = serialNumber;
        }

        return S_OK;
    }

    HRESULT CheckDeviceMatch()
    {
        HIDD_ATTRIBUTES attributes = {};
        if (!HidD_GetAttributes(m_hDevice, &attributes))
            return AtlHresultFromLastError();

        if (attributes.VendorID == 0x2341 && attributes.ProductID == 0x8036)
        {
            // Arduino Leonardo
        }
        else if (attributes.VendorID == 0x2341 && attributes.ProductID == 0x8037)
        {
            // Arduino Micro
        }
        else if (attributes.VendorID == 0x1B4F && attributes.ProductID == 0x9206)
        {
            // SparkFun Pro Micro
        }
        else
        {
            return AtlHresultFromWin32(ERROR_INVALID_DATA);
        }

        if (m_caps.UsagePage != HID_USAGE_PAGE_GENERIC || m_caps.Usage != HID_USAGE_GENERIC_JOYSTICK)
            return AtlHresultFromWin32(ERROR_INVALID_DATA);

        return S_OK;
    }

private:
    HANDLE m_hDevice = nullptr;
    PHIDP_PREPARSED_DATA m_preparsedData = nullptr;
    HIDP_CAPS m_caps = {};
    std::wstring m_product;
    std::wstring m_manufacturer;
    std::wstring m_serialNumber;

    Configuration m_configuration{};
};
    
//---------------------------------------------------------------------------

class HidDeviceCollection
{
public:
    HRESULT EnumerateDevices()
    {
        m_devices.clear();

        GUID hidGuid = {};
        HidD_GetHidGuid(&hidGuid);

        HDEVINFO hDeviceInfoSet = SetupDiGetClassDevs(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (hDeviceInfoSet == nullptr)
            return AtlHresultFromLastError();

        HRESULT hr = S_OK;

        DWORD index = 0;
        while (true)
        {
            SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
            if (!SetupDiEnumDeviceInterfaces(hDeviceInfoSet, nullptr, &hidGuid, index, &deviceInterfaceData))
            {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_NO_MORE_ITEMS)
                {
                    hr = AtlHresultFromWin32(dwError);
                }

                break;
            }

            DWORD dwRequiredSize = 0;
            if (!SetupDiGetDeviceInterfaceDetail(hDeviceInfoSet, &deviceInterfaceData, nullptr, 0, &dwRequiredSize, nullptr))
            {
                DWORD dwError = GetLastError();
                if (dwError != ERROR_INSUFFICIENT_BUFFER)
                {
                    hr = AtlHresultFromWin32(dwError);
                    break;
                }
            }

            std::unique_ptr<uint8_t[]> buffer(new uint8_t[dwRequiredSize]);

            SP_DEVICE_INTERFACE_DETAIL_DATA* pDeviceInterfaceDetailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(buffer.get());
            std::memset(pDeviceInterfaceDetailData, 0, dwRequiredSize);
            pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            if (SetupDiGetDeviceInterfaceDetail(hDeviceInfoSet, &deviceInterfaceData, pDeviceInterfaceDetailData, dwRequiredSize, &dwRequiredSize, nullptr))
            {
                auto device = std::make_shared<HidDevice>();
                hr = device->Open(pDeviceInterfaceDetailData->DevicePath);
                if (SUCCEEDED(hr))
                {
                    m_devices.push_back(device);
                }
            }

            index++;
        }

        SetupDiDestroyDeviceInfoList(hDeviceInfoSet);

        return S_OK;
    }

    size_t GetDeviceCount() const { return m_devices.size(); }
    std::shared_ptr<HidDevice> GetDevice(size_t item) const { return m_devices[item]; }

private:
    std::vector<std::shared_ptr<HidDevice>> m_devices;
};
