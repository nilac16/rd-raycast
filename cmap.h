#pragma once

#ifndef RC_CMAP_H
#define RC_CMAP_H

#include "dose.h"


/** Colormap base class. You know what to do with this.
 *  Do note that the raycasting function is aggressively multithreaded, so
 *  contentious actions within the callback should be fenced and sparse
 */
struct rc_colormap {
    void (*func)(struct rc_colormap *this, double dose, void *pixel);
};


/** This colormap uses a mapping similar to "Jet" of MATLAB fame, but with a
 *  superfluous alpha channel---FOUR BYTES
 */
struct dose_cmap {
    struct rc_colormap base;

    double dmax;
};


/** @brief Initialize a colormap for dose
 *  @param cmap
 *      The colormap
 *  @param dosemax
 *      The maximum dose value to use for this scale
 */
void dose_cmap_init(struct dose_cmap *cmap, double dosemax);


#endif /* RC_CMAP_H */
