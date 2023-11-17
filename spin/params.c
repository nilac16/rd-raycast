#include <stdio.h>
#include "params.h"
#include "rcmath.h"
#define OPT_IMPLEMENTATION 1
#include <opt.h>



static int main_optcb(int idx, unsigned count, char *args[], void *data);

static struct optspec opts[] = {
    { .shrt = 'a', .lng = "angle",   .args = 1, .func = main_optcb },
    { .shrt = 'd', .lng = "dist",    .args = 1, .func = main_optcb },
    { .shrt = 'f', .lng = "fov",     .args = 1, .func = main_optcb },
    { .shrt = 'q', .lng = "quality", .args = 1, .func = main_optcb },
    { .shrt = 'o', .lng = "offset",  .args = 3, .func = main_optcb },
    { .shrt = 'n', .lng = "frames",  .args = 1, .func = main_optcb },
    { .shrt = 't', .lng = "time",    .args = 1, .func = main_optcb },
    { .shrt = 'l', .lng = "linear",  .args = 0, .func = main_optcb },
    { .shrt = 'w', .lng = "width",   .args = 1, .func = main_optcb },
    { .shrt = 'h', .lng = "height",  .args = 1, .func = main_optcb },
    { .shrt = 0,   .lng = "dim",     .args = 2, .func = main_optcb },
    { .shrt = 0,   .lng = "res",     .args = 2, .func = main_optcb },
};

enum {
    OPT_ANGLE,
    OPT_DIST,
    OPT_FOV,
    OPT_QUALITY,
    OPT_OFFSET,
    OPT_FRAMES,
    OPT_FTIME,
    OPT_LINEAR,
    OPT_WIDTH,
    OPT_HEIGHT,
    OPT_DIM,
    OPT_RES
};


const char *spin_get_usage_opt(void)
/** I wonder how simple this would be to automate. Might be doable with macros
 */
{
    static const char *options =
"  -a, --angle   DEGREES    Set the viewing latitude angle to DEGREES\n"
"  -d, --dist    DIST       Set the viewing distance to DIST\n"
"  -f, --fov     DEGREES    Set the field of view to DEGREES\n"
"  -q, --quality PERCENT    Set the frame compression quality to PERCENT\n"
"  -o, --offset  X Y Z      Offset the camera target by X Y Z\n"
"  -n, --frames  COUNT      Set the number of frames to COUNT\n"
"  -t, --time    MS         Set the frame time to MS\n"
"  -l, --linear             Use linear interpolation instead of nearest\n"
"  -w, --width   X          Set the image width to X pixels\n"
"  -h, --height  Y          Set the image height to Y pixels\n"
"      --res     X Y        Set the image dimensions to X horizontal pixels and Y\n"
"      --dim     X Y        vertical pixels\n";

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
        return 1;
    }
    switch (idx) {
    case OPT_ANGLE:
        p->lat = atof(args[0]);
        break;
    case OPT_DIST:
        p->dist = atof(args[0]);
        break;
    case OPT_FOV:
        p->fov = atof(args[0]);
        break;
    case OPT_QUALITY:
        p->quality = atof(args[0]);
        p->quality = rc_fclamp(p->quality, 0.0f, 100.0f);
        break;
    case OPT_OFFSET:
        p->offset = rc_set(atof(args[0]), atof(args[1]), atof(args[2]), 0.0);
        break;
    case OPT_FRAMES:
        p->fcnt = atoi(args[0]);
        break;
    case OPT_FTIME:
        p->ftime = atoi(args[0]);
        break;
    case OPT_LINEAR:
        p->linear = 1;
        break;
    case OPT_DIM:
    case OPT_RES:
        p->height = atoi(args[1]);
        /* FALL THRU */
    case OPT_WIDTH:
        p->width = atoi(args[0]);
        break;
    case OPT_HEIGHT:
        p->height = atoi(args[0]);
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
