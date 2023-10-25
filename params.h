#pragma once

#ifndef RC_APP_PARAMS_H
#define RC_APP_PARAMS_H

#include "rcmath.h"


struct rc_app_params {
    const char title[50];   /* The window title */
    int        width;       /* Window width */
    int        height;      /* Window height */

    const char *path;       /* The path to the initial file */

    scal_t mu_k;
    scal_t speed;
    scal_t turbo;
    scal_t slow;
};


#endif /* RC_APP_PARAMS_H */
