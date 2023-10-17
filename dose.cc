#include <array>
#include <dcmtk/dcmrt/drmdose.h>

using namespace std;    /* fuck you */

extern "C" {
#   include "dose.h"
}


/** @brief It's just malloc, but wrapped up for mommy's special boy C++.
 *  Every time C++ makes me cast a void pointer I die a bit more inside
 *  @param size
 *      Size of the allocated block in bytes
 *  @returns A pointer to the block or NULL
 */
template <class PtrT>
static PtrT *tmalloc(size_t size)
    noexcept
{
    return reinterpret_cast<PtrT *>(malloc(size));
}


/** @brief Throw @p stat if stat.bad()
 *  @param stat
 *      OFCondition to test
 */
static void ofthrow(OFCondition stat)
{
    if (stat.bad()) {
        throw stat;
    }
}


/** @brief Update the upper bound vector
 *  @param dose
 *      Dose with dimensions array set
 */
static void rc_dose_update_bounds(struct rc_dose *dose)
    noexcept   /* I think g++ can probably tell but I'm gonna write it anyway */
{
    dose->ubnd = _mm_set_epi32(0x7FFFFFFF,
                               dose->dim[2] - 1,
                               dose->dim[1] - 1,
                               dose->dim[0] - 1);
}


/** @brief Load image dimensions
 *  @param dose
 *      Dose container
 *  @param rd
 *      RTDose
 *  @returns Nonzero on error
 */
static void rc_dose_get_dimensions(struct rc_dose *dose, const DRTDose &rd)
{
    Uint16 x, y;
    Sint32 z;

    ofthrow(rd.getColumns(x));
    ofthrow(rd.getRows(y));
    ofthrow(rd.getNumberOfFrames(z));
    dose->dim[0] = x;
    dose->dim[1] = y;
    dose->dim[2] = z;
    rc_dose_update_bounds(dose);
}


/** @brief Fetch ImagePositionPatient and stick it into the affine matrix
 *  @param dose
 *      Dose container
 *  @param rd
 *      RTDose
 */
static void rc_dose_get_origin(struct rc_dose *dose, const DRTDose &rd)
{
    RC_ALIGN scal_t spill[4];
    std::array<Float64, 3> org;
    unsigned long i;

    for (i = 0; i < org.size(); i++) {
        ofthrow(rd.getImagePositionPatient(org[i], i));
    }
    spill[0] = org[0];
    spill[1] = org[1];
    spill[2] = org[2];
    spill[3] = 1.0;
    dose->mat[3] = rc_load(spill);
}


/** @brief Get the orthonormal dose matrix
 *  @param dose
 *      Dose container
 *  @param rd
 *      RTDose
 */
static void rc_dose_get_ortho(struct rc_dose *dose, const DRTDose &rd)
{
    RC_ALIGN scal_t spill[4] = { 0 };
    std::array<Float64, 6> orient;
    unsigned long i;

    for (i = 0; i < orient.size(); i++) {
        ofthrow(rd.getImageOrientationPatient(orient[i], i));
    }
    spill[0] = orient[0];
    spill[1] = orient[1];
    spill[2] = orient[2];
    dose->mat[0] = rc_load(spill);
    spill[0] = orient[3];
    spill[1] = orient[4];
    spill[2] = orient[5];
    dose->mat[1] = rc_load(spill);
    dose->mat[2] = rc_cross(dose->mat[0], dose->mat[1]);
}


/** @brief Fetch the pixel spacing and multiply it into the affine matrix
 *  @param dose
 *      Dose container
 *  @param rd
 *      RTDose
 */
static void rc_dose_get_spacing(struct rc_dose *dose, const DRTDose &rd)
{
    std::array<Float64, 3> res;
    unsigned long i;
    vec_t vec;

    ofthrow(rd.getPixelSpacing(res[0], 0));
    ofthrow(rd.getPixelSpacing(res[1], 1));
    ofthrow(rd.getSliceThickness(res[2]));
    for (i = 0; i < res.size(); i++) {
        vec = rc_set1(res[i]);
        dose->mat[i] = rc_mul(dose->mat[i], vec);
    }
}


/** @brief Fetch the pixel data
 *  @param dose
 *      Dose container
 *  @param rd
 *      RTDose
 */
static void rc_dose_get_pixels(struct rc_dose *dose, const DRTDose &rd)
{
    const size_t framelen = (size_t)dose->dim[0] * dose->dim[1];
    const size_t len = framelen * dose->dim[2];
    std::unique_ptr<double[]> data;
    std::vector<Float64> image;
    unsigned long k;
    double *dest;

    dose->dmax = 0.0;
    dose->centr = rc_set(0, 0, 0, 1);
    data.reset(new (std::nothrow) double[len]);
    if (!data) {
        throw OFCondition(0,
                          0,
                          OF_error,
                          (len) ? "Not enough memory" : "Empty image");
    }
    dest = data.get();
    for (k = 0; k < dose->dim[2]; k++) {
        ofthrow(rd.getDoseImage(image, k));
        for (double px: image) {
            dose->dmax = std::max(dose->dmax, px);
            *dest++ = px;
        }
    }
    dose->data = data.release();
}


/** @brief Load all of the relevant data from @p rd
 *  @param dose
 *      Dose container
 *  @param rd
 *      RTDose
 */
static void rc_dose_get_data(struct rc_dose *dose, const DRTDose &rd)
{
    rc_dose_get_dimensions(dose, rd);
    rc_dose_get_origin(dose, rd);
    rc_dose_get_ortho(dose, rd);
    rc_dose_get_spacing(dose, rd);
    if (rc_matrix_invert(dose->mat, dose->inv)) {
        throw OFCondition(0, 0, OF_error, "Dose matrix is singular");
    }
    rc_dose_get_pixels(dose, rd);
}


extern "C" int rc_dose_load(struct rc_dose *dose, const char *dcm)
{
    OFCondition stat;
    DRTDose rd;
    int res = 0;

    try {
        stat = rd.loadFile(dcm);
        if (stat.bad()) {
            throw stat;
        }
        rc_dose_get_data(dose, rd);
    } catch (const OFCondition &ofc) {
        std::cerr << "dose error: " << ofc.text() << '\n';
        res = 1;
    }
    return res;
}


extern "C" void rc_dose_clear(struct rc_dose *dose)
{
    delete[] dose->data;
    dose->data = NULL;
    dose->dim[0] = 0;
    dose->dim[1] = 0;
    dose->dim[2] = 0;
}


/** @brief Check if @p idx is within the bounds of @p dose
 *  @param dose
 *      Dose volume
 *  @param idx
 *      Index vector
 *  @returns true if @p idx can safely index a pixel in @p dose, false othewise
 */
static bool rc_dose_bounds_check(const struct rc_dose *dose, __m128i idx)
    noexcept
{
    __m128i cmp;
    
    cmp = _mm_cmplt_epi32(idx, _mm_setzero_si128());
    cmp = _mm_or_si128(cmp, _mm_cmpgt_epi32(idx, dose->ubnd));
    return _mm_testz_si128(cmp, cmp);
}


extern "C" double rc_dose_nearest(const struct rc_dose *dose, vec_t pos)
{
    /* Idk if this is really UB since vectors are allowed to break strict
    aliasing... At any rate, GCC has a great time figuring out what I want to do
    when I use these unions */
    union {
        __m128i idx;
        int     xmm[4];
    } u;
    double res = 0.0;
    unsigned n;

    /* No need to round if I'm bounds checking (Not really nearest then...) */
    //pos = rc_round(pos, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
    u.idx = _mm_cvtps_epi32(pos);
    n = u.xmm[0] + dose->dim[0] * (u.xmm[1] + dose->dim[1] * u.xmm[2]);
    if (rc_dose_bounds_check(dose, u.idx)) {
        res = dose->data[n];
    }
    return res;
}


/** @brief Find the indices in each dimension of the last dose point above
 *      @p threshold
 *  @param dose
 *      Dose volume
 *  @param threshold
 *      Threshold dose
 *  @param org
 *      Origin indices of the rectangle
 *  @param end
 *      Ending indices of the rectangle
 */
static void rc_dose_findbounds(const struct rc_dose *dose,
                               double                threshold,
                               unsigned              org[],
                               unsigned              end[])
    noexcept
{
    const double *dptr = dose->data;
    unsigned i, j, k;

    org[0] = dose->dim[0] - 1;
    org[1] = dose->dim[1] - 1;
    org[2] = dose->dim[2] - 1;
    end[0] = 0;
    end[1] = 0;
    end[2] = 0;
    for (k = 0; k < dose->dim[2]; k++) {
        for (j = 0; j < dose->dim[1]; j++) {
            for (i = 0; i < dose->dim[0]; i++) {
                if (*dptr > threshold) {
                    org[0] = std::min(org[0], i);
                    org[1] = std::min(org[1], j);
                    org[2] = std::min(org[2], k);
                    end[0] = std::max(end[0], i);
                    end[1] = std::max(end[1], j);
                    end[2] = std::max(end[2], k);
                }
                dptr++;
            }
        }
    }
    end[0]++;
    end[1]++;
    end[2]++;
}


/** @brief Copy a rectangular portion of @p dose to @p dest
 *  @param dose
 *      Dose
 *  @param dest
 *      Destination buffer
 *  @param org
 *      Origin coordinates of the rectangle
 *  @param end
 *      Endpoint coordinates of the rectangle
 */
static void rc_dose_cubecpy(const struct rc_dose *dose,
                            double               *dest,
                            const unsigned        org[],
                            const unsigned        end[])
    noexcept
{
    size_t framelen = (size_t)dose->dim[0] * dose->dim[1];
    double *frame, *scan;
    unsigned i, j, k;

    for (k = org[2]; k < end[2]; k++) {
        frame = dose->data + framelen * k;
        for (j = org[1]; j < end[1]; j++) {
            scan = frame + dose->dim[0] * j;
            for (i = org[0]; i < end[0]; i++) {
                *dest = scan[i];
                dest++;
            }
        }
    }
}


extern "C" int rc_dose_compact(struct rc_dose *dose, double threshold)
{
    unsigned org[3], end[3], xlen, ylen, zlen;
    double *next;
    size_t len;
    vec_t offs;

    rc_dose_findbounds(dose, threshold * dose->dmax, org, end);
    xlen = std::max(end[0] - org[0], 0u);
    ylen = std::max(end[1] - org[1], 0u);
    zlen = std::max(end[2] - org[2], 0u);
    len = (size_t)xlen * ylen * zlen;
    next = new (std::nothrow) double[len];
    if (!next && len) {
        return 1;
    }

    printf("Compacted dose from %u x %u x %u\n"
           "                 to %u x %u x %u\n",
           dose->dim[0], dose->dim[1], dose->dim[2],
           xlen, ylen, zlen);

    rc_dose_cubecpy(dose, next, org, end);
    delete[] dose->data;
    dose->data = next;
    dose->dim[0] = xlen;
    dose->dim[1] = ylen;
    dose->dim[2] = zlen;
    rc_dose_update_bounds(dose);

    offs = rc_set((float)org[0], (float)org[1], (float)org[2], 1.0);
    dose->mat[3] = rc_mvmul4(dose->mat, offs);
    rc_matrix_invert(dose->mat, dose->inv);

    return 0;
}
