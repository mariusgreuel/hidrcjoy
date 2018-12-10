//
// Receiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include "Configuration.h"
#include "UsbReports.h"
#include "PpmReceiver.h"
#if HIDRCJOY_SRXL
#include "SrxlReceiver.h"
#endif
/////////////////////////////////////////////////////////////////////////////

class Receiver
{
public:
    void Initialize()
    {
        m_PpmReceiver.Initialize();
#if HIDRCJOY_SRXL
        m_SrxlReceiver.Initialize();
#endif
    }

    void LoadDefaultConfiguration()
    {
        m_Configuration.m_version = Configuration::version;
        m_Configuration.m_flags = 0;
        m_Configuration.m_minSyncPulseWidth = 3500;
        m_Configuration.m_centerChannelPulseWidth = 1500;
        m_Configuration.m_channelPulseWidthRange = 550;
        m_Configuration.m_polarity = 0;

        for (uint8_t i = 0; i < sizeof(m_Configuration.m_mapping); i++)
        {
            m_Configuration.m_mapping[i] = i;
        }
    }

    void UpdateConfiguration()
    {
        m_PpmReceiver.SetConfiguration(m_Configuration.m_minSyncPulseWidth, (m_Configuration.m_flags & Configuration::Flags::InvertedSignal) != 0);
    }

    bool IsValidConfiguration() const
    {
        if (m_Configuration.m_version != Configuration::version)
            return false;

        if (m_Configuration.m_minSyncPulseWidth < Configuration::minSyncWidth ||
            m_Configuration.m_minSyncPulseWidth > Configuration::maxSyncWidth)
            return false;

        if (m_Configuration.m_centerChannelPulseWidth < Configuration::minChannelPulseWidth ||
            m_Configuration.m_centerChannelPulseWidth > Configuration::maxChannelPulseWidth)
            return false;

        if (m_Configuration.m_channelPulseWidthRange < 10 ||
            m_Configuration.m_channelPulseWidthRange > Configuration::maxChannelPulseWidth)
            return false;

        for (uint8_t i = 0; i < sizeof(m_Configuration.m_mapping); i++)
        {
            if (m_Configuration.m_mapping[i] >= Configuration::maxChannels)
            {
                return false;
            }
        }

        return true;
    }

    void Update(uint32_t time)
    {
        m_PpmReceiver.Update(time);
#if HIDRCJOY_SRXL
        m_SrxlReceiver.Update(time);
#endif
    }

    uint16_t GetChannelPulseWidth(uint8_t channel) const
    {
        uint8_t index = m_Configuration.m_mapping[channel];
        
        if (m_PpmReceiver.IsDataAvailable())
        {
            return m_PpmReceiver.GetChannelPulseWidth(index);
        }
#if HIDRCJOY_SRXL
        else if (m_SrxlReceiver.IsDataAvailable())
        {
            return m_SrxlReceiver.GetChannelPulseWidth(index);
        }
#endif
        else
        {
            return 0;
        }
    }

    uint8_t GetStatus() const
    {
        if (m_PpmReceiver.IsDataAvailable())
        {
            return PpmSignal;
        }
#if HIDRCJOY_SRXL
        else if (m_SrxlReceiver.IsDataAvailable())
        {
            return SrxlSignal;
        }
#endif
        else
        {
            return NoSignal;
        }
    }

    uint8_t GetValue(uint8_t channel) const
    {
        int16_t center = m_Configuration.m_centerChannelPulseWidth;
        int16_t range = m_Configuration.m_channelPulseWidthRange;
        int16_t value = Polarity(channel, (int16_t)GetChannelPulseWidth(channel) - center);
        int32_t scaled = 128 + 128 * (int32_t)value / range;
        return Saturate(scaled);
    }

private:
    int16_t Polarity(uint8_t channel, int16_t value) const
    {
        return (m_Configuration.m_polarity & (1 << channel)) == 0 ? value : -value;
    }

    uint8_t Saturate(int32_t value) const
    {
        if (value < 0)
        {
            return 0;
        }
        else if (value > 255)
        {
            return 255;
        }
        else
        {
            return value;
        }
    }

public:
    Configuration m_Configuration;
    PpmReceiver m_PpmReceiver;
#if HIDRCJOY_SRXL
    SrxlReceiver m_SrxlReceiver;
#endif
};
