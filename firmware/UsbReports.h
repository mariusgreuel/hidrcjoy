//
// UsbReports.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

#include "PpmConfiguration.h"

enum ReportIds
{
    UnusedId,
    UsbReportId,
    UsbEnhancedReportId,
    ConfigurationId,
    LoadConfigurationDefaultsId,
    ReadConfigurationFromEepromId,
    WriteConfigurationToEepromId,
    JumpToBootloaderId,
};

struct UsbReport
{
    uint8_t m_reportId = UsbReportId;
    int8_t m_value[PpmConfiguration::maxChannels];
};

static_assert(sizeof(UsbReport) <= 8, "Report size for low-speed devices may not exceed 8 bytes");

struct UsbEnhancedReport
{
    uint32_t m_frequency;
    uint16_t m_syncPulseWidth;
    uint16_t m_channelPulseWidth[PpmConfiguration::maxChannels];
};
