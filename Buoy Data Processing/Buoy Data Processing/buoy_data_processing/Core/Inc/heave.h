/*
 * heave.h
 *
 *  Created on: May 6, 2026
 *      Author: jamie
 */

#ifndef HEAVE_H
#define HEAVE_H

#include "nav.h"

typedef struct
{
    float velocity_mps;
    float heave_m;
} HeaveState_t;

void heave_init(HeaveState_t *state);
void heave_update(const NavAccel_t *nav, float dt_s, HeaveState_t *state);

#endif
