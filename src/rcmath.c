#include <stdbool.h>
#include "rcmath.h"


/** @brief Swap vectors @p x and @p y */
static void rc_vecswap(vec_t *x, vec_t *y)
{
    vec_t swap = *x;

    *x = *y;
    *y = swap;
}


/** @brief Find a pivot column for @p idx
 *  @param mat
 *      Input matrix
 *  @param inv
 *      Output matrix
 *  @param idx
 *      Column index
 *  @returns Zero normally, nonzero if all columns contain zero in element
 *      @p idx (i.e. @p mat is singular)
 */
static int rc_matrix_pivot(vec_t mat[], vec_t inv[], const int idx)
{
    RC_ALIGN scal_t spill[4];
    int i, imax = idx;
    scal_t cur, mag;

    rc_spill(spill, mat[idx]);
    cur = mag = fabs(spill[idx]);
    for (i = idx + 1; i < 4; i++) {
        rc_spill(spill, mat[i]);
        mag = fabs(spill[idx]);
        if (isgreater(mag, cur)) {
            imax = i;
            cur = mag;
        }
    }
    if (!mag) {
        return 1;
    }
    rc_vecswap(&mat[imax], &mat[idx]);
    rc_vecswap(&inv[imax], &inv[idx]);
    return 0;
}


/** @brief Reduce diagonal entry @p idx for forward (and backward, actually)
 *      elimination
 *  @param mat
 *      Input matrix
 *  @param inv
 *      Output matrix
 *  @param idx
 *      Column index
 *  @returns Nonzero if @p mat is found to be singular
 */
static int rc_matrix_fwd_elim(vec_t mat[], vec_t inv[], const int idx)
{
    __m128i shufmask;
    vec_t diag, loc;
    int i;

    if (rc_matrix_pivot(mat, inv, idx)) {
        return 1;
    }
    shufmask = _mm_set1_epi32(idx);
    diag = rc_permutevar(mat[idx], shufmask);
    mat[idx] = rc_div(mat[idx], diag);
    inv[idx] = rc_div(inv[idx], diag);
    for (i = 0; i < 4; i++) {
        if (i == idx) {
            continue;
        }
        loc = rc_vnegate(rc_permutevar(mat[i], shufmask));
        mat[i] = rc_fmadd(mat[idx], loc, mat[i]);
        inv[i] = rc_fmadd(inv[idx], loc, inv[i]);
    }
    return 0;
}


int rc_matrix_invert(const vec_t mat[], vec_t inv[])
{
    vec_t copy[4] = { mat[0], mat[1], mat[2], mat[3] };

    inv[0] = rc_set(1.0, 0.0, 0.0, 0.0);
    inv[1] = rc_set(0.0, 1.0, 0.0, 0.0);
    inv[2] = rc_set(0.0, 0.0, 1.0, 0.0);
    inv[3] = rc_set(0.0, 0.0, 0.0, 1.0);
    return rc_matrix_fwd_elim(copy, inv, 0)
        || rc_matrix_fwd_elim(copy, inv, 1)
        || rc_matrix_fwd_elim(copy, inv, 2)
        || rc_matrix_fwd_elim(copy, inv, 3);
}


/** @brief Compute the reciprocal of @p q
 *  @param q
 *      Quaternion
 *  @returns The reciprocal of @p q
 */
static vec_t rc_qrecip(vec_t q)
{
    return rc_div(rc_qconj(q), rc_vsqrnorm(q));
}


vec_t rc_qpow(vec_t quat, int pow)
/** I have no idea if this is ideal. It seemed natural */
{
    bool neg = pow < 0;
    unsigned p;
    vec_t res;

    p = neg ? -pow : pow;
    res = rc_set(0.0, 0.0, 0.0, 1.0);
    while (p) {
        if (p % 2) {
            res = rc_qmul(res, quat);
            p--;
        } else {
            quat = rc_qmul(quat, quat);
            p /= 2;
        }
    }
    return neg ? rc_qrecip(res) : res;
}


vec_t rc_verspow(vec_t vers, int pow)
/** I think these functions are in need of a rewrite */
{
    bool neg = pow < 0;
    unsigned p;
    vec_t res;

    vers = neg ? rc_qconj(vers) : vers;
    p = neg ? -pow : pow;
    res = rc_set(0.0, 0.0, 0.0, 1.0);
    while (p) {
        if (p % 2) {
            res = rc_qmul(res, vers);
            p--;
        } else {
            vers = rc_qmul(vers, vers);
            p /= 2;
        }
    }
    return res;
}


vec_t rc_qalign(vec_t u, vec_t v)
{
    vec_t w, res;

    u = rc_qnorm(u);
    v = rc_qnorm(v);
    w = rc_qnorm(rc_add(u, v));
    res = rc_dp(w, v, 0xF8);
    res = rc_add(res, rc_cross(w, v));
    return res;
}
