//
// Timer.h
// Copyright (C) 2018 Marius Greuel. All rights reserved.
//

#pragma once
#include <stdint.h>
#include <avr/io.h>

/////////////////////////////////////////////////////////////////////////////

class Timer
{
public:
    void Initialize()
    {
        // Use timer0 Fast PWM, clk/64
        GTCCR = 0;
        TCCR0A = _BV(WGM01) | _BV(WGM00);
        TCCR0B = _BV(CS01) | _BV(CS00);

        // Enable timer0 overflow interrupt
#if defined (TIMSK)
        TIMSK = _BV(TOIE0);
#elif defined (TIMSK0)
        TIMSK0 = _BV(TOIE0);
#else
#error Unsupported architecture
#endif
    }

    void Overflow()
    {
        m_overflows++;
    }

    static uint32_t GetFrequency()
    {
        return F_CPU / 64;
    }

    uint32_t GetMicros() const
    {
        return TicksToUs(GetTicks());
    }

    uint32_t GetTicks() const
    {
        uint8_t oldSREG = SREG;
        cli();
        uint32_t ticks = GetTicksNoCli();
        SREG = oldSREG;
        return ticks;
    }

    uint32_t GetTicksNoCli() const
    {
        uint32_t overflows = m_overflows;
        uint8_t ticks = TCNT0;
#if defined (TIFR)
        if ((TIFR & _BV(TOV0)) && ticks < 255)
            overflows++;
#elif defined (TIFR0)
        if ((TIFR0 & _BV(TOV0)) && ticks < 255)
            overflows++;
#else
#error Unsupported architecture
#endif

        return (overflows << 8) + ticks;
    }

    static uint32_t TicksToUs(uint32_t value)
    {
        return value * (64 * 2) / (2 * F_CPU / 1000000);
    }

    static uint32_t UsToTicks(uint32_t value)
    {
        return value * (2 * F_CPU / 1000000) / (64 * 2);
    }

private:
    volatile uint32_t m_overflows = 0;
};
