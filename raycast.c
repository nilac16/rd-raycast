#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include "raycast.h"


void rc_cam_default(struct rc_cam *cam)
{
    struct rc_cam def = {
        .quat = rc_set(0.0, 0.0, 0.0, 1.0),
        .org  = rc_set(0.0, 0.0, 0.0, 1.0),
        .vel  = rc_zero()
    };

    *cam = def;
}


void rc_cam_normalize(struct rc_cam *cam)
{
    cam->quat = rc_vnorm(cam->quat);
}


/** @brief Eliminate the z-component of @p v and normalize it
 *  @param v
 *      Vector
 *  @returns The components of @p v in the xy-plane
 */
static vec_t xy_project(vec_t v)
{
    RC_ALIGN scal_t spill[4];

    rc_spill(spill, v);
    spill[2] = 0.0;
    return rc_load(spill);
}


void rc_cam_lookalong(struct rc_cam *cam, vec_t dir)
{
    vec_t z, z_xy, dir_xy, yaw, pitch;

    z = rc_set(0.0, 0.0, 1.0, 0.0);
    z_xy = xy_project(rc_qrot(cam->quat, z));
    dir_xy = xy_project(dir);
    yaw = rc_qalign(z_xy, dir_xy);
    yaw = isnan(rc_cvtsf(yaw)) ? rc_set(0.0, 0.0, 0.0, 1.0) : yaw;
    rc_cam_comp_left(cam, yaw);
    z = rc_qrot(cam->quat, z);
    pitch = rc_qalign(z, dir);
    pitch = isnan(rc_cvtsf(pitch)) ? rc_set(0.0, 0.0, 0.0, 1.0) : pitch;
    rc_cam_comp_left(cam, pitch);
}


void rc_cam_lookat(struct rc_cam *cam, vec_t pos)
{
    vec_t diff;

    diff = rc_sub(pos, cam->org);
    rc_cam_lookalong(cam, diff);
}


bool rc_cam_update(struct rc_cam *cam, vec_t accel, scal_t tau)
{
    const scal_t threshold = (scal_t)1e-3;
    const vec_t og = cam->org;
    vec_t tstep;

    tstep = rc_set1(tau);
    cam->vel = rc_fmadd(accel, tstep, cam->vel);
    cam->org = rc_fmadd(cam->vel, tstep, cam->org);
    return isgreater(rc_cvtsf(rc_vsqrnorm(rc_sub(cam->org, og))), threshold);
}


void rc_target_update(struct rc_target *target, const struct rc_screen *screen)
{
    const double aspect = (double)screen->dim[1] / (double)screen->dim[0];
    const double pi = RC_PI, conv = pi / 360;
    RC_ALIGN scal_t spill[4] = { 0 };

    spill[0] = (scal_t)(2.0 * tan(conv * screen->fov));
    spill[1] = (scal_t)aspect * spill[0];
    target->size = rc_load(spill);

    spill[0] = (scal_t)target->tex.dim[0];
    spill[1] = (scal_t)target->tex.dim[1];
    spill[2] = (scal_t)1.0;
    spill[3] = (scal_t)1.0;
    target->res = rc_div(target->size, rc_load(spill));
}


/** @brief Hit test the first two components of @p test with zero and the first
 *      two components of @p endp
 *  @param test
 *      Test point. If either of the first two components are a NaN, this
 *      returns the expected value of false
 *  @param endp
 *      Corner point
 *  @returns true if @p test is within the rectangle with point at zero and
 *      endpoint at @p endp
 */
static bool rc_hit_test(vec_t test, vec_t endp)
{
    vec_t cmp;

    cmp = rc_cmp(test, rc_zero(), _CMP_LT_OQ);
    cmp = rc_or(cmp, rc_cmp(test, endp, _CMP_GE_OQ));
    cmp = rc_permute(cmp, _MM_SHUFFLE(1, 0, 1, 0));
    return rc_testz(cmp, cmp);
}


/** @brief Compute intersection parameters for a line passing through point @p p
 *      and with tangent vector @p t
 *  @param dose
 *      Dose volume to intersect
 *  @param p
 *      "Starting" point on the line
 *  @param t
 *      Tangent vector
 *  @param isect
 *      Intersection parameters will be placed here. There must be at least two
 *  @returns The number of intersections, either two of zero
 */
static int rc_raycast_intersect(const struct rc_dose *dose,
                                vec_t                 p,
                                vec_t                 t,
                                vec_t                 isect[])
/** Holy MOLY this fucker is a MONSTER
 * 
 *  If only shuffle masks didn't have to be compile constants
 * 
 *  Thankfully GCC doesn't emit any instructions for permutes/shuffles with
 *  _MM_SHUFFLE(3, 2, 1, 0) so I will leave them in for code symmetry
 */
{
    vec_t param[2], dim, tau, loc, endp;
    int res = 0;

    dim = rc_set((scal_t)dose->dim[0],
                 (scal_t)dose->dim[1],
                 (scal_t)dose->dim[2],
                 (scal_t)1.0);
    param[0] = rc_div(rc_sub(rc_zero(), p), t);
    param[1] = rc_div(rc_sub(dim, p), t);
    {   /* x-plane testing */
        endp = rc_permute(dim, _MM_SHUFFLE(3, 0, 2, 1));
        tau = rc_permute(param[0], _MM_SHUFFLE(0, 0, 0, 0));
        loc = rc_fmadd(t, tau, p);
        if (rc_hit_test(rc_permute(loc, _MM_SHUFFLE(3, 0, 2, 1)), endp)) {
            isect[res++] = tau;
        }

        tau = rc_permute(param[1], _MM_SHUFFLE(0, 0, 0, 0));
        loc = rc_fmadd(t, tau, p);
        if (rc_hit_test(rc_permute(loc, _MM_SHUFFLE(3, 0, 2, 1)), endp)) {
            isect[res++] = tau;
        }
    }
    {   /* y-plane testing */
        endp = rc_permute(dim, _MM_SHUFFLE(3, 1, 2, 0));
        tau = rc_permute(param[0], _MM_SHUFFLE(1, 1, 1, 1));
        loc = rc_fmadd(t, tau, p);
        if (rc_hit_test(rc_permute(loc, _MM_SHUFFLE(3, 1, 2, 0)), endp)) {
            isect[res++] = tau;
        }

        tau = rc_permute(param[1], _MM_SHUFFLE(1, 1, 1, 1));
        loc = rc_fmadd(t, tau, p);
        if (rc_hit_test(rc_permute(loc, _MM_SHUFFLE(3, 1, 2, 0)), endp)) {
            isect[res++] = tau;
        }
    }
    {   /* z-plane testing */
        endp = rc_permute(dim, _MM_SHUFFLE(3, 2, 1, 0));
        tau = rc_permute(param[0], _MM_SHUFFLE(2, 2, 2, 2));
        loc = rc_fmadd(t, tau, p);
        if (rc_hit_test(rc_permute(loc, _MM_SHUFFLE(3, 2, 1, 0)), endp)) {
            isect[res++] = tau;
        }

        tau = rc_permute(param[1], _MM_SHUFFLE(2, 2, 2, 2));
        loc = rc_fmadd(t, tau, p);
        if (rc_hit_test(rc_permute(loc, _MM_SHUFFLE(3, 2, 1, 0)), endp)) {
            isect[res++] = tau;
        }
    }
    return res;
}


/** @brief Compute the pixel dose for a ray given by coordinate @p pos and
 *      tangent vector @p tangent
 *  @param dose
 *      Dose to raycast
 *  @param pos
 *      Ambient position of the point on the line
 *  @param tangent
 *      Tangent vector in the ambient space. This does not need to be normalized
 *      (and should not, because it will be in this function)
 *  @returns The dose picked out for this ray
 */
static double rc_raycast_compute(const struct rc_dose *dose,
                                 vec_t                 pos,
                                 vec_t                 tangent)
{
    double res = 0.0, next;
    int count, tau, end;
    vec_t params[6];

    pos = rc_mvmul4(dose->inv, pos);
    tangent = rc_mvmul3(dose->inv, tangent);
    tangent = rc_vnorm(tangent);
    count = rc_raycast_intersect(dose, pos, tangent, params);
    if (count > 1) {
        params[2] = rc_ceil(rc_max(rc_min(params[0], params[1]), rc_zero()));
        params[3] = rc_floor(rc_max(params[0], params[1]));
        pos = rc_fmadd(params[2], tangent, pos);
        tau = (int)rc_cvtsf(params[2]);
        end = (int)rc_cvtsf(params[3]);
        for (; tau < end; tau += 1) {
            next = rc_dose_nearest(dose, pos);
            res = rc_fmax(next, res);
            pos = rc_add(pos, tangent);
        }
    }
    return res;
}


/** A tangent basis for the image plane */
struct rc_basis {
    vec_t x;    /* The horizontal tangent basis vector */
    vec_t y;    /* The vertical tangent basis vector */
    vec_t org;  /* The physical coordinates of the top left corner */
};


/** @brief Compute the basis vectors for the target plane. Only the first two
 *      are returned; the third component is the origin. All outputs are in
 *      scene coordinates
 *  @param[out] basis
 *      Destination buffer
 *  @param camera
 *      Camera containing orientation quaternion
 *  @param target
 *      Target context
 */
static void rc_raycast_basis(struct rc_basis        *basis,
                             const struct rc_target *target,
                             const struct rc_cam    *camera)
{
    vec_t offs, resx, resy, scalx, scaly, two;

    two = rc_set1(2.0);
    basis->x = rc_qrot(camera->quat, rc_set(1.0, 0.0, 0.0, 0.0));
    basis->y = rc_qrot(camera->quat, rc_set(0.0, 1.0, 0.0, 0.0));
    offs = rc_qrot(camera->quat, rc_set(0.0, 0.0, 1.0, 0.0));
    resx = rc_permute(target->res, _MM_SHUFFLE(0, 0, 0, 0));
    resy = rc_permute(target->res, _MM_SHUFFLE(1, 1, 1, 1));
    scalx = rc_permute(target->size, _MM_SHUFFLE(0, 0, 0, 0));
    scaly = rc_permute(target->size, _MM_SHUFFLE(1, 1, 1, 1));
    scalx = rc_div(rc_sub(resx, scalx), two);
    scaly = rc_div(rc_sub(resy, scaly), two);
    offs = rc_fmadd(basis->x, scalx, offs);
    offs = rc_fmadd(basis->y, scaly, offs);
    basis->org = rc_add(camera->org, offs);
    basis->x = rc_mul(basis->x, resx);
    basis->y = rc_mul(basis->y, resy);
}


/** @brief No dose in sight, just rapidly colormap zero to @p target
 *  @param target
 *      Target texture
 *  @param cmap
 *      Colormap
 */
static void rc_raycast_empty(struct rc_target *target, struct rc_colormap *cmap)
{
    const unsigned flen = target->tex.dim[0] * target->tex.stride;
    unsigned char *scan, *pixel;
    unsigned i;
    int j, jend = target->tex.dim[1];

#if _OPENMP
#   pragma omp parallel for private(i, scan, pixel)
#endif /* _OPENMP */
    for (j = 0; j < jend; j++) {
        scan = (unsigned char *)target->tex.pixels + flen;
        for (i = 0; i < target->tex.dim[0]; i++) {
            pixel = scan + i * target->tex.stride;
            cmap->func(cmap, 0.0, pixel);
        }
    }
}


void rc_raycast_dose(const struct rc_dose *dose,
                     struct rc_target     *target,
                     struct rc_colormap   *cmap,
                     const struct rc_cam  *camera)
{
    vec_t scanpos, pxpos, tangent;
    struct rc_basis basis;
    unsigned i, offs;
    int j, jend = (int)target->tex.dim[1];
    double res;
    char *ptr;

    if (!dose->data) {
        rc_raycast_empty(target, cmap);
        return;
    }
    rc_raycast_basis(&basis, target, camera);

#if _OPENMP
#   pragma omp parallel for private(i, ptr, offs, scanpos, pxpos, tangent, res)
#endif /* _OPENMP */
    for (j = 0; j < jend; j++) {
        scanpos = rc_fmadd(basis.y, rc_set1((scal_t)j), basis.org);
        offs = target->tex.stride * target->tex.dim[0] * j;
        ptr = (char *)target->tex.pixels + offs;
        for (i = 0; i < target->tex.dim[0]; i++) {
            pxpos = rc_fmadd(basis.x, rc_set1((scal_t)i), scanpos);
            tangent = rc_sub(pxpos, camera->org);
            res = rc_raycast_compute(dose, pxpos, tangent);
            cmap->func(cmap, res, ptr);
            ptr += target->tex.stride;
        }
    }
}
