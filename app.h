#pragma once

#ifndef RC_APP_H
#define RC_APP_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "raycast.h"


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


struct rc_app {
    SDL_Window   *wnd;
    SDL_Renderer *rend;
    SDL_Texture  *tex;

    bool dirty;         /* If true, redraw */
    bool shouldquit;    /* If true, exit main loop */
    bool fullscreen;    /* Set to true when the app is fullscreen */
    bool autotarget;    /* Override camera look on move */

    struct rc_target target;
    struct rc_cam    camera;
    struct rc_screen screen;
    struct dose_cmap cmap;
    struct rc_dose   dose;

    /* Rotation quaternions for moving the camera */
    vec_t pitch;    /* Pitch the camera up and down about ITS x-axis */
    vec_t yaw;      /* Yaw the camera left and right about SCENE z-axis */

    int          nkeys;       /* The size of keystate */
    const Uint8 *keystate;    /* SDL's keystate buffer */

    Uint32 evwake;      /* Awaken event number */

    Uint32 lasttik;     /* The last update tick */
    scal_t spdmult;     /* Acceleration multiplier */
    scal_t slowmult;    /* Accel multiplier while slow (walk) key is held */
    scal_t turbomult;   /* Accel multiplier while turbo key is held */
    scal_t mu_k;        /* Kinetic camera friction coefficient */
};


/** @brief Start the app
 *  @param app
 *      Application state buffer
 *  @param params
 *      Initial app options
 *  @returns Nonzero on error
 */
int rc_app_open(struct rc_app *app, const struct rc_app_params *params);


/** @brief Run the app
 *  @param app
 *      Application state buffer
 *  @returns The application exit code
 */
int rc_app_run(struct rc_app *app);


/** @brief Terminate and free all resources. Outside the message loop, this is
 *      always safe to call
 *  @param app
 *      Application state buffer
 */
void rc_app_close(struct rc_app *app);


#endif /* RC_APP_H */
