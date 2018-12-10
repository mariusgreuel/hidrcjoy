//
// UsbReports.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////

#include "Configuration.h"

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

enum Status
{
    NoSignal,
    PpmSignal,
    SrxlSignal,
};

struct UsbReport
{
    uint8_t m_reportId;
    uint8_t m_value[MAX_CHANNELS];
};

#ifdef __cplusplus
static_assert(sizeof(UsbReport) <= 8, "Report size for low-speed devices may not exceed 8 bytes");
#endif

struct UsbEnhancedReport
{
    uint8_t m_reportId;
    uint8_t m_status;
    uint16_t m_channelPulseWidth[MAX_CHANNELS];
};
