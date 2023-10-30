#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if !defined(_WIN32) || !_WIN32
#   include "app.h"
#else
#   include "win32/app.h"
#endif


int main(int argc, char *argv[])
{
    struct rc_app app = { 0 };
    struct rc_app_params params = {
        .title  = "Raycast window",
        .width  = 512,
        .height = 288,
        .path   = argc > 1 ? argv[1] : "",
        .mu_k   = (scal_t)1.0,
        .slow   = (scal_t)0.1,
        .speed  = (scal_t)0.5,
        .turbo  = (scal_t)1.0
    };
    int res;

    res = rc_app_open(&app, &params);
    if (!res) {
        res = rc_app_run(&app);
    }
    rc_app_close(&app);
    return res;
}
