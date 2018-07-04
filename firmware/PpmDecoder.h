//
// PpmDecoder.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include "PpmConfiguration.h"

/////////////////////////////////////////////////////////////////////////////

class PpmDecoder
{
    static const int32_t scaleFactor = 0x2000;
    static const uint32_t signalTimeout = 100000;

public:
    void LoadDefaultConfiguration()
    {
        m_Configuration.m_version = PpmConfiguration::version;
        m_Configuration.m_flags = PpmConfiguration::Flags::Default;
        m_Configuration.m_minSyncPulseWidth = 3500;
        m_Configuration.m_centerChannelPulseWidth = 1500;
        m_Configuration.m_channelPulseWidthRange = 500;
        m_Configuration.m_polarity = 0;

        for (uint8_t i = 0; i < sizeof(m_Configuration.m_mapping); i++)
        {
            m_Configuration.m_mapping[i] = i;
        }
    }

    void ApplyConfiguration()
    {
        m_minSyncPulseWidth = UsToTicks(m_Configuration.m_minSyncPulseWidth);
        m_centerChannelPulseWidth = UsToTicks(m_Configuration.m_centerChannelPulseWidth);
        m_channelPulseWidthRange = UsToTicks(m_Configuration.m_channelPulseWidthRange);
    }

    bool IsValidConfiguration() const
    {
        if (m_Configuration.m_version != PpmConfiguration::version)
            return false;

        if (m_Configuration.m_minSyncPulseWidth < PpmConfiguration::minSyncWidth ||
            m_Configuration.m_minSyncPulseWidth > PpmConfiguration::maxSyncWidth)
            return false;

        if (m_Configuration.m_centerChannelPulseWidth < PpmConfiguration::minChannelPulseWidth ||
            m_Configuration.m_centerChannelPulseWidth > PpmConfiguration::maxChannelPulseWidth)
            return false;

        if (m_Configuration.m_channelPulseWidthRange < 10 ||
            m_Configuration.m_channelPulseWidthRange > PpmConfiguration::maxChannelPulseWidth)
            return false;

        for (uint8_t i = 0; i < sizeof(m_Configuration.m_mapping); i++)
        {
            if (m_Configuration.m_mapping[i] >= PpmConfiguration::maxChannels)
            {
                return false;
            }
        }

        return true;
    }

    void Update(uint32_t time)
    {
        uint8_t updateCounter = m_updateCounter;
        if (updateCounter == m_lastUpdateCount)
        {
            if (time - m_lastUpdateTime > signalTimeout)
            {
                m_isDataAvailable = false;
            }
        }
        else
        {
            m_lastUpdateCount = updateCounter;
            m_lastUpdateTime = time;
            m_isDataAvailable = true;
        }
    }

    void OnPinChanged(bool level, uint16_t time)
    {
        bool usePositiveEdge = (m_Configuration.m_flags & PpmConfiguration::Flags::Inverted) == 0;
        if (level != usePositiveEdge)
            return;

        uint16_t diff = time - m_lastTime;
        m_lastTime = time;

        if (diff >= m_minSyncPulseWidth)
        {
            m_syncPulseWidth = diff;
            m_currentChannel = 0;
            m_updateCounter++;
        }
        else if (m_currentChannel < PpmConfiguration::maxChannels)
        {
            m_channelPulseWidth[m_currentChannel] = diff;
            m_currentChannel++;
        }
    }

    bool IsDataAvailable() const
    {
        return m_isDataAvailable;
    }

    uint16_t GetSyncPulseWidth() const
    {
        cli();
        uint16_t value = m_syncPulseWidth;
        sei();
        return value;
    }

    uint16_t GetChannelPulseWidth(uint8_t channel) const
    {
        uint8_t index = m_Configuration.m_mapping[channel];
        cli();
        uint16_t value = m_channelPulseWidth[index];
        sei();
        return value;
    }

    int8_t GetValue(uint8_t channel) const
    {
        int16_t center = m_centerChannelPulseWidth;
        int16_t range = m_channelPulseWidthRange;
        int16_t value = Polarity(channel, (int16_t)GetChannelPulseWidth(channel) - center);
        int32_t scaled = 128 * (int32_t)value / range;
        return Saturate(scaled);
    }

private:
    int16_t Polarity(uint8_t channel, int16_t value) const
    {
        return (m_Configuration.m_polarity & (1 << channel)) == 0 ? value : -value;
    }

    int8_t Saturate(int32_t value) const
    {
        if (value < INT8_MIN)
        {
            return INT8_MIN;
        }
        else if (value > INT8_MAX)
        {
            return INT8_MAX;
        }
        else
        {
            return value;
        }
    }

    uint16_t UsToTicks(uint16_t value) const
    {
#if defined (__AVR_ATtiny85__)
        return value * (2 * F_CPU / 1000000) / (64 * 2);
#else
        return value * (F_CPU / 1000000) / 8;
#endif
    }

public:
    PpmConfiguration m_Configuration;
    uint32_t m_frequency;

private:
    uint16_t m_minSyncPulseWidth = 0;
    uint16_t m_centerChannelPulseWidth = 0;
    uint16_t m_channelPulseWidthRange = 0;
    uint16_t m_lastTime = 0;
    volatile uint16_t m_syncPulseWidth = 0;
    volatile uint16_t m_channelPulseWidth[PpmConfiguration::maxChannels] = {};
    volatile uint8_t m_updateCounter = 0;
    uint8_t m_currentChannel = 0;
    uint8_t m_lastUpdateCount = 0;
    uint32_t m_lastUpdateTime = 0;
    bool m_isDataAvailable = false;
};
