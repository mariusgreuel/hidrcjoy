//
// configuration.h
// Copyright (C) 2018 Marius Greuel
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once
#include <stdint.h>

struct Configuration
{
    static constexpr uint8_t Version = 20;
    static constexpr uint8_t MaxInputChannels = 9;
    static constexpr uint8_t MaxOutputChannels = 7;
    static constexpr uint16_t DefaultSyncWidth = 3500;
    static constexpr uint16_t DefaultPulseWidthCenter = 1500;
    static constexpr uint16_t DefaultPulseWidthRange = 550;
    static constexpr uint16_t MinSyncWidth = 2000;
    static constexpr uint16_t MaxSyncWidth = 10000;
    static constexpr uint16_t MinPulseWidth = 500;
    static constexpr uint16_t MaxPulseWidth = 3000;

    static_assert(MaxOutputChannels <= 7, "MaxOutputChannels must not exceed 7");
    static_assert(MaxOutputChannels <= MaxInputChannels, "MaxInputChannels must not exceed 9");

    enum Flags
    {
        InvertedSignal = 1,
    };

    uint8_t reportId = 0;
    uint8_t version = 0;
    uint8_t flags = 0;
    uint8_t dummy = 0;
    uint16_t minSyncWidth = 0;
    uint16_t pulseWidthCenter = 0;
    uint16_t pulseWidthRange = 0;
    uint16_t clockCorrection = 0;
    uint8_t invert = 0;
    uint8_t mapping[MaxOutputChannels] = {};
};
