#pragma once

#ifndef SPIN_PARAMS_H
#define SPIN_PARAMS_H


struct params {
    double      colat;      /* -a, --angle (colatitude in degrees) */
    double      dist;       /* -d, --dist (centroid distance) */
    double      fov;        /* -f, --fov (horizontal field of view) */
    int         fcnt;       /* -n, --frames (number of frames) */
    int         ftime;      /* -t, --time (frame time in ms) */
    int         width;      /* -w, --width; also --dim WIDTH HEIGHT */
    int         height;     /* -h, --height; also --dim WIDTH HEIGHT */
    const char *file;       /* The input file (positional argument zero) */
    const char *output;     /* The output path (positional argument one) */
};


/** @brief Read cmd args
 *  @param argc
 *      Cmdarg count
 *  @param argv
 *      Cmdarg strings
 *  @param res
 *      Program parameters to be filled
 *  @returns Nonzero on error
 */
int spin_parse_opt(int argc, char *argv[], struct params *res);


#endif /* SPIN_PARAMS_H */
