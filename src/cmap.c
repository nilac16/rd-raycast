#include "cmap.h"
#include "rcmath.h"


union pixel4 {
    unsigned char comp[4];
    int           value;
};


void dose_cmapfn(struct rc_colormap *this, double dose, void *pixel)
{
    alignas (__m128) static const float prot[][4] = {
        { 0,   0,   255, 255 },
        { 0,   128, 0,   255 },
        { 255, 255, 0,   255 },
        { 255, 192, 0,   255 },
        { 255, 0,   0,   255 },
        { 255, 0,   0,   255 }
    };
    union {
        __m128 vec;
        float  xmm[4];
    } u;
    struct dose_cmap *cm = (struct dose_cmap *)this;
    const unsigned len = (sizeof prot / sizeof *prot) - 2;
    float rel, x, z;
    __m128 lo, diff;
    int idx;

    rel = dose * cm->norm;
    x = rel * (float)len;
    z = floor(x);
    x -= z;
    idx = (int)z;
    lo = _mm_load_ps(prot[idx]);
    diff = _mm_load_ps(prot[idx + 1]);
    diff = _mm_sub_ps(diff, lo);
    u.vec = _mm_fmadd_ps(_mm_set1_ps(x), diff, lo);
    *(union pixel4 *)pixel = (union pixel4){
        .comp = { u.xmm[0], u.xmm[1], u.xmm[2], u.xmm[3] }
    };
}


void dose_cmap_init(struct dose_cmap *cmap, double dosemax)
{
    cmap->base.func = dose_cmapfn;
    cmap->norm = 1.0 / dosemax;
}
