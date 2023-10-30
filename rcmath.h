#pragma once

#ifndef RC_MATH_H
#define RC_MATH_H

#include <tgmath.h>
#include <stdint.h>
#include <immintrin.h>
#include <stdalign.h>


#define RC_PI    3.141592653589793238
#define RC_ROOT2 1.4142135623730951


/** Scalar type for the vec_t typedef below */
typedef float scal_t;


/** Don't forget, quaternions are represented with their real part as the *last*
 *  element, not the first
 */
typedef __m128 vec_t;


/** Force proper alignment of spill arrays */
#define RC_ALIGN alignas (alignof (vec_t))


#define rc_spill(addr, vec)     _mm_store_ps(addr, vec)
#define rc_load(addr)           _mm_load_ps(addr)
#define rc_set(x, y, z, w)      _mm_set_ps(w, z, y, x)
#define rc_set1(val)            _mm_set1_ps(val)
#define rc_zero()               _mm_setzero_ps()

#define rc_add(a, b)            _mm_add_ps(a, b)
#define rc_sub(a, b)            _mm_sub_ps(a, b)
#define rc_mul(a, b)            _mm_mul_ps(a, b)
#define rc_div(a, b)            _mm_div_ps(a, b)
#define rc_fmadd(a, b, c)       _mm_fmadd_ps(a, b, c)
#define rc_fmsub(a, b, c)       _mm_fmsub_ps(a, b, c)

#define rc_dp(a, b, mask)       _mm_dp_ps(a, b, mask)
#define rc_rsqrt(v)             _mm_rsqrt_ps(v)
#define rc_sqrt(v)              _mm_sqrt_ps(v)

#define rc_permute(v, mask)     _mm_permute_ps(v, mask)
#define rc_permutevar(v, mask)  _mm_permutevar_ps(v, mask)
#define rc_shuffle(a, b, mask)  _mm_shuffle_ps(a, b, mask)

#define rc_cmp(a, b, op)        _mm_cmp_ps(a, b, op)
#define rc_testz(a, b)          _mm_testz_ps(a, b)
#define rc_and(a, b)            _mm_and_ps(a, b)
#define rc_or(a, b)             _mm_xor_ps(a, b)

#define rc_min(a, b)            _mm_min_ps(a, b)
#define rc_max(a, b)            _mm_max_ps(a, b)

#define rc_round(v, mask)       _mm_round_ps(v, mask)
#define rc_floor(v)             _mm_floor_ps(v)
#define rc_ceil(v)              _mm_ceil_ps(v)

/** @brief Extract the first component of @p v */
#define rc_cvtsf(v)             _mm_cvtss_f32(v)

/** @brief Convert packed 32-bit integers to a floating-point vector
 *  @todo I don't like this name, and it's only referenced once so far
 */
#define rc_cvtep(v)             _mm_cvtepi32_ps(v)

#if defined(__cplusplus) && __cplusplus
extern "C" {
#endif


/** @brief Invert 4x4 matrix @p mat
 *  @param[in] mat
 *      Input matrix
 *  @param[out] inv
 *      Output matrix
 *  @returns Zero on success, nonzero if @p mat is singular
 */
int rc_matrix_invert(const vec_t mat[], vec_t inv[]);


/** @brief Multiply 3x3 matrix @p mat by 3x1 vector @p v
 *  @param mat
 *      Matrix
 *  @param v
 *      Vector
 *  @returns The result
 */
static inline vec_t rc_mvmul3(const vec_t mat[], vec_t v)
{
    vec_t res;

    res = rc_zero();
    res = rc_fmadd(mat[0], rc_permute(v, _MM_SHUFFLE(0, 0, 0, 0)), res);
    res = rc_fmadd(mat[1], rc_permute(v, _MM_SHUFFLE(1, 1, 1, 1)), res);
    res = rc_fmadd(mat[2], rc_permute(v, _MM_SHUFFLE(2, 2, 2, 2)), res);
    return res;
}


/** @brief Multiply 4x4 matrix @p mat by 4x1 vector @p v
 *  @param mat
 *      Matrix
 *  @param v
 *      Vector
 *  @returns The result of the operation
 */
static inline vec_t rc_mvmul4(const vec_t mat[], vec_t v)
{
    vec_t res;

    res = rc_mvmul3(mat, v);
    res = rc_fmadd(mat[3], rc_permute(v, _MM_SHUFFLE(3, 3, 3, 3)), res);
    return res;
}


/** @brief Multiply 3x3 matrix @p lhs by @p rhs and store the results in @p rhs
 *  @param lhs
 *      Left matrix
 *  @param rhs
 *      Right matrix and destination
 */
static inline void rc_mmmul3(const vec_t lhs[], vec_t rhs[])
{
    rhs[0] = rc_mvmul3(lhs, rhs[0]);
    rhs[1] = rc_mvmul3(lhs, rhs[1]);
    rhs[2] = rc_mvmul3(lhs, rhs[2]);
}


/** @brief Multiply 4x4 matrix @p lhs by @p rhs and store the result in @p rhs
 *  @param lhs
 *      Left matrix
 *  @param rhs
 *      Right matrix and destination
 */
static inline void rc_mmmul4(const vec_t lhs[], vec_t rhs[])
{
    rc_mmmul3(lhs, rhs);
    rhs[3] = rc_mvmul4(lhs, rhs[3]);
}


/** @brief Compute the Euclidean cross product of @p a and @p b. The low bits of
 *      the result are zeroed by this operation (i.e. this will turn coordinate
 *      vectors into tangent vectors)
 *  @param a
 *      Left operand
 *  @param b
 *      Right operand
 *  @returns @p a × @p b
 */
static inline vec_t rc_cross(vec_t a, vec_t b)
{
    vec_t aperm, bperm, res;

    aperm = rc_permute(a, _MM_SHUFFLE(3, 0, 2, 1));
    bperm = rc_permute(b, _MM_SHUFFLE(3, 0, 2, 1));
    res = rc_mul(b, aperm);
    res = rc_fmsub(a, bperm, res);
    return rc_permute(res, _MM_SHUFFLE(3, 0, 2, 1));
}


/** @brief Compute the squared norm of @p v
 *  @param v
 *      Vector
 *  @returns A vector containing the squared norm of @p v in every lane
 *  @note This results in the intended effect when used on quaternions as well
 */
static inline vec_t rc_vsqrnorm(vec_t v)
{
    return rc_dp(v, v, 0xFF);
}


/** @brief Quickly normalize @p v. This function uses the rsqrtps instruction
 *      and as a result, is less accurate. This will turn a zero vector into a
 *      NaN vector, so be wary of its use
 *  @param v
 *      Vector to normalize
 *  @returns The unit vector pointing in the direction of @p v
 *  @note Supplying a quaternion as @p v will also result in the intended effect
 *      at reduced accuracy
 *  @warning This will likely not have the expected effect when used on a
 *      coordinate vector, as it will affect and be affected by the projective
 *      component
 */
static inline vec_t rc_vnorm(vec_t v)
{
    vec_t norm;

    norm = rc_vsqrnorm(v);
    norm = rc_rsqrt(norm);
    return rc_mul(v, norm);
}


/** @brief Fully normalize @p quat. This will turn a zero quaternion into a NaN
 *      quaternion, so be warned
 *  @param quat
 *      Quaternion
 *  @returns The versor corresponding to @p quat. This operation does not use
 *      rsqrtps, and is more expensive as a result
 *  @note Supplying a tangent vector as @p quat will result in the intended
 *      normalization effect, but at improved accuracy over rc_vnorm
 */
static inline vec_t rc_qnorm(vec_t quat)
{
    vec_t norm;

    norm = rc_vsqrnorm(quat);
    norm = rc_sqrt(norm);
    return rc_div(quat, norm);
}


/** @brief Extract the vector part of @p quat by simply zeroing its real part
 *  @param quat
 *      Quaternion
 *  @returns The vector part of @p quat
 */
static inline vec_t rc_qvec(vec_t quat)
{
    RC_ALIGN scal_t spill[4];

    rc_spill(spill, quat);
    spill[3] = 0.0;
    return rc_load(spill);
}


/** @brief Compute the Hamilton product of @p q1 and @p q2
 *  @param q1
 *      Left operand
 *  @param q2
 *      Right operand
 *  @returns The result of the operation
 */
static inline vec_t rc_qmul(vec_t q1, vec_t q2)
{
    RC_ALIGN scal_t spill[4];
    vec_t r1, r2, acc;

    r1 = rc_permute(q1, _MM_SHUFFLE(3, 3, 3, 3));
    r2 = rc_permute(q2, _MM_SHUFFLE(3, 3, 3, 3));
    q1 = rc_qvec(q1);
    q2 = rc_qvec(q2);
    acc = rc_cross(q1, q2);
    acc = rc_fmadd(r2, q1, acc);
    acc = rc_fmadd(r1, q2, acc);
    rc_spill(spill, acc);
    acc = rc_dp(q1, q2, 0x71);
    spill[3] = fma(rc_cvtsf(r1), rc_cvtsf(r2), -rc_cvtsf(acc));
    return rc_load(spill);
}


/** @brief Conjugate @p quat
 *  @param quat
 *      Quaternion. This does not have to be a versor
 *  @returns The conjugate of @p quat
 */
static inline vec_t rc_qconj(vec_t quat)
{
    return _mm_xor_ps(quat, rc_set(-0.0f, -0.0f, -0.0f, 0.0f));
}


/** @brief Negate all components of @p v
 *  @param v
 *      Vector
 *  @returns The additive inverse of @p v
 */
static inline vec_t rc_vnegate(vec_t v)
{
    return _mm_xor_ps(v, rc_set(-0.0f, -0.0f, -0.0f, -0.0f));
}


/** @brief Rotate @p v by versor @p quat
 *  @param quat
 *      Unit quaternion
 *  @param v
 *      Vector to rotate.
 *  @returns The rotation of @p v
 */
static inline vec_t rc_qrot(vec_t quat, vec_t v)
{
    return rc_qmul(rc_qmul(quat, v), rc_qconj(quat));
}


/** @brief Project homogeneous coordinates in @p v to the unit affine plane
 *  @param v
 *      (Homogeneous) coordinate vector
 *  @returns @p v with all components divided by its last. Do note that
 *      supplying a tangent vector to this function will instantly provide you
 *      with a NaN vector as a result
 */
static inline vec_t rc_vhproj(vec_t v)
{
    return rc_div(v, rc_permute(v, _MM_SHUFFLE(3, 3, 3, 3)));
}


/** @brief Exponentiate @p quat
 *  @param quat
 *      Quaternion base
 *  @param pow
 *      Integral power to raise @p quat to
 *  @returns The result
 */
vec_t rc_qpow(vec_t quat, int pow);


/** @brief Exponentiate versor @p vers
 *  @warning No checks are performed to guarantee that @p vers is unit!
 *  @param vers
 *      Unit quaternion (aka "versor")
 *  @param pow
 *      Integral power
 *  @returns The result
 */
vec_t rc_verspow(vec_t vers, int pow);


/** @brief Create a versor that will rotate @p u to align with @p v
 *  @note Do not normalize @p u or @p v if you can help it, because this
 *      function will
 *  @param u
 *      Tangent vector to rotate
 *  @param v
 *      Tangent vector with which to align
 *  @returns A quaternion that will rotate @p u to point along @p v
 *  @warning If rotation is not possible, this function will return a quaternion
 *      that contains all NaN! You really *should* check this unless you know it
 *      will always work (i.e. neither @p u nor @p v are zero vectors)
 */
vec_t rc_qalign(vec_t u, vec_t v);


#if defined(__cplusplus) && __cplusplus
}
#endif  /* MIXED C CXX */




#if !defined(__cplusplus) || !__cplusplus


/** Single-precision max-of-two */
static inline float rc_fmaxf(float x, float y)
{
    return isgreater(x, y) ? x : y;
}

static inline float rc_fminf(float x, float y)
{
    return isless(x, y) ? x : y;
}

static inline float rc_fclampf(float x, float lo, float hi)
{
    return rc_fminf(rc_fmaxf(x, lo), hi);
}


/** Double-precision max-of-two */
static inline double rc_fmax(double x, double y)
{
    return isgreater(x, y) ? x : y;
}

static inline double rc_fmin(double x, double y)
{
    return isless(x, y) ? x : y;
}

static inline double rc_fclamp(double x, double lo, double hi)
{
    return rc_fmin(rc_fmax(x, lo), hi);
}


/** Extended-precision max-of-two */
static inline long double rc_fmaxl(long double x, long double y)
{
    return isgreater(x, y) ? x : y;
}

static inline long double rc_fminl(long double x, long double y)
{
    return isless(x, y) ? x : y;
}

static inline long double rc_fclampl(long double x,
                                     long double lo,
                                     long double hi)
{
    return rc_fminl(rc_fmaxl(x, lo), hi);
}


/** @brief Find the maximum of (x, y) */
#define rc_fmax(x, y) _Generic((x),     \
        float:       rc_fmaxf(x, y),    \
        double:      rc_fmax(x, y),     \
        long double: rc_fmaxl(x, y))


/** @brief Find the minimum of (x, y) */
#define rc_fmin(x, y) _Generic((x),     \
        float:       rc_fminf(x, y),    \
        double:      rc_fmin(x, y),     \
        long double: rc_fminl(x, y))


/** @brief Clamp @p x to the range [lo, hi] (i.e. if @p x is out of range then
 *      it is set to the closest bounding value)
 */
#define rc_fclamp(x, lo, hi) _Generic((x),      \
        float:       rc_fclampf(x, lo, hi),     \
        double:      rc_fclamp(x, lo, hi),      \
        long double: rc_fclampl(x, lo, hi))


#endif /* C ONLY */


#endif /* RC_MATH_H */
