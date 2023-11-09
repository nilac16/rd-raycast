#include <alloca.h>
#include <stdio.h>
#include <string.h>
#include <webp/encode.h>
#include "params.h"
#include "anim.h"
#include "raycast.h"


/** hackin again */
extern void dose_cmapfn(struct rc_colormap *this, double dose, void *pixel);


struct scene {
    struct rc_dose   dose;
    struct dose_cmap cmap;
    struct rc_screen screen;
    struct rc_target target;
    struct rc_cam    camera;
};


/** Get ready */
static int main_prepare_scene(struct scene *sc, const struct params *p)
{
    const scal_t angle = RC_PI / 4.0;
    const int stride = 4;

    if (rc_dose_load(&sc->dose, p->file)) {
        return 1;
    }
    rc_dose_compact(&sc->dose, 0.05);
    dose_cmap_init(&sc->cmap, sc->dose.dmax);
    sc->screen.dim[0] = sc->target.tex.dim[0] = p->width;
    sc->screen.dim[1] = sc->target.tex.dim[1] = p->height;
    sc->screen.fov = p->fov;
    sc->target.tex.stride = stride;
    sc->target.tex.pixels = malloc((size_t)p->width * p->height * stride);
    rc_target_update(&sc->target, &sc->screen);
    rc_cam_default(&sc->camera);
    rc_cam_comp_right(&sc->camera, rc_set(sin(angle), 0.0, 0.0, cos(angle)));
    if (!sc->target.tex.pixels) {
        perror("Failed to allocate frame pixel buffer");
    }
    return sc->target.tex.pixels == NULL;
}


/** Cleanup */
static void main_clear_scene(struct scene *sc)
{
    rc_dose_clear(&sc->dose);
    free(sc->target.tex.pixels);
}


/** Iterate each angle and raycast to target */
static int main_create_frames(struct scene        *sc,
                              const struct params *p,
                              struct anim         *anim)
{
    double sect = (RC_PI * 2.0) / (double)p->fcnt;
    double phi, theta = p->colat * (RC_PI / 180.0);
    double costheta, sintheta, cosphi, sinphi;
    vec_t disp, radius, centr;
    int i;

    costheta = cos(theta);
    sintheta = sin(theta);
    radius = rc_set1((scal_t)p->dist);
    centr = rc_add(sc->dose.centr, p->offset);
    for (i = 0; i < p->fcnt; i++) {
        phi = (double)i * sect;
        cosphi = cos(phi);
        sinphi = sin(phi);
        disp = rc_set(sintheta * sinphi, -sintheta * cosphi, costheta, 0.0);
        sc->camera.org = rc_fmadd(disp, radius, centr);
        rc_cam_lookat(&sc->camera, centr);
        rc_raycast_dose(&sc->dose, &sc->target, &sc->cmap.base, &sc->camera);
        if (anim_add_frame(anim, sc->target.tex.pixels)) {
            return 1;
        }
    }
    return 0;
}


/** Create the animation */
static int main_generate_image(const struct params *p)
{
    struct scene scene = { 0 };
    struct anim anim = { 0 };
    int res;

    if (anim_init(&anim, p) || main_prepare_scene(&scene, p)) {
        return 1;
    }
    res = main_create_frames(&scene, p, &anim)
       || anim_write(&anim, p->output);
    main_clear_scene(&scene);
    anim_clear(&anim);
    return res;
}


static void main_print_usage(void)
{
    static const char *usage =
"Usage: spin [OPTION] INPUT [OUTPUT]\n"
"Create a rotating maximum-intensity projection of DICOM RTDose file INPUT. If\n"
"an OUTPUT filename is not provided, the result will be written to output.webp\n"
"in the current directory\n";

    puts(usage);
    puts("Options:");
    puts(spin_get_usage_opt());
}


int main(int argc, char *argv[])
{
    struct params params = {
        .colat   = 90,
        .dist    = 200,
        .fov     = 75,
        .quality = 50.0f,
        .offset  = rc_zero(),
        .fcnt    = 8,
        .ftime   = 1000 / params.fcnt,
        .width   = 512,
        .height  = 512,
        .file    = NULL,
        .output  = "output.webp"
    };

    if (spin_parse_opt(argc, argv, &params)) {
        return 1;
    }
    if (!params.file) {
        fputs("Missing input file\n", stderr);
        main_print_usage();
        return 1;
    }
    printf("Creating an image with the following parameters:\n"
           "  Colatitude:  %g degrees\n"
           "  Distance:    %g units\n"
           "  FOV:         %g degrees\n"
           "  Quality:     %.2f%%\n"
           "  Offset:      %g %g %g\n"
           "  Frame count: %d\n"
           "  Frame time:  %d ms\n"
           "  Width:       %d pixels\n"
           "  Height:      %d pixels\n"
           "  File:        %s\n"
           "  Output path: %s\n",
        params.colat, params.dist,
        params.fov, params.quality,
        ((scal_t *)&params.offset)[0],
        ((scal_t *)&params.offset)[1],
        ((scal_t *)&params.offset)[2],
        params.fcnt, params.ftime,
        params.width, params.height,
        params.file, params.output);
    return main_generate_image(&params);
}
