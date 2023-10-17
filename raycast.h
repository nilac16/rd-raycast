#pragma once

#ifndef RAYCAST_H
#define RAYCAST_H

#include <stdbool.h>
#include "math.h"
#include "dose.h"
#include "cmap.h"


/** Context struct containing geometric information for a pinhole camera */
struct rc_cam {
    vec_t quat;     /* Orientation quaternion as a rotation of the scene axes */
    vec_t org;      /* Position of the pinhole */
    vec_t vel;      /* Tangent velocity */
};


/** @brief Produce a camera struct with origin at zero and null rotation
 *      quaternion. Do not that this is *NOT* equivalent to setting the struct
 *      to zero!
 *  @details The orientation quaternion starts at null rotation, which
 *      corresponds to a real part of one and no vector part. The position is
 *      a coordinate "vector," and must thus contain 1.0 as its projective
 *      component. Note the symmetry: These elements are members of their
 *      respective group kernels, and the representation is identical
 *  @param cam
 *      Camera to initialize
 */
void rc_cam_default(struct rc_cam *cam);


/** @brief Normalize the camera's orientation quaternion
 *  @param cam
 *      Camera
 */
void rc_cam_normalize(struct rc_cam *cam);


/** @brief Left-multiply the camera orientation by @p quat. The result is a
 *      rotation by @p quat using the SCENE coordinate system
 *  @param cam
 *      Camera
 *  @param quat
 *      Rotation quaternion to compose with @p cam
 */
static inline void rc_cam_comp_left(struct rc_cam *cam, vec_t quat)
{
    cam->quat = rc_qmul(quat, cam->quat);
}


/** @brief Right-multiply the camera orientation by @p quat. The result is a
 *      rotation about the relevant CAMERA axis
 *  @param cam
 *      Camera
 *  @param quat
 *      Rotation quaternion to compose with @p cam
 */
static inline void rc_cam_comp_right(struct rc_cam *cam, vec_t quat)
{
    cam->quat = rc_qmul(cam->quat, quat);
}


/** @brief Move the camera by its velocity according to timestep @p tau
 *  @details This uses extremely simple Euler-Cromer symplectic integration
 *  @param cam
 *      Camera
 *  @param accel
 *      Acceleration tangent vector
 *  @param tau
 *      Scale factor by which to multiply the velocity
 *  @returns true if the camera position changed, false otherwise
 */
static inline bool rc_cam_update(struct rc_cam *cam, vec_t accel, scal_t tau)
{
    const scal_t threshold = 1e-3;
    const vec_t og = cam->org;
    vec_t tstep;

    tstep = rc_set1(tau);
    cam->vel = rc_fmadd(accel, tstep, cam->vel);
    cam->org = rc_fmadd(cam->vel, tstep, cam->org);
    return rc_cvtsf(rc_vsqrnorm(rc_sub(cam->org, og))) > threshold;
}


/** Pixel information for a target texture. This contains no geometric
 *  information
 */
struct rc_texture {
    unsigned dim[2];  /* Pixel dimensions */
    unsigned stride;  /* Pixel size in bytes */
    void    *pixels;  /* Pixel data */
};


/** A target texture and relevant information. You set the texture components,
 *  but don't touch size and res
 */
struct rc_target {
    vec_t             size;     /* The physical size in each dimension */
    vec_t             res;      /* Pixel spacing */
    struct rc_texture tex;      /* Texture details */
};


/** Extra information that cannot be comfortably considered part of the camera
 *  or texture. All of this information should be set by you
 */
struct rc_screen {
    unsigned dim[2];    /* Pixel dimensions */
    double   fov;       /* Horizontal field of view (in degrees) */
};


/** @brief Update @p target with screen-specific information. This should be
 *      called to initialize @p target and whenever @p screen changes
 *  @param target
 *      Target context
 *  @param screen
 *      Screen buffer information
 */
void rc_target_update(struct rc_target *target, const struct rc_screen *screen);


/** @brief Volume raycast @p dose to @p target
 *  @param dose
 *      Dose volume
 *  @param target
 *      Render target
 *  @param cmap
 *      Colormap
 *  @param camera
 *      Camera information
 */
void rc_raycast_dose(const struct rc_dose *dose,
                     struct rc_target     *target,
                     struct rc_colormap   *cmap,
                     const struct rc_cam  *camera);


#endif /* RAYCAST_H */
