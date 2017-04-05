#include <stdio.h>

#include "actions.h"
#include "envelope.h"
#include "utils.h"
#include "dynamic_wlist.h"
#include "sample.h"
#include "string_manip.h"

static int song_action(expression_t *arg, sgen_ctx_t *c) { 

	song_t s;

	if (c->num_songs == 0) { c->songs = malloc(sizeof(song_t)); }
	++c->num_songs;

	c->songs = realloc(c->songs, c->num_songs*sizeof(song_t));

	if (!read_song(arg, &s, c)) return 0;

	printf("sgen: found song block \"%s\" with tracks: ", s.name);
	wlist_print(s.active_track_ids);

	c->songs[c->num_songs-1] = s;

	return 1; 
} 

static int track_action(expression_t *arg, sgen_ctx_t *c) { 

	if (c->num_tracks == 0) { c->tracks = malloc(sizeof(track_t)); }
	++c->num_tracks;

	c->tracks = realloc(c->tracks, c->num_tracks*sizeof(track_t));

	track_t t;
	if (!read_track(arg, &t, c)) return 0;

	c->tracks[c->num_tracks-1] = t;

	return 1;
}

static int sample_action(expression_t *arg, sgen_ctx_t *c) { 

	if (c->num_samples == 0) { c->samples = malloc(sizeof(sample_t)); }
	++c->num_samples;

	c->samples = realloc(c->samples, c->num_samples*sizeof(sample_t));

	dynamic_wlist_t *args = get_primitive_args(arg);
	if (!args) return 0;

	if (args->num_items < 2) {
		SGEN_WARNING("sample: invalid number of arguments! expected 2 (filename, OPENMODE)\n");
		return 0;
	}

	const char *filename = args->items[0];
	const char *openmodestr = strip(args->items[1]);
	const char *samplename;
	if ((samplename = get_primitive_identifier(arg)) == NULL) { return 0; }

	int openmode = SAMPLE_OPENMODE_BINARY;

	if (strcmp(openmodestr, "SAMPLE_OPENMODE_WAV") == 0) {
		openmode = SAMPLE_OPENMODE_WAV;
	}
	else {
		openmode = SAMPLE_OPENMODE_BINARY;
	}

	sample_t *sample = sample_file_as_U16_2ch_wav(filename, samplename, openmode);

	if (!sample) { 
		puts("sample_file_as_U16_2ch_wav returned NULL!\n");
		return 0;
       	}

	printf("constructed sample %s from input file %s with openmode %s! (size %ld bytes)\n", samplename, filename, openmodestr, sample->bufsize_bytes); 

	c->samples[c->num_samples - 1] = *sample;

	return 1; 
}

static int tempo_action(expression_t *arg, sgen_ctx_t *c) { 

	if (arg->wlist->num_items < 2) {
		SGEN_ERROR("error while parsing tempo directive. Defaulting to 120.\n");
		return 0;
	}

	char *val = arg->wlist->items[1];

	double d;

	convert_string_to_double(val, &d);
	c->tempo_bpm = d;

	printf("sgen: found tempo directive: \"%s\" bpm.\n", val);

	return 1; 
}

static int duration_action(expression_t *arg, sgen_ctx_t *c) { 
	if (arg->wlist->num_items < 2) {
		SGEN_ERROR("invalid duration directive. Defaulting to 10 seconds.\n");
		return 0;
	}
	
	char *val = arg->wlist->items[1];

	double d;
	convert_string_to_double(val, &d);
	c->duration_s = d;

	printf("sgen: found duration directive: \"%s\" s.\n", val);
	return 1; 
}

static int samplerate_action(expression_t *arg, sgen_ctx_t *c) { return 1; }

static int envelope_action(expression_t *arg, sgen_ctx_t *c) { 

	if (arg->wlist->num_items < 2) {
		SGEN_ERROR("invalid envelope input.\n");
		return 0;
	}

	char *envname;
	if ((envname = get_primitive_identifier(arg)) == NULL) { return 0; }

	// parse args
	dynamic_wlist_t *args = get_primitive_args(arg);
	if (!args) return 0;

	float parms[ENV_NUM_PARMS];
	
	if (args->num_items > ENV_NUM_PARMS) {
		SGEN_WARNING("too many envelope args (expected %d - Amplitude [0;1], Attack, Decay, Sustain, Sustain Level [0;1], Release, Relative Length, got %ld)! Ignoring excessive arguments.\n", ENV_NUM_PARMS, args->num_items);
		args->num_items = ENV_NUM_PARMS;
	}

	for (int i = 0; i < args->num_items; ++i) {
		double o;
		convert_string_to_double(args->items[i], &o);
		parms[i] = o;
	}

	if (args->num_items < ENV_NUM_PARMS) {
		SGEN_WARNING("expected %d (Amplitude [0;1], Attack, Decay, Sustain, Sustain Level [0;1], Release, Relative Length) numeric arguments, got %d (filling in missing ones from envelope \"default\").\n", ENV_NUM_PARMS, (int)args->num_items);
		// fill in the missing args from the default envelope
		envelope_t *default_envelope = ctx_get_envelope(c, "default");
		for (int i = ENV_NUM_PARMS - args->num_items; i < ENV_NUM_PARMS; ++i) {
			parms[i] = default_envelope->parms[i];
		}
	} 

	printf("sgen: found custom envelope \"%s\"\n", envname);
	envelope_t *e = envelope_generate(envname, parms[ENV_AMPLITUDE], parms[ENV_ATTACK], parms[ENV_DECAY], parms[ENV_SUSTAIN], parms[ENV_SUSTAIN_LEVEL], parms[ENV_RELEASE], parms[ENV_LEGATO]);
	wlist_destroy(args);

	ctx_add_envelope(c, e);

	return 1; 
}

static int vibrato_action(expression_t *arg, sgen_ctx_t *c) { 

	vibrato_t v;

	if (c->num_vibratoes == 0) { c->vibratoes = malloc(sizeof(vibrato_t)); }
	++c->num_vibratoes;

	c->vibratoes = realloc(c->vibratoes, c->num_vibratoes*sizeof(vibrato_t));

	if ((v.name = get_primitive_identifier(arg)) == NULL) { return 0; }

	dynamic_wlist_t *args = get_primitive_args(arg);
	if (!args) return 0;

	if (args->num_items != 2) {
		SGEN_WARNING("invalid vibrato constructor: expected exactly 2 numeric arguments (breadth, freq)! Defaulting to (0.0008, 40).\n");
		v.width = 0.0008;
		v.freq = 40;
	} else {
		double d;
		convert_string_to_double(args->items[0], &d);
		v.width = d;
		convert_string_to_double(args->items[1], &d);
		v.freq = d;
	}

	c->vibratoes[c->num_vibratoes-1] = v;

	return 1;
}

keywordfunc get_matching_action(const char* w) {

	int unknown = 1;
	for (int i = 0; i < num_keyword_action_pairs; ++i) {
		if (strcmp(w, keyword_action_pairs[i].keyword) == 0){
			return keyword_action_pairs[i].action;
		}
	}

	SGEN_ERROR("sgen: error: unknown keyword \"%s\"!\n", w); // fix weird error
	return NULL;

}


const keyword_action_pair_t keyword_action_pairs[] = { 

{ "song", song_action}, 
{ "track", track_action }, 
{ "sample", sample_action },
{ "tempo", tempo_action },
{ "duration", duration_action },
{ "samplerate", samplerate_action },
{ "envelope", envelope_action },
{ "vibrato", vibrato_action }

};

const size_t num_keyword_action_pairs = sizeof(keyword_action_pairs) / sizeof(keyword_action_pairs[0]);
