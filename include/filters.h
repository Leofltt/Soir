#ifndef FILTERS_H
#define FILTERS_H

#include <3ds/types.h>

typedef enum {
    NDSP_BIQUAD_NONE = 0,
    NDSP_BIQUAD_LOPASS = 1,
    NDSP_BIQUAD_HIPASS = 2,
    NDSP_BIQUAD_BANDPASS = 3,
    NDSP_BIQUAD_NOTCH = 4,
    NDSP_BIQUAD_PEAK = 5
} NdspFilterType;

extern const char* ndsp_biquad_filter_names[];

typedef struct {
    NdspFilterType filter_type;
    bool update_params;
    float cutoff_freq;
    int id;
} NdspBiquad;

extern void update_ndspbiquad(NdspBiquad filter);

#endif  // FILTERS_H