/*
 * imu.h
 *
 *  Created on: May 6, 2026
 *      Author: jamie
 */

#ifndef IMU_H
#define IMU_H

#include <stdint.h>

typedef struct
{
    uint32_t t_ms;

    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    float mx;
    float my;
    float mz;
} ImuSample_t;

void imu_init(void);
void imu_read(ImuSample_t *sample);

#endif
