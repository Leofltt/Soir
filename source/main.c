#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>
#include <opusfile.h>

#include "sound/oscillators.h"
#include "sound/envelope.h"
#include "sound/filters.h"
#include "sound/audio_utils.h"

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

// static const int THREAD_AFFINITY = -1;           // Execute thread on any core
// static const int THREAD_STACK_SZ = 32 * 1024;    // 32kB stack for audio thread

// static const size_t WAVEBUF_SIZE = SAMPLESPERBUF * NCHANNELS * sizeof(int16_t); // Size of NDSP wavebufs

// ------------------------------------------------------------

int main(int argc, char **argv) {
	gfxInitDefault();

	PrintConsole topScreen;
	consoleInit(GFX_TOP, &topScreen);
	consoleSelect(&topScreen);
    
	// Enable N3DS 804MHz operation, where available
    osSetSpeedupEnable(true);

	// u32 kDownOld = 0, kHeldOld = 0, kUpOld = 0; //In these variables there will be information about keys detected in the previous frame

	u32* audioBuffer = (u32*) linearAlloc(SAMPLESPERBUF * BYTESPERSAMPLE * NCHANNELS);

	bool fillBlock = false;

	romfsInit();
	ndspInit();

	ndspChnReset(0);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, SAMPLERATE);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = 1.0;
	mix[1] = 1.0;
	ndspChnSetMix(0, mix);

	// Note Frequencies
	int notefreq[] = {
		220,
		440, 880, 1760, 3520, 7040,
		14080,
		7040, 3520, 1760, 880, 440
	};

	int note = 4;
	int cf = 5;
		
	NdspBiquad biquadFilter = { .cutoff_freq = (float)notefreq[cf],
	                      .filter_type = NDSP_BIQUAD_NONE, 
						  .update_params = false,
						  .id = 0 
	};
	NdspBiquad* filter = &biquadFilter;

	PolyBLEPOscillator pbOscillator = { .frequency = notefreq[note],
	                   .samplerate = SAMPLERATE,
                       .waveform = SINE,
					   .phase = 0.,
					   .phase_inc = notefreq[note] * M_TWOPI / SAMPLERATE
					   };
	PolyBLEPOscillator* osc = &pbOscillator;

	Envelope ampEnvelope = { .atk = 10,
	                          .dec = 10,
							  .rel = 10,
							  .dur = 40,
							  .sus_level = 0.6,
							  .sus_time = 10,
							  .gate = ENV_OFF,
							  .env_pos = 0,
							  .sr = SAMPLERATE
							};
	Envelope* env = &ampEnvelope;
	updateEnvelope(env, 20, 200, 0.4, 300, 500);

	// We set up two wave buffers and alternate between the two,
	// effectively streaming an infinitely long sine wave.

	ndspWaveBuf waveBuf[2];
	memset(waveBuf,0,sizeof(waveBuf));
	waveBuf[0].data_vaddr = &audioBuffer[0];
	waveBuf[0].nsamples = SAMPLESPERBUF;
	waveBuf[1].data_vaddr = &audioBuffer[SAMPLESPERBUF];
	waveBuf[1].nsamples = SAMPLESPERBUF;

	size_t stream_offset = 0;

	fillPolyblepAudiobuffer(audioBuffer, SAMPLESPERBUF * NCHANNELS, osc);
	fillEnvelopeAudiobuffer(audioBuffer, SAMPLESPERBUF * NCHANNELS, env);
	
	stream_offset += SAMPLESPERBUF;

	ndspChnWaveBufAdd(0, &waveBuf[0]);
	ndspChnWaveBufAdd(0, &waveBuf[1]);

	printf("Press up/down to change tone frequency\n");
	printf("Press left/right to change filter\n");
	printf("\x1b[6;1Hnote = %i Hz        ", notefreq[note]);
	printf("\x1b[7;1Hfilter = %s         ", ndsp_biquad_filter_names[filter->filter_type]);
	printf("\x1b[8;1Hcf = %i Hz          ", notefreq[cf]);
	printf("\x1b[30;16HPress Start to exit.");

	while(aptMainLoop()) {

		gfxSwapBuffers();
		gfxFlushBuffers();
		gspWaitForVBlank();
		hidScanInput();
		
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();

		if (kHeld & KEY_START) break; // break in order to return to hbmenu 

		if (kDown & KEY_X) {
			//trigger a not
			triggerEnvelope(env);
		}

		// OSC NOTE
		if (kDown & KEY_DOWN) {
			note--;
			if (note < 0) {
				note = ARRAY_SIZE(notefreq) - 1;
			}
			printf("\x1b[6;1Hnote = %i Hz        ", notefreq[note]);
			setOscFrequency(osc, notefreq[note]);
		} else if (kDown & KEY_UP) {
			note++;
			if (note >= ARRAY_SIZE(notefreq)) {
				note = 0;
			}
			printf("\x1b[6;1Hnote = %i Hz        ", notefreq[note]);
			setOscFrequency(osc, notefreq[note]);
		}
		
		// FILTER TYPE
		if (kDown & KEY_LEFT) {	
			if (filter->filter_type == NDSP_BIQUAD_NONE) {
				filter->filter_type = NDSP_BIQUAD_PEAK;
			} else { filter->filter_type--; }
			filter->update_params = true;
		} else if (kDown & KEY_RIGHT) {
			if (filter->filter_type == NDSP_BIQUAD_PEAK) {
				filter->filter_type = NDSP_BIQUAD_NONE;
			} else { filter->filter_type++; }
			filter->update_params = true;
		} 
		
		// FILTER FREQ
		if (kDown & KEY_L) {
			cf--;
			if (cf < 0) {
				cf = ARRAY_SIZE(notefreq) - 1;
			}
			filter->cutoff_freq = (float)notefreq[cf];
			filter->update_params = true;
		} else if (kDown & KEY_R) {
			cf++;
			if (cf >= ARRAY_SIZE(notefreq)) {
				cf = 0;
			}
			filter->cutoff_freq = (float)notefreq[cf];
			filter->update_params = true;
		}


		if (filter->update_params) {
			printf("\x1b[7;1Hfilter = %s         ", ndsp_biquad_filter_names[filter->filter_type]);
			printf("\x1b[8;1Hcf = %i Hz          ", notefreq[cf]);
			
			update_ndspbiquad(*filter);
			filter->update_params=false;
		}


		if (waveBuf[fillBlock].status == NDSP_WBUF_DONE) {
			fillPolyblepAudiobuffer(waveBuf[fillBlock].data_pcm16, waveBuf[fillBlock].nsamples, osc);
			fillEnvelopeAudiobuffer(waveBuf[fillBlock].data_pcm16, waveBuf[fillBlock].nsamples, env);
			ndspChnWaveBufAdd(0, &waveBuf[fillBlock]);
			stream_offset += waveBuf[fillBlock].nsamples;
			fillBlock = !fillBlock;
		}
	}

	ndspChnReset(0);
	linearFree(audioBuffer);
	linearFree(env->env_buffer);

	ndspExit();
	romfsExit();
	gfxExit();
	return 0;
}
