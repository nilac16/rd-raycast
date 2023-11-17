#pragma once

#ifndef RC_APP_H
#define RC_APP_H

#define UNICODE 1
#define _UNICODE 1
#include <Windows.h>

#include <stdbool.h>

#include "params.h"
#include "raycast.h"

#pragma warning(disable: 4244)


struct rc_win32_cmap {
    struct dose_cmap base;
};


struct rc_app {
    HINSTANCE hinst;
    HWND      hwnd;

    BITMAPINFO bmpinfo; /* Target bitmap information for "rapid" blitting */
    
    bool dirty;
    bool shouldquit;
    bool autotarget;
    bool linear;

    struct rc_target target;
    struct rc_cam    camera;
    struct rc_screen screen;
    struct rc_dose   dose;
    struct rc_win32_cmap cmap;

    POINT lastpos;  /* Last cursor position */
    POINT mupdate;  /* Accumulated mouse deflections */

    LARGE_INTEGER lasttik;
    scal_t        tikmult;      /* Multiply a perf count by this to get ms */

    vec_t yaw;
    vec_t pitch;

    scal_t mu_k;
    scal_t speed;
    scal_t slow;
    scal_t turbo;
};


/** @brief Launch the application
 *  @param app
 *      Application state buffer
 *  @param params
 *      Application startup parameters
 *  @returns Nonzero on error
 */
int rc_app_open(struct rc_app *app, const struct rc_app_params *params);


/** @brief Run the application
 *  @param app
 *      Application state buffer
 *  @returns The application exit code
 */
int rc_app_run(struct rc_app *app);


/** @brief Close the application. If @p app was zero-initialized, this is always
 *      safe to call
 *  @param app
 *      Application state buffer
 */
void rc_app_close(struct rc_app *app);


#endif /* RC_APP_H */
