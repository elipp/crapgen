#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>

struct sgen_ctx_t;

typedef struct {
	char **items;
	long num_items;
	long capacity;
} dynamic_wlist_t;

typedef struct expression_t {
	char* statement;
	dynamic_wlist_t *wlist;
} expression_t;

typedef float (*PFNWAVEFORM)(float, float, float);

typedef struct sound_t {
	const char *name;
	PFNWAVEFORM wform;
} sound_t;

enum SGEN_ENV_PARMS {
	ENV_AMPLITUDE,
	ENV_ATTACK,
	ENV_DECAY,
	ENV_SUSTAIN,
	ENV_SUSTAIN_LEVEL,
	ENV_RELEASE,
	ENV_NUM_PARMS
};

enum ENVELOPE_MODES {
	ENV_FIXED,
	ENV_RANDOM_PER_TRACK,
	ENV_RANDOM_PER_NOTE
};

typedef struct envelope_t { 
	char *name;
	float parms[6];
	float *envelope;
	int num_samples;
} envelope_t; 

typedef struct input_t {
	char *filename;
	size_t filesize;
	expression_t *exprs;
	long num_active_exprs;
	int error;
} input_t;

struct articulation_t; //nyi
struct filter_t; //nyi

typedef struct vibrato_t {
	char *name;
	float width;
	float freq;
} vibrato_t;

typedef struct note_t {
	int *values;
	long num_values;
	int transpose;
	float value; // relative duration. 1 for whole note, 2 for half note etc. (ex. a_4)
	float duration_s;
	envelope_t *env;
} note_t;

typedef struct track_t {
	char* name;

	note_t *notes;
	size_t num_notes;
	
	int loop;
	int active;
	int transpose;
	int channel; // 0x1 == left, 0x2 == right, 0x3 == both
	int inverse;
	int reverse;
	float delay;
	float notes_per_beat;
	float eqtemp_coef;
	float duration_s;
	float note_dur_s;
	sound_t sound;
	envelope_t *envelope;
	int envelope_mode;
	vibrato_t *vibrato;

} track_t;

typedef int (*PFNTRACKPROPACTION)(const char* val, track_t*, struct sgen_ctx_t*);

enum SGEN_FORMATS {
	S16_FORMAT_LE_MONO = 0,
	S16_FORMAT_LE_STEREO = 1
};

// ref: https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

typedef struct WAV_hdr_t {
	int ChunkID_BE;
	int ChunkSize_LE;
	int Format_BE;
	int Subchunk1ID_BE;
	int Subchunk1Size_LE;
	short AudioFormat_LE;
	short NumChannels_LE;
	int SampleRate_LE;
	int ByteRate_LE;
	short BlockAlign_LE;
	short BitsPerSample_LE;
	int Subchunk2ID_BE;
	int Subchunk2Size_LE;
} WAV_hdr_t;

typedef struct sgen_float32_buffer_t {
	float *buffer;
	long num_samples_per_channel;
} sgen_float32_buffer_t;

typedef struct output_t {
	int samplerate;
	int channels;
	int bitdepth;
	int format;
	sgen_float32_buffer_t float32_buffer;
} output_t;

typedef struct song_t {
	char *name;
	track_t* tracks;
	int num_tracks;
	int tracks_constructed;
	dynamic_wlist_t *active_track_ids;
	float duration_s;
	float tempo_bpm;
	envelope_t *envelopes;
	int num_envelopes;
} song_t;


typedef struct track_prop_action_t {
	const char* prop;
	const char* valid_values[16]; // TODO: this is crap.
	int num_valid_values;
	PFNTRACKPROPACTION action;
} track_prop_action_t;

typedef struct sgen_ctx_t {
	// primitives

	float tempo_bpm;
	float duration_s;

	envelope_t *envelopes;
	int num_envelopes;

	vibrato_t *vibratoes;
	int num_vibratoes;

	track_t *tracks;
	int num_tracks;

	song_t *songs;
	int num_songs;

} sgen_ctx_t;

#endif
