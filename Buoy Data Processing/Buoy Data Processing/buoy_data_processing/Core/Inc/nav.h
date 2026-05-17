/*
 * nav.h
 *
 *  Created on: May 6, 2026
 *      Author: jamie
 */

#ifndef NAV_H
#define NAV_H

#include "imu.h"
#include "fusion.h"

typedef struct
{
    float ax_n;
    float ay_n;
    float az_n;
} NavAccel_t;

void nav_init(NavAccel_t *nav);
void nav_compute(const ImuSample_t *imu, const Orientation_t *ori, NavAccel_t *nav);

#endif
