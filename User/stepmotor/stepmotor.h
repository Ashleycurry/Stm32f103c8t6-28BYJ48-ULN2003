#ifndef __STEPMOTOR_H
#define __STEPMOTOR_H

#include "stm32f10x.h"

#define STEPS_PER_REV  4096

void StepMotor_Init(void);
void StepMotor_Rotate(int32_t steps);

#endif
