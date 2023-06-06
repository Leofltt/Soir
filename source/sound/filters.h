#ifndef FILTERS_H
#define FILTERS_H

typedef enum {
	NDSP_BIQUAD_NONE = 0,
	NDSP_BIQUAD_LOPASS = 1,
	NDSP_BIQUAD_HIPASS = 2,
	NDSP_BIQUAD_BANDPASS = 3,
	NDSP_BIQUAD_NOTCH = 4,
	NDSP_BIQUAD_PEAK = 5
} NdspFilterType;

const char* ndsp_biquad_filter_names[] = {
		"None",
		"Low-Pass",
		"High-Pass",
		"Band-Pass",
		"Notch",
		"Peaking"
	};

typedef struct {
    NdspFilterType filter_type;
    bool update_params;
    float cutoff_freq;
	int id;
} NdspBiquad;

void update_ndspbiquad(NdspBiquad filter) {
    switch (filter.filter_type) {
			default:
				ndspChnIirBiquadSetEnable(filter.id, false);
				break;
			case 1:
				ndspChnIirBiquadSetParamsLowPassFilter(filter.id, filter.cutoff_freq, 0.707f);
				break;
			case 2:
				ndspChnIirBiquadSetParamsHighPassFilter(filter.id, filter.cutoff_freq, 0.707f);
				break;
			case 3:
				ndspChnIirBiquadSetParamsBandPassFilter(filter.id, filter.cutoff_freq, 0.707f);
				break;
			case 4:
				ndspChnIirBiquadSetParamsNotchFilter(filter.id, filter.cutoff_freq, 0.707f);
				break;
			case 5:
				ndspChnIirBiquadSetParamsPeakingEqualizer(filter.id, filter.cutoff_freq, 0.707f, 3.0f);
				break;
			}
};

#endif // FILTERS_H