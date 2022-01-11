#pragma once

namespace wiringpi
{
    enum EPWMMode
    {
        PWM_MODE_MS	= 0,
        PWM_MODE_BAL = 1
    };

    enum EPinMode
    {
        INPUT = 0,
        OUTPUT = 1,
        PWM_OUTPUT = 2,
        GPIO_CLOCK = 3,
        SOFT_PWM_OUTPUT = 4,
        SOFT_TONE_OUTPUT = 5,
        PWM_TONE_OUTPUT = 6
    };

    enum EPinValue
    {
        LOW = 0,
        HIGH = 1
    };
}