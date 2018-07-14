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
    ConfigurationReportId,
    LoadConfigurationDefaultsId,
    ReadConfigurationFromEepromId,
    WriteConfigurationToEepromId,
    JumpToBootloaderId,
};

struct UsbReport
{
    uint8_t m_reportId;
    int8_t m_value[MAX_CHANNELS];
};

#ifdef __cplusplus
static_assert(sizeof(UsbReport) <= 8, "Report size for low-speed devices may not exceed 8 bytes");
#endif

struct UsbEnhancedReport
{
    uint8_t m_reportId;
    uint8_t m_dummy[3];
    uint32_t m_frequency;
    uint16_t m_syncPulseWidth;
    uint16_t m_channelPulseWidth[MAX_CHANNELS];
};
