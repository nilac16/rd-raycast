#include <stdio.h>
#include <stdint.h>
#include <stb/stb_image_write.h>
#include "raycast.h"


/** @brief Compute the displacement vectors from the dose centroid to the edges
 *      of the matrix
 *  @param dose
 *      Dose
 *  @param[out] l
 *      Displacement to the zero corner of the dose
 *  @param[out] r
 *      Displacement to the endpoint corner
 */
static void centr_dist(const struct rc_dose *dose,
                       vec_t                *l,
                       vec_t                *r)
{
    vec_t v;

    v = rc_set(0.0, 0.0, 0.0, 1.0);
    v = rc_mvmul4(dose->mat, v);
    *l = rc_sub(v, dose->centr);
    v = rc_set((double)(dose->dim[0] - 1),
               (double)(dose->dim[1] - 1),
               (double)(dose->dim[2] - 1),
               1.0);
    v = rc_mvmul4(dose->mat, v);
    *r = rc_sub(v, dose->centr);
}

#define cot(theta) (1.0 / tan(theta))


static void raycast_dir(const struct rc_dose   *dose,
                        struct rc_target       *target,
                        const struct rc_screen *screen,
                        struct rc_colormap     *cmap,
                        int                     hdim,   /* Horizontal dimension */
                        vec_t                   dir)    /* Axial dimension */
{
    const double pi = RC_PI, cvt = pi / 180.0;
    RC_ALIGN scal_t spill1[4], spill2[4];
    struct rc_cam cam;
    scal_t w, dist;
    vec_t l, r;

    rc_cam_default(&cam);
    centr_dist(dose, &l, &r);
    rc_spill(spill1, l);
    rc_spill(spill2, r);
    w = fmax(fabs(spill1[hdim]), fabs(spill2[hdim]));
    printf("Computed horizontal distance in dimension %d: %g\n", hdim, w);
    dist = w * cot(screen->fov * cvt / 2.0);
    cam.org = rc_fmadd(dir, rc_set1(-dist), dose->centr);
    //rc_cam_lookat(&cam, dose->centr);
    rc_cam_lookalong(&cam, dir);
    rc_raycast_dose(dose, target, cmap, &cam);
}


static void raycast_xyz(const struct rc_dose   *dose,
                        struct rc_target       *target,
                        const struct rc_screen *screen,
                        struct rc_colormap     *cmap)
{
    const double root3 = 1.7320508075688772, r3recip = 1 / root3;
    const double pi = RC_PI, cvt = pi / 180.0;
    RC_ALIGN scal_t spill1[4], spill2[4];
    struct rc_cam cam;
    scal_t w, dist;
    vec_t l, r;

    rc_cam_default(&cam);
    centr_dist(dose, &l, &r);
    rc_spill(spill1, l);
    rc_spill(spill2, r);
    w = fmax(fabs(spill1[1]), fabs(spill2[1]));
    dist = w * cot(screen->fov * cvt / 2.0);
    cam.org = rc_fmadd(rc_set(r3recip, r3recip , r3recip, 0.0),
                       rc_set1(-dist),
                       dose->centr);
    rc_cam_lookat(&cam, dose->centr);
    rc_raycast_dose(dose, target, cmap, &cam);
}


static int write_texture(const char *path, const struct rc_texture *tex)
{
    const int quality = 90;
    int res;

    res = stbi_write_jpg(path,
                         tex->dim[0],
                         tex->dim[1],
                         tex->stride,
                         tex->pixels,
                         quality);
    return res < 1; /* ??? */
}


/** @brief Create the maximum-intensity projections along each principal axis,
 *      and an additional pointing along the uniform linear combination of all
 *      three
 *  @param dose
 *      Dose
 *  @returns Nonzero on error
 */
static int create_images(const struct rc_dose *dose)
{
    const unsigned size = 1024;
    const double fov = 65.0;
    struct rc_target target = {
        .tex = {
            .dim    = { size, size },
            .stride = 4
        }
    };
    struct rc_screen screen = {
        .dim = { size, size },
        .fov = fov
    };
    struct dose_cmap cmap;
    size_t tsize;
    vec_t axis;

    tsize = target.tex.stride * target.tex.dim[0] * target.tex.dim[1];
    target.tex.pixels = malloc(tsize);
    if (!target.tex.pixels) {
        perror("Cannot allocate target texture");
        return -1;
    }
    rc_target_update(&target, &screen);
    dose_cmap_init(&cmap, dose->dmax);

    axis = rc_set(1.0, 0.0, 0.0, 0.0);
    raycast_dir(dose, &target, &screen, &cmap.base, 1, axis);
    write_texture("xcast.jpg", &target.tex);

    axis = rc_set(0.0, 1.0, 0.0, 0.0);
    raycast_dir(dose, &target, &screen, &cmap.base, 0, axis);
    write_texture("ycast.jpg", &target.tex);

    axis = rc_set(0.0, 0.0, 1.0, 0.0);
    raycast_dir(dose, &target, &screen, &cmap.base, 0, axis);
    write_texture("zcast.jpg", &target.tex);

    raycast_xyz(dose, &target, &screen, &cmap.base);
    write_texture("xyzcast.jpg", &target.tex);

    return 0;
}


int main(int argc, char *argv[])
{
    const double ldthresh = 0.01;
    struct rc_dose dose;
    const char *path = argv[1];

    if (argc < 2) {
        fputs("Supply an input file\n", stderr);
        return -1;
    }
    if (rc_dose_load(&dose, path)) {
        fprintf(stderr, "Could not load the DICOM file at %s\n", path);
        return -1;
    }
    if (rc_dose_compact(&dose, ldthresh)) {
        perror("Cannot compact the dose");
    }
    create_images(&dose);
    rc_dose_clear(&dose);
    return 0;
}
