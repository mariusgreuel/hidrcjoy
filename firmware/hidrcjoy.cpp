//
// hidrcjoy.cpp
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#include <stdint.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#if defined(USB_V_USB)
extern "C" {
#include "usbdrv/usbdrv.h"
}
#elif defined(USB_LUFA)
#include <LUFA/Drivers/USB/USB.h>
#include "Descriptors.h"
#endif

#include "Timer.h"
#include "PpmDecoder.h"
#include "UsbReports.h"

/////////////////////////////////////////////////////////////////////////////

#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))

#if defined (BOARD_Digispark)
#define PPM_SIGNAL_PIN PINB
#define PPM_SIGNAL_PORT PORTB
#define PPM_SIGNAL 2 // Pin 2
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 1 // Pin 1 (built-in LED)
#elif defined (BOARD_DigisparkPro)
#define PPM_SIGNAL_PIN PINA
#define PPM_SIGNAL_PORT PORTA
#define PPM_SIGNAL 4
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 1 // Pin 1 (built-in LED)
#elif defined (BOARD_FabISP)
#define PPM_SIGNAL_PIN PINA
#define PPM_SIGNAL_PORT PORTA
#define PPM_SIGNAL 6 // ADC6/MOSI
#define LED_STATUS_DDR DDRA
#define LED_STATUS_PORT PORTA
#define LED_STATUS 5 // PA5/MISO
#elif defined (BOARD_ProMicro)
#define PPM_SIGNAL_PIN PIND
#define PPM_SIGNAL_PORT PORTD
#define PPM_SIGNAL 4 // Pin 4
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 0 // Pin 17 (built-in Rx LED)
#else
#error Unsupported board
#endif

//---------------------------------------------------------------------------

static Timer g_Timer;
static PpmDecoder g_PpmDecoder;
static UsbReport g_UsbReport;
static UsbEnhancedReport g_UsbEnhancedReport;
static PpmConfiguration g_EepromConfiguration __attribute__((section(".eeprom")));

//---------------------------------------------------------------------------

static void PrepareUsbReport()
{
    bool isDataAvailable = g_PpmDecoder.IsDataAvailable();

    g_UsbReport.m_reportId = UsbReportId;
    for (uint8_t i = 0; i < COUNTOF(g_UsbReport.m_value); i++)
    {
        g_UsbReport.m_value[i] = isDataAvailable ? g_PpmDecoder.GetValue(i) : 0;
    }
}

static void PrepareUsbEnhancedReport()
{
    bool isDataAvailable = g_PpmDecoder.IsDataAvailable();

    g_UsbEnhancedReport.m_reportId = UsbEnhancedReportId;
    g_UsbEnhancedReport.m_frequency = g_PpmDecoder.m_frequency;
    g_UsbEnhancedReport.m_syncPulseWidth = isDataAvailable ? g_PpmDecoder.GetSyncPulseWidth() : 0;

    for (uint8_t i = 0; i < COUNTOF(g_UsbEnhancedReport.m_channelPulseWidth); i++)
    {
        g_UsbEnhancedReport.m_channelPulseWidth[i] = isDataAvailable ? g_PpmDecoder.GetChannelPulseWidth(i) : 0;
    }
}

static void LoadConfigurationDefaults()
{
    g_PpmDecoder.LoadDefaultConfiguration();
    g_PpmDecoder.ApplyConfiguration();
}

static void ReadConfigurationFromEeprom()
{
    eeprom_read_block(&g_PpmDecoder.m_Configuration, &g_EepromConfiguration, sizeof(g_EepromConfiguration));

    if (!g_PpmDecoder.IsValidConfiguration())
    {
        g_PpmDecoder.LoadDefaultConfiguration();
    }

    g_PpmDecoder.ApplyConfiguration();
}

static void WriteConfigurationToEeprom()
{
    eeprom_write_block(&g_PpmDecoder.m_Configuration, &g_EepromConfiguration, sizeof(g_EepromConfiguration));
}

static void JumpToBootloader()
{
    asm volatile ("rjmp __vectors - 4"); // jump to application reset vector at end of flash
}

//---------------------------------------------------------------------------
// USB

#if defined(USB_V_USB)

const uint8_t usbHidReportDescriptor[] PROGMEM =
{
    0x05, 0x01,         // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,         // USAGE (Joystick)
    0xA1, 0x01,         // COLLECTION (Application)
    0x09, 0x01,         //   USAGE (Pointer)
    0x85, UsbReportId,  //   REPORT_ID (UsbReportId)
    0x75, 0x08,         //   REPORT_SIZE (8)
    0x15, 0x80,         //   LOGICAL_MINIMUM (-128)
    0x25, 0x7F,         //   LOGICAL_MAXIMUM (127)
    0xA1, 0x00,         //   COLLECTION (Physical)
    0x09, 0x30,         //     USAGE (X)
    0x09, 0x31,         //     USAGE (Y)
    0x95, 0x02,         //     REPORT_COUNT (2)
    0x81, 0x02,         //     INPUT (Data,Var,Abs)
    0xC0,               //   END_COLLECTION
    0xA1, 0x00,         //   COLLECTION (Physical)
    0x09, 0x32,         //     USAGE (Z)
    0x09, 0x33,         //     USAGE (Rx)
    0x95, 0x02,         //     REPORT_COUNT (2)
    0x81, 0x02,         //     INPUT (Data,Var,Abs)
    0xC0,               //   END_COLLECTION
    0xA1, 0x00,         //   COLLECTION (Physical)
    0x09, 0x34,         //     USAGE (Ry)
    0x09, 0x35,         //     USAGE (Rz)
    0x09, 0x36,         //     USAGE (Slider)
    0x95, 0x03,         //     REPORT_COUNT (3)
    0x81, 0x02,         //     INPUT (Data,Var,Abs)
    0xC0,               //   END_COLLECTION
    0xA1, 0x02,         //   COLLECTION (Logical)
    0x06, 0x00, 0xFF,   //     USAGE_PAGE (Vendor Defined Page 1)
    0x85, UsbEnhancedReportId, // REPORT_ID (UsbEnhancedReportId)
    0x95, sizeof(UsbEnhancedReport), // REPORT_COUNT (...)
    0x09, 0x00,         //     USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, ConfigurationReportId, // REPORT_ID (...)
    0x95, sizeof(PpmConfiguration), // REPORT_COUNT (...)
    0x09, ConfigurationReportId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, LoadConfigurationDefaultsId, // REPORT_ID (...)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x09, LoadConfigurationDefaultsId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, ReadConfigurationFromEepromId, //REPORT_ID (...)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x09, ReadConfigurationFromEepromId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, WriteConfigurationToEepromId, // REPORT_ID (...)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x09, WriteConfigurationToEepromId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0x85, JumpToBootloaderId, // REPORT_ID (...)
    0x95, 0x01,         //     REPORT_COUNT (1)
    0x09, JumpToBootloaderId, // USAGE (...)
    0xB1, 0x02,         //     FEATURE (Data,Var,Abs)
    0xC0,               //   END_COLLECTION
    0xC0,               // END COLLECTION
};

static_assert(sizeof(usbHidReportDescriptor) == USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH, "USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH size mismatch");

static uint8_t g_UsbWriteReportId;
static uint8_t g_UsbWritePosition;
static uint8_t g_UsbWriteBytesRemaining;

static void SetupUsbWrite(uint8_t reportId, uint8_t transferSize)
{
    g_UsbWriteReportId = reportId;
    g_UsbWritePosition = 0;
    g_UsbWriteBytesRemaining = transferSize;
}

extern "C" uchar usbFunctionWrite(uchar* data, uchar length)
{
    if (g_UsbWriteBytesRemaining == 0)
        return 1; // end of transfer

    if (length > g_UsbWriteBytesRemaining)
        length = g_UsbWriteBytesRemaining;

    memcpy(reinterpret_cast<uint8_t*>(&g_PpmDecoder.m_Configuration) + g_UsbWritePosition, data, length);
    g_UsbWritePosition += length;
    g_UsbWriteBytesRemaining -= length;

    if (g_UsbWriteBytesRemaining == 0)
    {
        g_PpmDecoder.ApplyConfiguration();
    }

    return g_UsbWriteBytesRemaining == 0; // return 1 if this was the last chunk
}

extern "C" usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    static uint8_t idleRate;
    const usbRequest_t* request = (const usbRequest_t*)data;

    if ((request->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
    {
        if (request->bRequest == USBRQ_HID_GET_REPORT)
        {
            uint8_t reportId = request->wValue.bytes[0];
            switch (reportId)
            {
            case UsbReportId:
                PrepareUsbReport();
                usbMsgPtr = (usbMsgPtr_t)&g_UsbReport;
                return sizeof(g_UsbReport);
            case UsbEnhancedReportId:
                PrepareUsbEnhancedReport();
                usbMsgPtr = (usbMsgPtr_t)&g_UsbEnhancedReport;
                return sizeof(g_UsbEnhancedReport);
            case ConfigurationReportId:
                g_PpmDecoder.m_Configuration.m_reportId = ConfigurationReportId;
                usbMsgPtr = (usbMsgPtr_t)&g_PpmDecoder.m_Configuration;
                return sizeof(g_PpmDecoder.m_Configuration);
            default:
                return 0;
            }
        }
        else if (request->bRequest == USBRQ_HID_SET_REPORT)
        {
            uint8_t reportId = request->wValue.bytes[0];
            switch (reportId)
            {
            case ConfigurationReportId:
                SetupUsbWrite(reportId, sizeof(g_PpmDecoder.m_Configuration));
                return USB_NO_MSG;
            case LoadConfigurationDefaultsId:
                LoadConfigurationDefaults();
                return 0;
            case ReadConfigurationFromEepromId:
                ReadConfigurationFromEeprom();
                return 0;
            case WriteConfigurationToEepromId:
                WriteConfigurationToEeprom();
                return 0;
            case JumpToBootloaderId:
                JumpToBootloader();
                return 0;
            default:
                return 0;
            }
        }
        else if (request->bRequest == USBRQ_HID_GET_IDLE)
        {
            usbMsgPtr = (usbMsgPtr_t)&idleRate;
            return 1;
        }
        else if (request->bRequest == USBRQ_HID_SET_IDLE)
        {
            idleRate = request->wValue.bytes[1];
        }
    }

    return 0;
}

static void InitializeUsb(void)
{
    usbInit();
    usbDeviceDisconnect();

    for (int i = 0; i < 256; i++)
    {
        wdt_reset();
        _delay_ms(1);
    }

    usbDeviceConnect();
}

static void ProcessUsb(void)
{
    usbPoll();

    if (usbInterruptIsReady())
    {
        PrepareUsbReport();
        usbSetInterrupt((uchar*)&g_UsbReport, sizeof(g_UsbReport));
    }
}

#elif defined(USB_LUFA)

void EVENT_USB_Device_ControlRequest(void)
{
    if (!Endpoint_IsSETUPReceived())
        return;

    switch (USB_ControlRequest.bRequest)
    {
    case HID_REQ_GetReport:
        if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            uint8_t reportId = (USB_ControlRequest.wValue & 0xFF);
            switch (reportId)
            {
            case UsbReportId:
                PrepareUsbReport();
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&g_UsbReport, sizeof(g_UsbReport));
                Endpoint_ClearOUT();
                break;
            case UsbEnhancedReportId:
                PrepareUsbEnhancedReport();
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&g_UsbEnhancedReport, sizeof(g_UsbEnhancedReport));
                Endpoint_ClearOUT();
                break;
            case ConfigurationReportId:
                g_PpmDecoder.m_Configuration.m_reportId = ConfigurationReportId;
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&g_PpmDecoder.m_Configuration, sizeof(g_PpmDecoder.m_Configuration));
                Endpoint_ClearOUT();
                break;
            }
        }
        break;
    case HID_REQ_SetReport:
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
        {
            uint8_t reportId = (USB_ControlRequest.wValue & 0xFF);
            switch (reportId)
            {
            case ConfigurationReportId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&g_PpmDecoder.m_Configuration, sizeof(g_PpmDecoder.m_Configuration));
                Endpoint_ClearIN();
                break;
            case LoadConfigurationDefaultsId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&reportId, sizeof(reportId));
                Endpoint_ClearIN();
                LoadConfigurationDefaults();
                break;
            case ReadConfigurationFromEepromId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&reportId, sizeof(reportId));
                Endpoint_ClearIN();
                ReadConfigurationFromEeprom();
                break;
            case WriteConfigurationToEepromId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&reportId, sizeof(reportId));
                Endpoint_ClearIN();
                WriteConfigurationToEeprom();
                break;
            case JumpToBootloaderId:
                Endpoint_ClearSETUP();
                Endpoint_Read_Control_Stream_LE(&reportId, sizeof(reportId));
                Endpoint_ClearIN();
                JumpToBootloader();
                break;
            }
        }
        break;
    }
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
    Endpoint_ConfigureEndpoint(JOYSTICK_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
}

static void InitializeUsb(void)
{
    USB_Init();
}

static void ProcessUsb(void)
{
    USB_USBTask();

    if (USB_DeviceState == DEVICE_STATE_Configured)
    {
        Endpoint_SelectEndpoint(JOYSTICK_EPADDR);

        if (Endpoint_IsINReady())
        {
            PrepareUsbReport();
            Endpoint_Write_Stream_LE(&g_UsbReport, sizeof(g_UsbReport), NULL);
            Endpoint_ClearIN();
        }
    }
}

#endif

//---------------------------------------------------------------------------

static void InitializePorts(void)
{
    // LED_STATUS is output
    LED_STATUS_DDR = _BV(LED_STATUS);

    // Pull-up on PPM_SIGNAL
    PPM_SIGNAL_PORT = _BV(PPM_SIGNAL);
}

//---------------------------------------------------------------------------

#ifndef TIMER0_OVF_vect
#define TIMER0_OVF_vect TIM0_OVF_vect
#endif

ISR(TIMER0_OVF_vect)
{
    g_Timer.Overflow();
}

//---------------------------------------------------------------------------

#if defined (__AVR_ATtiny85__)
static void InitializeUsi(void)
{
    USISR = 0x0F;

    // Outputs disabled, enable counter overflow interrupt, external clock source
    USICR = _BV(USIOIE) | _BV(USIWM1) | _BV(USIWM0) | _BV(USICS1);

    g_PpmDecoder.m_frequency = F_CPU / 64;
}

ISR(USI_OVF_vect)
{
    uint16_t ticks = g_Timer.GetTicksNoCli();
    bool level = (PPM_SIGNAL_PIN & _BV(PPM_SIGNAL)) != 0;

    // Clear counter overflow flag
    USISR = _BV(USIOIF) | 0x0F;

    sei();
    g_PpmDecoder.OnPinChanged(level, ticks);
}
#endif

#if defined (__AVR_ATtiny44__) || defined (__AVR_ATtiny167__) || defined (__AVR_ATmega32U4__)
static void InitializeInputCapture(void)
{
#if defined (BOARD_FabISP)
    // On the FabISP board, the ICP pin is tied up for USB, so use the ADC comparator with ADC6 instead

    // Analog comparator bandgap select, analog comparator input capture enable
    ACSR = _BV(ACBG) | _BV(ACIC);

    // Disable ADC
    ADCSRA = 0;

    // Analog comparator multiplexer enable
    ADCSRB = _BV(ACME);

    // ADC6
    ADMUX = _BV(MUX2) | _BV(MUX1);

    // Noise canceler, input capture rising edge, clk/8
    TCCR1B = _BV(ICNC1) | _BV(CS11);
#else
    // Noise canceler, input capture rising edge, clk/8
    TCCR1B = _BV(ICNC1) | _BV(ICES1) | _BV(CS11);
#endif

    g_PpmDecoder.m_frequency = F_CPU / 8;

    // Input capture interrupt enable
    TIMSK1 = _BV(ICIE1);
}

ISR(TIMER1_CAPT_vect)
{
    uint16_t ticks = ICR1;
    sei();
    g_PpmDecoder.OnPinChanged(true, ticks);
}
#endif

//---------------------------------------------------------------------------

void BlinkStatusLed(bool good, uint32_t time)
{
    static uint32_t lastTime;

    uint32_t period = good ? 50000 : 500000;
    if (time - lastTime > period)
    {
        lastTime = time;
        LED_STATUS_PORT ^= _BV(LED_STATUS);
    }
}

//---------------------------------------------------------------------------

int main(void)
{
    wdt_enable(WDTO_1S);

    InitializePorts();
    g_Timer.Initialize();
#if defined (__AVR_ATtiny85__)
    InitializeUsi();
#elif defined (__AVR_ATtiny44__) || defined (__AVR_ATtiny167__) || defined (__AVR_ATmega32U4__)
    InitializeInputCapture();
#else
#error Unsupported MCU
#endif

    InitializeUsb();
    ReadConfigurationFromEeprom();
    sei();

    for (;;)
    {
        wdt_reset();
        ProcessUsb();

        uint32_t time = g_Timer.GetMicros();
        g_PpmDecoder.Update(time);
        BlinkStatusLed(g_PpmDecoder.IsDataAvailable(), time);
    }

    return 0;
}
