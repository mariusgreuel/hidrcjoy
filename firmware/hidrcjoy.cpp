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

/////////////////////////////////////////////////////////////////////////////

#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))

#if defined (BOARD_Digispark)
#define HIDRCJOY_SRXL 0
#define PPM_SIGNAL_PIN PINB
#define PPM_SIGNAL_PORT PORTB
#define PPM_SIGNAL 2 // Pin 2
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 1 // Pin 1 (built-in LED)
#elif defined (BOARD_DigisparkPro)
#define HIDRCJOY_SRXL 0
#define PPM_SIGNAL_PIN PINA
#define PPM_SIGNAL_PORT PORTA
#define PPM_SIGNAL 4
#define LED_STATUS_DDR DDRB
#define LED_STATUS_PORT PORTB
#define LED_STATUS 1 // Pin 1 (built-in LED)
#elif defined (BOARD_FabISP)
#define HIDRCJOY_SRXL 0
#define PPM_SIGNAL_PIN PINA
#define PPM_SIGNAL_PORT PORTA
#define PPM_SIGNAL 6 // ADC6/MOSI
#define LED_STATUS_DDR DDRA
#define LED_STATUS_PORT PORTA
#define LED_STATUS 5 // PA5/MISO
#elif defined (BOARD_ProMicro)
#define HIDRCJOY_SRXL 1
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

#include "Timer.h"
#include "Receiver.h"
#include "UsbReports.h"

//---------------------------------------------------------------------------

static Timer g_Timer;
static Receiver g_Receiver;
static UsbReport g_UsbReport;
static UsbEnhancedReport g_UsbEnhancedReport;
static Configuration g_EepromConfiguration __attribute__((section(".eeprom")));

//---------------------------------------------------------------------------

static void PrepareUsbReport()
{
    bool hasData = g_Receiver.GetStatus() != NoSignal;

    g_UsbReport.m_reportId = UsbReportId;
    for (uint8_t i = 0; i < COUNTOF(g_UsbReport.m_value); i++)
    {
        g_UsbReport.m_value[i] = hasData ? g_Receiver.GetValue(i) : 0x80;
    }
}

static void PrepareUsbEnhancedReport()
{
    g_UsbEnhancedReport.m_reportId = UsbEnhancedReportId;
    g_UsbEnhancedReport.m_status = g_Receiver.GetStatus();

    for (uint8_t i = 0; i < COUNTOF(g_UsbEnhancedReport.m_channelPulseWidth); i++)
    {
        g_UsbEnhancedReport.m_channelPulseWidth[i] = g_UsbEnhancedReport.m_status != 0 ? g_Receiver.GetChannelPulseWidth(i) : 0;
    }
}

static void LoadConfigurationDefaults()
{
    g_Receiver.LoadDefaultConfiguration();
    g_Receiver.UpdateConfiguration();
}

static void ReadConfigurationFromEeprom()
{
    eeprom_read_block(&g_Receiver.m_Configuration, &g_EepromConfiguration, sizeof(g_EepromConfiguration));

    if (!g_Receiver.IsValidConfiguration())
    {
        g_Receiver.LoadDefaultConfiguration();
    }

    g_Receiver.UpdateConfiguration();
}

static void WriteConfigurationToEeprom()
{
    eeprom_write_block(&g_Receiver.m_Configuration, &g_EepromConfiguration, sizeof(g_EepromConfiguration));
}

static void JumpToBootloader()
{
    asm volatile ("rjmp __vectors - 4"); // jump to application reset vector at end of flash
}

//---------------------------------------------------------------------------
// USB

#if defined(USB_V_USB)

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

    memcpy(reinterpret_cast<uint8_t*>(&g_Receiver.m_Configuration) + g_UsbWritePosition, data, length);
    g_UsbWritePosition += length;
    g_UsbWriteBytesRemaining -= length;

    if (g_UsbWriteBytesRemaining == 0)
    {
        g_Receiver.UpdateConfiguration();
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
                g_Receiver.m_Configuration.m_reportId = ConfigurationReportId;
                usbMsgPtr = (usbMsgPtr_t)&g_Receiver.m_Configuration;
                return sizeof(g_Receiver.m_Configuration);
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
                SetupUsbWrite(reportId, sizeof(g_Receiver.m_Configuration));
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
                g_Receiver.m_Configuration.m_reportId = ConfigurationReportId;
                Endpoint_ClearSETUP();
                Endpoint_Write_Control_Stream_LE(&g_Receiver.m_Configuration, sizeof(g_Receiver.m_Configuration));
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
                Endpoint_Read_Control_Stream_LE(&g_Receiver.m_Configuration, sizeof(g_Receiver.m_Configuration));
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
}

ISR(USI_OVF_vect)
{
    uint16_t ticks = g_Timer.GetTicksNoCli();
    bool level = (PPM_SIGNAL_PIN & _BV(PPM_SIGNAL)) != 0;

    // Clear counter overflow flag
    USISR = _BV(USIOIF) | 0x0F;

    sei();
    g_Receiver.m_PpmReceiver.OnPinChanged(level, ticks);
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

    // Input capture interrupt enable
    TIMSK1 = _BV(ICIE1);
}

ISR(TIMER1_CAPT_vect)
{
    uint16_t ticks = ICR1;
    sei();
    g_Receiver.m_PpmReceiver.OnPinChanged(true, ticks);
}
#endif

#if HIDRCJOY_SRXL
ISR(USART1_RX_vect)
{
    uint32_t time = g_Timer.GetMicros();
    g_Receiver.m_SrxlReceiver.OnDataReceived(time);
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
    g_Receiver.Initialize();
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
        g_Receiver.Update(time);
        BlinkStatusLed(g_Receiver.GetStatus() != NoSignal, time);
    }

    return 0;
}
