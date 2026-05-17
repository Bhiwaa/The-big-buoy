/*
 * nav.c
 *
 *  Created on: May 6, 2026
 *      Author: jamie
 */

#include "nav.h"

void nav_init(NavAccel_t *nav)
{
    if (nav == 0)
    {
        return;
    }

    nav->ax_n = 0.0f;
    nav->ay_n = 0.0f;
    nav->az_n = 0.0f;
}

void nav_compute(const ImuSample_t *imu, const Orientation_t *ori, NavAccel_t *nav)
{
    (void)ori; /* Placeholder: orientation not used yet */

    if ((imu == 0) || (nav == 0))
    {
        return;
    }

    /* Placeholder navigation-frame acceleration:
       for now just copy body accel and subtract gravity on z.
       This is NOT the final frame transformation. */
    nav->ax_n = imu->ax;
    nav->ay_n = imu->ay;
    nav->az_n = imu->az - 9.81f;
}


