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

    int linear; /* If nonzero, use linear interpolation instead */
};


/** @brief Parse cmd args
 *  @param argc
 *      Argument count
 *  @param argv
 *      Argument vector
 *  @param p
 *      Application parameters
 *  @returns Nonzero on error
 */
int rc_parse_opt(int argc, char *argv[], struct rc_app_params *p);


/** @brief Get the options table shown in the usage string */
const char *rc_get_usage_opt(void);


#endif /* RC_APP_PARAMS_H */
