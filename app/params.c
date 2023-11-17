#include <stdio.h>
#include "params.h"
#define OPT_IMPLEMENTATION 1
#include <opt.h>


static int rc_opt_callback(int idx, unsigned count, char *args[], void *data);


static const struct optspec opts[] = {
    { 'l', "linear", 0, rc_opt_callback },
    { 'w', "width",  1, rc_opt_callback },
    { 'h', "height", 1, rc_opt_callback },
    { 0,   "res",    2, rc_opt_callback },
    { 0,   "dim",    2, rc_opt_callback }
};

enum {
    RC_OPT_LINEAR,
    RC_OPT_WIDTH,
    RC_OPT_HEIGHT,
    RC_OPT_RES,
    RC_OPT_DIM
};


const char *rc_get_usage_opt(void)
{
    static const char *usage =
"  -l, --linear         Use linear interpolation instead of nearest\n"
"  -w, --width  X       Set the target size to X pixels in width\n"
"  -h, --height Y       Set the target size to Y pixels in height\n"
"      --res    X Y     Set the target size to X pixels in width and Y pixels in\n"
"      --dim    X Y     height\n";

    return usage;
}


static int rc_opt_callback(int idx, unsigned count, char *args[], void *data)
{
    struct rc_app_params *p = data;
    int diff = opts[idx].args - (int)count;

    if (diff > 0) {
        fprintf(stderr,
                "Option %s is missing %d required arguments\n",
                opts[idx].lng,
                diff);
        return 1;
    }
    switch (idx) {
    case RC_OPT_LINEAR:
        p->linear = 1;
        break;
    case RC_OPT_RES:
    case RC_OPT_DIM:
        p->height = atoi(args[1]);
        /* FALL THRU */
    case RC_OPT_WIDTH:
        p->width = atoi(args[0]);
        break;
    case RC_OPT_HEIGHT:
        p->height = atoi(args[0]);
        break;
    }
    return 0;
}


static int rc_opt_errorfn(int type, char shrt, char *lng, void *data)
{
    char sbuf[2] = { shrt, 0 };

    (void)data;
    fprintf(stderr,
            "Unrecognized %s option \"%s\"\n",
            type ? "long" : "short",
            type ? lng : sbuf);
    return 0;
}


static int rc_opt_posfn(int idx, unsigned count, char *args[], void *data)
{
    struct rc_app_params *p = data;

    (void)idx;
    if (count) {
        p->path = args[0];
    }
    return 0;
}


int rc_parse_opt(int argc, char *argv[], struct rc_app_params *p)
{
    struct optinfo info = {
        .argc   = argc,
        .argv   = argv,
        .fstact = OPT_FIRST_SKIP,
        .endact = OPT_END_ALLOW,
        .errcb  = rc_opt_errorfn,
        .poscb  = rc_opt_posfn,
        .data   = p
    };

    return opt_parse(&info, sizeof opts / sizeof *opts, opts);
}
