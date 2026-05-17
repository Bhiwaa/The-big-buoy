/*
 * fusion.c
 *
 *  Created on: May 6, 2026
 *      Author: jamie
 */

#include "fusion.h"

void fusion_init(Orientation_t *ori)
{
    if (ori == 0)
    {
        return;
    }

    ori->roll_deg = 0.0f;
    ori->pitch_deg = 0.0f;
    ori->yaw_deg = 0.0f;
}

void fusion_update(const ImuSample_t *imu, float dt_s, Orientation_t *ori)
{
    if ((imu == 0) || (ori == 0))
    {
        return;
    }

    /* Placeholder fusion:
       just integrate gyro rates [deg/s] into Euler angles [deg].
       This is not the final algorithm. */
    ori->roll_deg  += imu->gx * dt_s;
    ori->pitch_deg += imu->gy * dt_s;
    ori->yaw_deg   += imu->gz * dt_s;

    /* Keep yaw in a manageable range */
    if (ori->yaw_deg > 180.0f)
    {
        ori->yaw_deg -= 360.0f;
    }
    else if (ori->yaw_deg < -180.0f)
    {
        ori->yaw_deg += 360.0f;
    }
}
