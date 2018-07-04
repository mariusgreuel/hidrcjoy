//
// PpmConfiguration.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

struct PpmConfiguration
{
    static const uint8_t version = 0x10;
    static const uint8_t maxChannels = 7;
    static const uint16_t minSyncWidth = 2000;
    static const uint16_t maxSyncWidth = 10000;
    static const uint16_t minChannelPulseWidth = 500;
    static const uint16_t maxChannelPulseWidth = 3000;

    enum Flags
    {
        Normal = 0,
        Inverted = 1,
        Default = Normal
    };

    uint8_t m_version;
    uint8_t m_flags;
    uint16_t m_minSyncPulseWidth;
    uint16_t m_centerChannelPulseWidth;
    uint16_t m_channelPulseWidthRange;
    uint8_t m_polarity;
    uint8_t m_mapping[maxChannels];
};
