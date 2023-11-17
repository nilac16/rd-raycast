#pragma once

#ifndef RC_DOSE_H
#define RC_DOSE_H

#include "rcmath.h"

#if defined(__cplusplus) && __cplusplus
extern "C" {
#endif


/** A rectangular dose array */
struct rc_dose {
    vec_t    centr;     /* Center of dose/centroid in ambient coordinates */
    vec_t    mat[4];    /* Affine transformation matrix: Pixel to ambient */
    vec_t    inv[4];    /* Inverse affine matrix: Ambient to pixel */
    unsigned dim[3];    /* Pixel dimensions */
    __m128i  ubnd;      /* Upper bounds */
    double   dmax;      /* Maximum dose value */
    double  *data;      /* Pixel data */
};


/** @brief Load a DICOM file @p dcm
 *  @param dose
 *      Dose container
 *  @param dcm
 *      Path to DICOM file
 *  @returns Nonzero on failure/error
 */
int rc_dose_load(struct rc_dose *dose, const char *dcm);


/** @brief Delete the stored pixels and free all memory
 *  @param dose
 *      Dose container. The dimensions will be zeroed
 */
void rc_dose_clear(struct rc_dose *dose);


/** @brief Signature for a function that interpolates a dose value at a given
 *      position
 *  @param dose
 *      The dose volume
 *  @param pos
 *      Real-valued pixel coordinates
 *  @returns The interpolated dose
 */
typedef double rc_dose_interpfn_t(const struct rc_dose *dose, vec_t pos);


/** @brief Find the nearest dose value to the (real) pixel coordinates @p pos
 *  @param dose
 *      Dose
 *  @param pos
 *      Real-valued pixel position. If this is out-of-bounds then zero is
 *      returned instead
 *  @returns The nearest @p dose value to @p pos
 */
double rc_dose_nearest(const struct rc_dose *dose, vec_t pos);


/** @brief Linearly interpolate the dose at real pixel coordinates @p pos
 *  @param dose
 *      Dose volume
 *  @param pos
 *      Pixel position over the reals
 *  @returns The interpolated dose at @p pos. Corner doses out-of-bounds are
 *      clamped to zero. This may result in oddly smoothed edge artifacts
 */
double rc_dose_linear(const struct rc_dose *dose, vec_t pos);


/** @brief Compact @p dose by removing all boundary regions below a threshold.
 *      All planes that are below the computed threshold are DELETED. This is a
 *      destructive operation. The only way to restore a dose that was compacted
 *      is to reload it from disk
 *  @param dose
 *      Dose to compact
 *  @param threshold
 *      PROPORTION (i.e. <= 1.0) of max dose above which the point shall be
 *      retained
 *  @returns Nonzero if there is not enough memory to complete this operation.
 *      On error, errno(3) will be set to the relevant value. On failure,
 *      @p dose is still valid
 */
int rc_dose_compact(struct rc_dose *dose, double threshold);


#if defined(__cplusplus) && __cplusplus
}
#endif

#endif /* RC_DOSE_H */
