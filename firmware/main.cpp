//
// main.cpp
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#include <stdint.h>
#include <string.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
extern "C" {
#include "usbdrv/usbdrv.h"
}

#include "Timer.h"
#include "PpmDecoder.h"
#include "UsbReports.h"

/////////////////////////////////////////////////////////////////////////////

#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))

#if defined (__AVR_ATtiny85__) // Digispark
#define PPM_SIGNAL_PIN PINB
#define PPM_SIGNAL_PORT PORTB
#define PPM_SIGNAL 2 // P2
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 1 // P1 (built-in)
#elif defined (__AVR_ATtiny167__) // Digispark Pro
#define PPM_SIGNAL_PIN PINA
#define PPM_SIGNAL_PORT PORTA
#define PPM_SIGNAL 4
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 1 // P1 (built-in)
#elif defined (__AVR_ATtiny44__) // FabISP
#define PPM_SIGNAL_PIN PINA
#define PPM_SIGNAL_PORT PORTA
#define PPM_SIGNAL 6 // ADC6/MOSI
#define LED_STATUS_DDR DDRA
#define LED_STATUS_PORT PORTA
#define LED_STATUS 5 // PA5/MISO
#else
#error Unsupported architecture
#endif

//---------------------------------------------------------------------------

static Timer g_Timer;
static PpmDecoder g_PpmDecoder;
static UsbReport g_UsbReport;
static UsbEnhancedReport g_UsbEnhancedReport;

//---------------------------------------------------------------------------

static void PrepareUsbReport()
{
    bool isDataAvailable = g_PpmDecoder.IsDataAvailable();

    for (uint8_t i = 0; i < COUNTOF(g_UsbReport.m_value); i++)
    {
        g_UsbReport.m_value[i] = isDataAvailable ? g_PpmDecoder.GetValue(i) : 0;
    }
}

static void PrepareUsbEnhancedReport()
{
    bool isDataAvailable = g_PpmDecoder.IsDataAvailable();

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
    eeprom_read_block(&g_PpmDecoder.m_Configuration, 0, sizeof(g_PpmDecoder.m_Configuration));

    if (!g_PpmDecoder.IsValidConfiguration())
    {
        g_PpmDecoder.LoadDefaultConfiguration();
    }

    g_PpmDecoder.ApplyConfiguration();
}

static void WriteConfigurationToEeprom()
{
    eeprom_write_block(&g_PpmDecoder.m_Configuration, 0, sizeof(g_PpmDecoder.m_Configuration));
}

static void JumpToBootloader()
{
    asm volatile ("rjmp __vectors - 4"); // jump to application reset vector at end of flash
}

//---------------------------------------------------------------------------
// USB

PROGMEM const uint8_t usbHidReportDescriptor[] =
{
    0x05, 0x01,                     // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                     // USAGE (Joystick)
    0xA1, 0x01,                     // COLLECTION (Application)
    0x09, 0x01,                     //   USAGE (Pointer)
    0x85, 0x01,                     //   REPORT_ID (1)
    0x75, 0x08,                     //   REPORT_SIZE (8)
    0x15, 0x80,                     //   LOGICAL_MINIMUM (-128)
    0x25, 0x7F,                     //   LOGICAL_MAXIMUM (127)
    0xA1, 0x00,                     //   COLLECTION (Physical)
    0x09, 0x30,                     //     USAGE (X)
    0x09, 0x31,                     //     USAGE (Y)
    0x95, 0x02,                     //     REPORT_COUNT (2)
    0x81, 0x02,                     //     INPUT (Data,Var,Abs)
    0xC0,                           //   END_COLLECTION
    0xA1, 0x00,                     //   COLLECTION (Physical)
    0x09, 0x32,                     //     USAGE (Z)
    0x09, 0x33,                     //     USAGE (Rx)
    0x95, 0x02,                     //     REPORT_COUNT (2)
    0x81, 0x02,                     //     INPUT (Data,Var,Abs)
    0xC0,                           //   END_COLLECTION
    0xA1, 0x00,                     //   COLLECTION (Physical)
    0x09, 0x34,                     //     USAGE (Ry)
    0x09, 0x35,                     //     USAGE (Rz)
    0x09, 0x36,                     //     USAGE (Slider)
    0x95, 0x03,                     //     REPORT_COUNT (3)
    0x81, 0x02,                     //     INPUT (Data,Var,Abs)
    0xC0,                           //   END_COLLECTION
    0xA1, 0x02,                     //   COLLECTION (Logical)
    0x06, 0x00, 0xFF,               //     USAGE_PAGE (Vendor Defined Page 1)
    0x85, UsbEnhancedReportId,      //     REPORT_ID (UsbEnhancedReportId)
    0x95, sizeof(g_UsbEnhancedReport), // REPORT_COUNT (...)
    0x09, 0x00,                     //     USAGE (Undefined)
    0xB1, 0x02,                     //     FEATURE (Data,Var,Abs)
    0x85, ConfigurationId,          //     REPORT_ID (...)
    0x95, sizeof(PpmConfiguration), //     REPORT_COUNT (...)
    0x09, 0x00,                     //     USAGE (Undefined)
    0xB1, 0x02,                     //     FEATURE (Data,Var,Abs)
    0x85, LoadConfigurationDefaultsId, //  REPORT_ID (...)
    0x95, 0x01,                     //     REPORT_COUNT (1)
    0x09, 0x00,                     //     USAGE (Undefined)
    0xB1, 0x02,                     //     FEATURE (Data,Var,Abs)
    0x85, ReadConfigurationFromEepromId, //REPORT_ID (...)
    0x95, 0x01,                     //     REPORT_COUNT (1)
    0x09, 0x00,                     //     USAGE (Undefined)
    0xB1, 0x02,                     //     FEATURE (Data,Var,Abs)
    0x85, WriteConfigurationToEepromId, // REPORT_ID (...)
    0x95, 0x01,                     //     REPORT_COUNT (1)
    0x09, 0x00,                     //     USAGE (Undefined)
    0xB1, 0x02,                     //     FEATURE (Data,Var,Abs)
    0x85, JumpToBootloaderId,       //     REPORT_ID (...)
    0x95, 0x01,                     //     REPORT_COUNT (1)
    0x09, 0x00,                     //     USAGE (Undefined)
    0xB1, 0x02,                     //     FEATURE (Data,Var,Abs)
    0xC0,                           //   END_COLLECTION
    0xC0,                           // END COLLECTION
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

    if (g_UsbWritePosition == 0 && length > 0)
    {
        // Skip the report ID
        data++;
        length--;
    }

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
                usbMsgPtr = (usbMsgPtr_t)&g_UsbReport;
                return sizeof(g_UsbReport);
            case UsbEnhancedReportId:
                PrepareUsbEnhancedReport();
                usbMsgPtr = (usbMsgPtr_t)&g_UsbEnhancedReport;
                return sizeof(g_UsbEnhancedReport);
            case ConfigurationId:
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
            case ConfigurationId:
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

#if defined (__AVR_ATtiny44__) || defined (__AVR_ATtiny167__)
static void InitializeInputCapture(void)
{
#if defined (__AVR_ATtiny44__)
    // On the FabISP board, the ICP pin is tied up for USB, so use the ADC comparator with ADC6 instead

    // Analog comparator bandgap select, analog comparator input capture enable
    ACSR = _BV(ACBG) | _BV(ACIC);

    // Disable ADC
    ADCSRA = 0;

    // Analog comparator multiplexer enable
    ADCSRB = _BV(ACME);

    // ADC6
    ADMUX = _BV(MUX2) | _BV(MUX1);
#endif

    // Noise canceler, input capture rising edge, clk/8
#if defined (__AVR_ATtiny44__)
    TCCR1B = _BV(ICNC1) | _BV(CS11);
#else
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
#elif defined (__AVR_ATtiny44__) || defined (__AVR_ATtiny167__)
    InitializeInputCapture();
#else
#error Unsupported architecture
#endif

    usbInit();
    usbDeviceDisconnect();

    for (int i = 0; i < 256; i++)
    {
        wdt_reset();
        _delay_ms(1);
    }

    ReadConfigurationFromEeprom();

    usbDeviceConnect();
    sei();

    for (;;)
    {
        wdt_reset();
        usbPoll();

        uint32_t time = g_Timer.GetMicros();
        g_PpmDecoder.Update(time);

        BlinkStatusLed(g_PpmDecoder.IsDataAvailable(), time);

        if (usbInterruptIsReady())
        {
            PrepareUsbReport();
            usbSetInterrupt((uchar*)&g_UsbReport, sizeof(g_UsbReport));
        }
    }

    return 0;
}
