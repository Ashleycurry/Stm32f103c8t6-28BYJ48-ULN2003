#include "stepmotor.h"

#define MOTOR_PORT  GPIOA
#define MOTOR_PINS  (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3)

static const uint8_t StepSeq[8] = {
    0x01, 0x03, 0x02, 0x06, 0x04, 0x0C, 0x08, 0x09
};

static void Delay_us(uint32_t us)
{
    uint32_t i;
    while (us--)
        for (i = 0; i < 8; i++) __NOP();
}

void StepMotor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin   = MOTOR_PINS;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(MOTOR_PORT, &GPIO_InitStructure);

    GPIO_ResetBits(MOTOR_PORT, MOTOR_PINS);
}

void StepMotor_Rotate(int32_t steps)
{
    static uint8_t phase = 0;
    int32_t dir = (steps >= 0) ? 1 : -1;
    uint32_t n  = (steps >= 0) ? steps : -steps;

    while (n--)
    {
        phase = (phase + dir) & 0x07;
        MOTOR_PORT->ODR = (MOTOR_PORT->ODR & ~MOTOR_PINS) | StepSeq[phase];
        Delay_us(1500);
    }

    GPIO_ResetBits(MOTOR_PORT, MOTOR_PINS);
}
