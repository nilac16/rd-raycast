#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#if !defined(_WIN32) || !_WIN32
#   include "app.h"
#else
#   include "win32/app.h"
#endif


static void main_print_usage(void)
{
    static const char *usage =
"Usage: dcast [OPTION] PATH\n"
"Open an interactive raycast window displaying a maximum-intensity perspective\n"
"projection of DICOM RTDose file at PATH\n";

    puts(usage);
    puts(rc_get_usage_opt());
}


int main(int argc, char *argv[])
{
    struct rc_app app = { 0 };
    struct rc_app_params params = {
        .title  = "Raycast window",
        .width  = 512,
        .height = 288,
        .path   = NULL,
        .mu_k   = (scal_t)1.0,
        .slow   = (scal_t)0.1,
        .speed  = (scal_t)0.5,
        .turbo  = (scal_t)1.0
    };
    int res;

    if (rc_parse_opt(argc, argv, &params)) {
        main_print_usage();
        return 1;
    }
    if (!params.path) {
        fputs("Missing input file\n", stderr);
        main_print_usage();
        return 1;
    }
    res = rc_app_open(&app, &params);
    if (!res) {
        res = rc_app_run(&app);
    }
    rc_app_close(&app);
    return res;
}
