#include "filters.h"
#include <3ds.h>

const char* ndsp_biquad_filter_names[] = {
	"None",
	"Low-Pass",
	"High-Pass",
	"Band-Pass",
	"Notch",
	"Peaking"
};

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