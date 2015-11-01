#include <stdio.h>

#include "actions.h"
#include "envelope.h"
#include "utils.h"
#include "dynamic_wlist.h"

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

int track_action(expression_t *arg, sgen_ctx_t *c) { 

	if (c->num_tracks == 0) { c->tracks = malloc(sizeof(track_t)); }
	++c->num_tracks;

	c->tracks = realloc(c->tracks, c->num_tracks*sizeof(track_t));

	track_t t;
	if (!read_track(arg, &t, c)) return 0;

	c->tracks[c->num_tracks-1] = t;

	return 1;
}

int sample_action(expression_t *arg, sgen_ctx_t *c) { 
	printf("sgen: sample: NYI! :/\n"); 
	return 1; 
}

int tempo_action(expression_t *arg, sgen_ctx_t *c) { 

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

int duration_action(expression_t *arg, sgen_ctx_t *c) { 
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

int samplerate_action(expression_t *arg, sgen_ctx_t *c) { return 1; }

int envelope_action(expression_t *arg, sgen_ctx_t *c) { 

	if (arg->wlist->num_items < 2) {
		SGEN_ERROR("invalid envelope input.\n");
		return 0;
	}
	if (c->num_envelopes < 1) { c->envelopes = malloc(sizeof(envelope_t)); }
	++c->num_envelopes;

	c->envelopes = realloc(c->envelopes, c->num_envelopes*sizeof(envelope_t));	// TODO: this sux, lazy as hell 
	char *envname;

	if ((envname = get_primitive_identifier(arg)) == NULL) { return 0; }

	// parse args

	dynamic_wlist_t *args = get_primitive_args(arg);
	if (!args) return 0;

	float parms[ENV_NUM_PARMS];
	
	if (args->num_items < 6) {
		SGEN_WARNING("expected exactly %d (ie. Amplitude [0;1], Attack, Decay, Sustain, Sustain Level [0;1] & Release) numeric arguments, got %d\n", ENV_NUM_PARMS, (int)args->num_items);
		for (int i = ENV_NUM_PARMS - args->num_items; i < ENV_NUM_PARMS; ++i) {
			parms[i] = 1.0;
		}
	}
	else {
		for (int i = 0; i < ENV_NUM_PARMS; ++i) {
			double o;
			convert_string_to_double(args->items[i], &o);
			parms[i] = o;
		}
	}

	printf("sgen: found custom envelope \"%s\"\n", envname);
	envelope_t *e = malloc(sizeof(envelope_t));
	*e = envelope_generate(envname, 0.1, parms[ENV_ATTACK], parms[ENV_DECAY], parms[ENV_SUSTAIN], parms[ENV_SUSTAIN_LEVEL], parms[ENV_RELEASE]);
	wlist_destroy(args);

	c->envelopes[c->num_envelopes-1] = *e;

	return 1; 
}

int vibrato_action(expression_t *arg, sgen_ctx_t *c) { 

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
