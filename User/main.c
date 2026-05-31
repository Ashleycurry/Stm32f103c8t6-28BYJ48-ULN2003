#include "stm32f10x.h"
#include "stepmotor/stepmotor.h"

static void Delay_ms(uint32_t ms)
{
    uint32_t i;
    while (ms--)
        for (i = 0; i < 8000; i++) __NOP();
}

int main(void)
{
    StepMotor_Init();

    StepMotor_Rotate(STEPS_PER_REV);
    Delay_ms(500);
    StepMotor_Rotate(-STEPS_PER_REV / 2);

    while (1) { }
}
