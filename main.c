#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "app.h"


int main(int argc, char *argv[])
{
    struct rc_app app = { 0 };
    struct rc_app_params params = {
        .title  = "Raycast window",
        .width  = 512,
        .height = 288,
        .path   = argv[1],
        .mu_k   = 1.0,
        .slow   = 0.01,
        .speed  = 0.5,
        .turbo  = 1.0
    };
    int res;

    (void)argc;
    res = rc_app_open(&app, &params);
    if (!res) {
        res = rc_app_run(&app);
    }
    rc_app_close(&app);
    return res;
}
