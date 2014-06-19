#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "track.h"
#include "utils.h"
#include "waveforms.h"
#include "envelope.h"

static int boolean_str_true(const char* str) {
	if ((strcmp(str, "true") == 0 || strcmp(str, "1") == 0)) return 1;
	else if ((strcmp(str, "false") == 0 || strcmp(str, "0") == 0)) return 0;
	else {
		SGEN_WARNING("expected a boolean string as input (ie. \"true\" / \"1\" or \"false\" / \"0\"), got \"%s\" instead\n", str);
		return -1;
	}
}

static int track_beatdiv_action(const char* val, track_t* t, sgen_ctx_t *c) {
	double d = 1;
	convert_string_to_double(val, &d);
	t->notes_per_beat = d;

	return 1;
}

static int track_channel_action(const char* val, track_t* t, sgen_ctx_t *c) {
	if (strcmp(val, "left") == 0 || strcmp(val, "l") == 0) {
		t->channel |= 0x1;
	}
	else if (strcmp(val, "right") == 0 || strcmp(val, "r") == 0) {
		t->channel |= 0x2;
	}
	else if (strcmp(val, "stereo") == 0 || strcmp(val, "both") == 0 || strcmp(val, "l+r") == 0) {
		t->channel = 0x1 | 0x2;
	}
	else {
		SGEN_WARNING("args: unrecognized channel option \"%s\". defaulting to stereo (l+r)", val);
	}
	return 1;
}

static int track_sound_action(const char* val, track_t* t, sgen_ctx_t *c) {
	int found = 0;
	for (int i = 0; i < num_sounds; ++i) {
		if (strcmp(val, sounds[i].name) == 0) {
			printf("track %s: found sound prop with val \"%s\"\n", t->name, val);
			t->sound = sounds[i];
			found = 1;
			break;
		}
	}
	if (!found) {
		SGEN_WARNING("track %s: undefined sound nameid \"%s\", defaulting to sine.\n", t->name, val);
	}

	return 1;
}

static int track_reverse_action(const char* val, track_t* t, sgen_ctx_t *c) {
	if (boolean_str_true(val)) t->reverse = 1;
	return 1;
}

static int track_inverse_action(const char* val, track_t* t, sgen_ctx_t *c) {
	if (boolean_str_true(val)) t->inverse = 1;
	return 1;
}

static int track_equal_temperament_steps_action(const char* val, track_t* t, sgen_ctx_t *c) {
	double d = 1;
	convert_string_to_double(val, &d);
	t->eqtemp_coef = pow(2, 1.0/d);
	return 1;
}

static int track_loop_action(const char* val, track_t* t, sgen_ctx_t *c) {
	if (boolean_str_true(val)) t->loop = 1;
	return 1;
}

static int track_active_action(const char* val, track_t* t, sgen_ctx_t *c) {
	if (!boolean_str_true(val)) t->active = 0;
	return 1;
}

static int track_transpose_action(const char* val, track_t* t, sgen_ctx_t *c) {
	double d = 1;
	convert_string_to_double(val, &d);
	t->transpose = d;

	return 1;
}

static int track_delay_action(const char* val, track_t* t, sgen_ctx_t *c) {
	double d = 1;
	convert_string_to_double(val, &d);
	t->delay = d;

	return 1;
}

static int track_envelope_action(const char* val, track_t* t, sgen_ctx_t *c) {

	if (strcmp(val, "random-per-note") == 0) {
		t->envelope_mode = ENV_RANDOM_PER_NOTE;
		return 1;
	}

	else if (strcmp(val, "random-per-track") == 0) {
		t->envelope_mode = ENV_RANDOM_PER_TRACK;
		t->envelope = malloc(sizeof(envelope_t));
		*t->envelope = random_envelope();
		return 1;
	}

	int found = 0;
	for (int i = 0; i < c->num_envelopes; ++i) {
		if (strcmp(val, c->envelopes[i].name) == 0) {
			t->envelope = &c->envelopes[i];
			found = 1;
			printf("found envelope \"%s\" from list\n", val);
			break;
		}
	}
	if (!found) {
		SGEN_WARNING("use of undefined envelope id \"%s\", defaulting to \"default\".\n", val);
		return 0;
	}
	else return 1;
}
	
static int track_vibrato_action(const char* val, track_t *t, sgen_ctx_t *c) {

	int found = 0;
	fprintf(stderr, "%s, %d\n", val, c->num_vibratoes);
	for (int i = 0; i < c->num_vibratoes; ++i) {
		if (strcmp(val, c->vibratoes[i].name) == 0) {
			t->vibrato = &c->vibratoes[i];
			found = 1;
			printf("found vibrato \"%s\" from list\n", val);
			break;
		}
	}
	if (!found) {
		SGEN_WARNING("use of undefined vibrato id \"%s\", defaulting to \"default\".\n", val);
		return 0;
	}
	else return 1;

}

static int track_unknown_action(const char* val, track_t *t, sgen_ctx_t *c) {
	SGEN_WARNING("unknown prop with value \"%s\", ignoring.\n", val); // TODO: somehow propagate the unknown string to this
	return 1;
}


const track_prop_action_t track_prop_actions[] = {
	{ "beatdiv", { "<numeric>", NULL }, 1, track_beatdiv_action },
	{ "channel", { "left", "l", "right", "r", "l+r", "both", "stereo", NULL }, 7, track_channel_action },
	{ "sound", { "<primitive-id>", "sample-from-file", NULL }, 2, track_sound_action },
	{ "reverse", { "<boolean>", NULL }, 1, track_reverse_action },
	{ "inverse", { "<boolean>", NULL }, 1, track_inverse_action },
	{ "equal_temperament_steps", { "<numeric>", NULL }, 1, track_equal_temperament_steps_action },
	{ "loop", { "<boolean>", NULL }, 1, track_loop_action },
	{ "active", { "<boolean>", NULL }, 1, track_active_action },
	{ "transpose", { "<numeric>", NULL }, 1, track_transpose_action },
	{ "delay", { "<numeric>", NULL }, 1, track_delay_action },
	{ "envelope", { "<primitive-id>", "random-per-track", "random-per-note", NULL }, 3, track_envelope_action },
	{ "vibrato", { "<primitive-id>", NULL }, 1, track_vibrato_action },
	{ "unknown", { NULL }, 0, track_unknown_action }
};


const size_t num_track_prop_actions = sizeof(track_prop_actions)/sizeof(track_prop_actions[0]);

