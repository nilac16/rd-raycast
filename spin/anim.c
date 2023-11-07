#include <stdio.h>
#include "anim.h"


/** Initialize the picture struct used for individual frames */
static int anim_init_picture(struct anim *anim, const struct params *p)
{
    if (!WebPConfigInit(&anim->cfg)) {
        fputs("Failed to initialize WebPConfig for frame data\n", stderr);
        return 1;
    }
    if (!WebPPictureInit(&anim->pic)) {
        fputs("Failed initializing the frame picture object\n", stderr);
        return 1;
    }
    anim->pic.use_argb = 1;
    anim->pic.width = p->width;
    anim->pic.height = p->height;
    return 0;
}


int anim_init(struct anim *anim, const struct params *p)
{
    WebPAnimEncoderOptions opt = { 0 };

    if (anim_init_picture(anim, p)) {
        return 1;
    }
    if (!WebPAnimEncoderOptionsInit(&opt)) {
        fputs("Failed initializing the animation encoder\n", stderr);
        return 1;
    }
    opt.anim_params.loop_count = 0;
    opt.anim_params.bgcolor    = 0;
    opt.minimize_size = 0;
    opt.kmin          = 1;
    opt.kmax          = 1;
    opt.allow_mixed   = 1;
    opt.verbose       = 0;

    anim->enc = WebPAnimEncoderNew(p->width, p->height, &opt);
    if (!anim->enc) {
        fputs("Failed allocating the animation encoder\n", stderr);
        return 1;
    }
    anim->width  = p->width;
    anim->height = p->height;
    anim->tdiff  = p->ftime;
    anim->tnext  = 0;
    return 0;
}


void anim_clear(struct anim *anim)
{
    WebPAnimEncoderDelete(anim->enc);
    WebPPictureFree(&anim->pic);
}


int anim_add_frame(struct anim *anim, const void *pixels)
{
    const int stride = 4;

    if (!WebPPictureImportRGBA(&anim->pic, pixels, stride * anim->width)) {
        fputs("Failed importing pixel data to image frame\n", stderr);
        return 1;
    }
    if (!WebPAnimEncoderAdd(anim->enc, &anim->pic, anim->tnext, &anim->cfg)) {
        fprintf(stderr, "Failed adding image frame to animation encoder: %s\n",
                WebPAnimEncoderGetError(anim->enc));
        return 1;
    }
    anim->tnext += anim->tdiff;
    return 0;
}


int anim_write(struct anim *anim, const char *path)
{
    WebPData data = { 0 };
    FILE *fp;

    if (!WebPAnimEncoderAdd(anim->enc, NULL, anim->tnext, &anim->cfg)) {
        fprintf(stderr, "Failed adding final NULL frame: %s\n",
                WebPAnimEncoderGetError(anim->enc));
        return 1;
    }
    if (!WebPAnimEncoderAssemble(anim->enc, &data)) {
        fprintf(stderr, "Failed assembling WebP animation: %s\n",
                WebPAnimEncoderGetError(anim->enc));
        return 1;
    }

    fp = fopen(path, "wb");
    if (!fp) {
        perror("Cannot open output file");
        return 1;
    }
    if (!fwrite(data.bytes, data.size, 1UL, fp)) {
        perror("Cannot write all data to output file");
    }
    fclose(fp);
    return 0;
}
