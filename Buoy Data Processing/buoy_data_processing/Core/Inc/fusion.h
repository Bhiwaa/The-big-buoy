/*
 * fusion.h
 *
 *  Created on: May 6, 2026
 *      Author: jamie
 */

#ifndef FUSION_H
#define FUSION_H

#include "imu.h"

typedef struct
{
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
} Orientation_t;

void fusion_init(Orientation_t *ori);
void fusion_update(const ImuSample_t *imu, float dt_s, Orientation_t *ori);

#endif
