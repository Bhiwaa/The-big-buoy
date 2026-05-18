/*
 * imu.c
 *
 *  Created on: May 6, 2026
 *      Author: jamie
 */


#include "imu.h"

void imu_init(void)
{
    /* Placeholder for future real IMU init */
}

void imu_read(ImuSample_t *sample)
{
    static uint32_t k = 0;

    if (sample == 0)
    {
        return;
    }

    sample->t_ms = k * 100U;   /* pretend we sample every 100 ms */

    /* Simple dummy signals for testing */
    {
        uint32_t p1 = k % 40U;
        uint32_t p2 = k % 25U;
        uint32_t p3 = k % 60U;

        float tri1 = (p1 < 20U) ? ((float)p1 / 20.0f) : ((float)(40U - p1) / 20.0f);
        float tri2 = (p2 < 12U) ? ((float)p2 / 12.0f) : ((float)(25U - p2) / 13.0f);
        float tri3 = (p3 < 30U) ? ((float)p3 / 30.0f) : ((float)(60U - p3) / 30.0f);

        float c1 = 2.0f * tri1 - 1.0f;
        float c2 = 2.0f * tri2 - 1.0f;
        float c3 = 2.0f * tri3 - 1.0f;

        /* Accelerometer [m/s^2] */
        sample->ax = 0.30f * c1;
        sample->ay = 0.20f * c2;
        sample->az = 9.81f + 0.15f * c3;

        /* Gyroscope [deg/s] */
        sample->gx = 5.0f * c2;
        sample->gy = 3.0f * c3;
        sample->gz = 8.0f * c1;

        /* Magnetometer [uT] */
        sample->mx = 25.0f + 2.0f * c1;
        sample->my = -5.0f + 1.5f * c2;
        sample->mz = 40.0f + 1.0f * c3;
    }

    k++;
}

