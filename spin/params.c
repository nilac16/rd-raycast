#include <stdio.h>
#include "params.h"
#include "rcmath.h"
#define OPT_IMPLEMENTATION 1
#include "opt.h"



static int main_optcb(int idx, unsigned count, char *args[], void *data);

static struct optspec opts[] = {
    { 'a', "angle",  1, main_optcb },
    { 'd', "dist",   1, main_optcb },
    { 'f', "fov",    1, main_optcb },
    { 'n', "frames", 1, main_optcb },
    { 't', "time",   1, main_optcb },
    { 'w', "width",  1, main_optcb },
    { 'h', "height", 1, main_optcb },
    { 0,   "dim",    1, main_optcb },
    { 0,   "res",    1, main_optcb },
};

enum {
    OPT_ANGLE,
    OPT_DIST,
    OPT_FOV,
    OPT_FRAMES,
    OPT_FTIME,
    OPT_WIDTH,
    OPT_HEIGHT,
    OPT_DIM,
    OPT_RES
};


const char *spin_get_usage_opt(void)
/** I wonder how simple this would be to automate. It would have to obey the 80-
 *  column limit
 */
{
    static const char *options =
"  -a, --angle  DEGREES Set the viewing colatitude angle to DEGREES\n"
"  -d, --dist   DIST    Set the viewing distance to DIST\n"
"  -f, --fov    DEGREES Set the field of view to DEGREES\n"
"  -n, --frames COUNT   Set the number of frames to COUNT\n"
"  -t, --time   MS      Set the frame time to MS\n"
"  -w, --width  COUNT   Set the image width to COUNT pixels\n"
"  -h, --height COUNT   Set the image height to COUNT pixels\n"
"      --res    X Y     Set the image dimension to X horizontal pixels and Y\n"
"      --dim    X Y     vertical pixels\n";

    return options;
}


/** @brief Master callback for all options */
static int main_optcb(int idx, unsigned count, char *args[], void *data)
{
    struct params *p = data;
    int diff = opts[idx].args - (int)count;

    if (diff > 0) {
        fprintf(stderr,
                "Warning: Option \"%s\" is missing %d required argument%s\n",
                opts[idx].lng, diff, diff == 1 ? "" : "s");
        return 0;   /* let it keep going? */
    }
    switch (idx) {
    case OPT_ANGLE:
        p->colat = atof(args[0]);
        break;
    case OPT_DIST:
        p->dist = atof(args[0]);
        break;
    case OPT_FOV:
        p->fov = atof(args[0]);
        break;
    case OPT_FRAMES:
        p->fcnt = atoi(args[0]);
        break;
    case OPT_FTIME:
        p->ftime = atoi(args[0]);
        break;
    case OPT_WIDTH:
        p->width = atoi(args[0]);
        break;
    case OPT_HEIGHT:
        p->height = atoi(args[0]);
        break;
    case OPT_DIM:
    case OPT_RES:
        p->width = atoi(args[0]);
        p->height = atoi(args[1]);
        break;
    default:
        break;
    }
    return 0;
}


/** @brief Callback for unrecognized options */
static int main_opterr(int type, char shrt, char *lng, void *data)
{
    char sbuf[2] = { shrt, '\0' };
    const char *spec = type ? "long" : "short";
    char *name = type ? lng : sbuf;

    (void)data;

    fprintf(stderr, "Unrecognized %s option \"%s\"\n", spec, name);
    return 0;
}


/** @brief Callback for positional cmdargs */
static int main_optpos(int idx, unsigned count, char *args[], void *data)
{
    struct params *p = data;

    (void)idx;
    (void)data;

    if (count > 0) {
        p->file = args[0];
    }
    if (count > 1) {
        p->output = args[1];
    }
    return 0;
}


int spin_parse_opt(int argc, char *argv[], struct params *res)
{
    struct optinfo info = {
        .argc   = argc,
        .argv   = argv,
        .fstact = OPT_FIRST_SKIP,
        .endact = OPT_END_ALLOW,
        .errcb  = main_opterr,
        .poscb  = main_optpos,
        .data   = res
    };

    return opt_parse(&info, sizeof opts / sizeof *opts, opts);
}
