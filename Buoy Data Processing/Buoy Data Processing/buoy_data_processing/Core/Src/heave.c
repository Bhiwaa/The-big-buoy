/*
 * heave.c
 *
 *  Created on: May 6, 2026
 *      Author: jamie
 */

#include "heave.h"

void heave_init(HeaveState_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->velocity_mps = 0.0f;
    state->heave_m = 0.0f;
}

void heave_update(const NavAccel_t *nav, float dt_s, HeaveState_t *state)
{
    const float leak_v = 0.995f;
    const float leak_h = 0.995f;

    if ((nav == 0) || (state == 0))
    {
        return;
    }

    /* Placeholder leaky double integration */
    state->velocity_mps = leak_v * (state->velocity_mps + nav->az_n * dt_s);
    state->heave_m      = leak_h * (state->heave_m + state->velocity_mps * dt_s);
}


