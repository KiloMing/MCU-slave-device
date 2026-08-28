#ifndef __ENCODER_H__
#define __ENCODER_H__

#include "main.h"
#include "tim.h"

// Per-sample encoder pulse delta and converted speed.
extern int32_t encoder_left_count;
extern int32_t encoder_right_count;
extern int32_t encoder_left_speed;
extern int32_t encoder_right_speed;

void encoder_init(void);
int32_t encoder_get_left_count(void);
int32_t encoder_get_right_count(void);
void encoder_reset_left(void);
void encoder_reset_right(void);
void encoder_calculate_speed(void);

#endif
