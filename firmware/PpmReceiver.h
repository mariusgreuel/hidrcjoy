//
// PpmReceiver.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include "Configuration.h"

/////////////////////////////////////////////////////////////////////////////

class PpmReceiver
{
    static const uint32_t signalTimeout = 100000;

public:
    void Initialize(void)
    {
    }

    void SetConfiguration(uint16_t minSyncPulseWidth, bool invertedSignal)
    {
        m_minSyncPulseWidth = UsToTicks(minSyncPulseWidth);
        m_invertedSignal = invertedSignal;
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
        if (level == m_invertedSignal)
            return;

        uint16_t diff = time - m_lastTime;
        m_lastTime = time;

        if (diff >= m_minSyncPulseWidth)
        {
            m_currentChannel = 0;
            m_updateCounter++;
        }
        else if (m_currentChannel < Configuration::maxChannels)
        {
            cli();
            m_channelPulseWidth[m_currentChannel] = diff;
            sei();
            m_currentChannel++;
        }
    }

    bool IsDataAvailable() const
    {
        return m_isDataAvailable;
    }

    uint16_t GetChannelPulseWidth(uint8_t channel) const
    {
        cli();
        uint16_t value = m_channelPulseWidth[channel];
        sei();
        return TicksToUs(value);
    }

    uint16_t TicksToUs(uint16_t value) const
    {
#if defined (__AVR_ATtiny85__)
        return value * (64 * 2) / (2 * F_CPU / 1000000);
#else
        return value * 8 / (F_CPU / 1000000);
#endif
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
    uint32_t m_frequency;

private:
    uint16_t m_minSyncPulseWidth = 0;
    bool m_invertedSignal = false;
    uint16_t m_lastTime = 0;
    volatile uint16_t m_channelPulseWidth[Configuration::maxChannels] = {};
    uint8_t m_currentChannel = 0;
    volatile uint8_t m_updateCounter = 0;
    uint8_t m_lastUpdateCount = 0;
    uint32_t m_lastUpdateTime = 0;
    bool m_isDataAvailable = false;
};
