#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>

typedef struct {
	char **items;
	long num_items;
	long capacity;
} dynamic_wlist_t;

typedef struct {
	char* statement;
	dynamic_wlist_t *wlist;
} expression_t;

typedef struct { 
	float attack;
	float decay;
	float sustain;
	float release;
} envelope_t; 


typedef struct {
	char *filename;
	size_t filesize;
	expression_t *exprs;
	long num_active_exprs;
	int error;
} input_t;

struct articulation_t; //nyi
struct filter_t; //nyi

typedef struct {
	int *values;
	long num_values;
	int transpose;
} note_t;

typedef struct {
	char* name;
	int sound_index;
	int loop;
	note_t *notes;
	int num_notes;
	int active;
	int transpose;
	int channel; // 0x1 == left, 0x2 == right, 0x3 == both
	int inverse;
	int reverse;
	float npb;
//	this(char[] track_expr);
} track_t;

typedef struct {
	track_t* tracks;
	int num_tracks;
	int tracks_constructed;
	dynamic_wlist_t *active_track_ids;
	float duration;
	float tempo_bpm;
//	this(char[] song_expr);
} song_t;

enum SGEN_FORMATS {
	S16_FORMAT_LE_MONO = 0,
	S16_FORMAT_LE_STEREO = 1
};

typedef struct {
	int samplerate;// = 44100;
	int channels;//  = 2;
	int bitdepth;// = 16;
	int format;// = SGEN_FORMATS.S16_FORMAT_LE_STEREO;
//	int dump_to_file(short* data);
//	int dump_to_stdout(short* data);
} output_t;

typedef struct {
	float *buffer;
	long num_samples;
} sgen_float32_buffer_t;

#endif
