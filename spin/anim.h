#pragma once

#ifndef SPIN_ANIM_H
#define SPIN_ANIM_H

#include "params.h"
#include <webp/encode.h>
#include <webp/mux.h>


struct anim {
    WebPConfig       cfg;       /* Config for individual WebPPictures */
    WebPPicture      pic;       /* WebPPicture filled with frame data */
    WebPAnimEncoder *enc;       /* Animation encoder */
    int              width;     /* Frame width in pixels */
    int              height;    /* Frame height in pixels */
    int              tdiff;     /* Timestamp difference/time per frame */
    int              tnext;     /* Timestamp of next frame */
};


/** @brief Initialize an animation context */
int anim_init(struct anim *anim, const struct params *p);


/** @brief Free resources */
void anim_clear(struct anim *anim);


/** @brief Add a frame to @p anim */
int anim_add_frame(struct anim *anim, const void *pixels);


/** @brief Write the image */
int anim_write(struct anim *anim, const char *path);


#endif /* SPIN_ANIM_H */
