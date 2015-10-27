#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <fftw3.h>
#include <ctype.h>

#include "utils.h"
#include "types.h"
#include "waveforms.h"
#include "string_allocator.h"
#include "string_manip.h"
#include "dynamic_wlist.h"
#include "envelope.h"
#include "WAV.h"
#include "track.h"
#include "timer.h"

static int read_track(expression_t *track_expr, track_t *t, sgen_ctx_t *c);
static int read_song(expression_t *track_expr, song_t *s, sgen_ctx_t *c);

static char *get_primitive_identifier(expression_t *prim_expr);
static dynamic_wlist_t *get_primitive_args(expression_t *prim_expr);

typedef int (*keywordfunc)(expression_t*, sgen_ctx_t*);

static int map_notename_to_int(const char *notestr) {
	
	if (!notestr) return -1;
	if (str_all_digits(notestr)) return str_to_int_b10(notestr); 

//	size_t nlen = strlen(notestr);

	char *note_lower = str_tolower(notestr);
	if (!note_lower) return -1;

	char *n = &note_lower[0];
	if (*n == 'h') *n = 'b'; // convert h to b

	if (*n < 'a' || *n > 'g') { 
		SGEN_ERROR("map_notename_to_int: invalid notename: \"%s\"\n", notestr);
		return -1;
	}	

	int base = -1;

	// trusting on the compiler's ability to optimize this shit, will probably compile into a cute jump table :D
	switch (*n) {
		case 'c':
			base = 0;
			break;
		case 'd':
			base = 2;
			break;
		case 'e':
			base = 4;
			break;
		case 'f':
			base = 5;
			break;
		case 'g':
			base = 7;
			break;
		case 'a':
			base = 9;
			break;
		case 'b':
			base = 11;
			break;
		default:
			break;
	}

	int rval = 0;

	if (strcmp(note_lower, "as") == 0) {
		return 8;
	}
	else if (strcmp(note_lower, "es") == 0) {
		return 3;
	}
	else if (strcmp(note_lower + 1, "is") == 0 || strcmp(note_lower + 1, "#") == 0) {
		rval = base+1;
	}
	else if (strcmp(note_lower + 1, "es") == 0 || strcmp(note_lower + 1, "b") == 0) {
		rval = base-1;
	}
	else rval = base;

	return rval;
	
}

int song_action(expression_t *arg, sgen_ctx_t *c) { 

	song_t s;

	if (c->num_songs == 0) { c->songs = malloc(sizeof(song_t)); }
	++c->num_songs;

	c->songs = realloc(c->songs, c->num_songs*sizeof(song_t));

	if (!read_song(arg, &s, c)) return 0;

	printf("sgen: found song block \"%s\" with tracks: ", s.name);
	dynamic_wlist_print(s.active_track_ids);

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
	dynamic_wlist_destroy(args);

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

static const struct { 
	const char* keyword; 
	keywordfunc action; 
} keyword_action_pairs[] = { 

{ "song", song_action}, 
{ "track", track_action }, 
{ "sample", sample_action },
{ "tempo", tempo_action },
{ "duration", duration_action },
{ "samplerate", samplerate_action },
{ "envelope", envelope_action },
{ "vibrato", vibrato_action }

};


static char* get_primitive_identifier(expression_t *track_expr) {
	char* index = NULL;

	char *w = track_expr->wlist->items[1];
	if ((index = strchr(w, '(')) != NULL) {
		return tidy_string(substring(w, 0, index - w));
	}
	else {
		SGEN_ERROR("syntax error: must use argument list (\"()\", even if empty) before primitive contents (\"{}\"). (got \"%s\")", track_expr->statement);
		return NULL;
	}
}

static dynamic_wlist_t *get_primitive_args(expression_t *prim_expr) {

	char *argstr;
	if (find_stuff_between('(', ')', prim_expr->statement, &argstr) <= 0) { return NULL; }
	
	dynamic_wlist_t *wr = tokenize_wr_delim(argstr, ",");
	sa_free(argstr);

	return wr;
}

static void dump_note_t(note_t *n) {
	fprintf(stderr, "note: num_children = %d\n", n->num_children);
	if (n->num_children == 0) {
		fprintf(stderr, "(single) note pitch: %d\n", n->pitch);
		return;
	}	
	for (int i = 0; i < n->num_children; ++i) {
		fprintf(stderr, "(chord) value %d/%d: %d\n", i+1, n->num_children, n->children[i].pitch);
		//printf("(chord) value %d/%d: \n", i, n->num_values);
	}
	
}

static int count_transposition(const char *notestr) {

	size_t len = strlen(notestr);
	int up = 0, down = 0;
	for (int i = 0; i < len; ++i) {
		if (notestr[i] == '\'') ++up;
		else if (notestr[i] == ',') ++down;
	}
	return (up - down)*12;

}

static int validate_digit_note(const char *notestr) {

	double d = 0;
	int ret = 0;
	if ((ret = convert_string_to_double(notestr, &d)) < 1) {
		if (ret == -1) {
			return 0; //invalid, couldn't perform even partial conversion
		}
		if (ret == 0) {
			// the only valid non-digit characters after the digit part (as of now) are '\'' and ','
			dynamic_wlist_t *tokens = tokenize_wr_delim(notestr, "\',");
			if (tokens->num_items == 1) {
				// then there's most definitely erroneous characters
				dynamic_wlist_destroy(tokens);
				return 0;
			} else {
				for (int i = 1; i < tokens->num_items; ++i) {
					if (tokens->items[i] != NULL) { // there should only be NULL tokens if it's correct
						dynamic_wlist_destroy(tokens);
						return 0;
					}
				}
			}
			dynamic_wlist_destroy(tokens);
		}
	}

	return 1;
}

static int parse_digit_note(const char *notestr, note_t *note, track_ctx_t *ctx) {

	dynamic_wlist_t *tokens = tokenize_wr_delim(notestr, "_");
		
	const char *basestr = NULL, *valuestr = NULL;

	if (tokens->num_items > 2) {
		SGEN_ERROR("parse_note: invalid digit-based note \"%s\" (multiple \'_\'s?)\n", notestr);
		return -1;
	}

	if (tokens->num_items > 1) {
		basestr = tokens->items[0];
		valuestr = tokens->items[1];
	} else {
		basestr = notestr;
		valuestr = NULL;
	}

	if (!validate_digit_note(basestr)) {
		SGEN_ERROR("parse_note: invalid digit note! note = \"%s\"\n", notestr);
		dynamic_wlist_destroy(tokens);
		return -1;
	}


	int transp = count_transposition(basestr);	
	ctx->transpose += transp;

	// we know at this stage that the string can be (at least partially) strtod'd (because of validate_digit_note)

	float pitch;
	convert_string_to_float(basestr, &pitch);
	note->pitch = pitch + ctx->transpose;

	if (valuestr) {
		convert_string_to_float(valuestr, &note->value);
		// make new value the current value :P
		ctx->value = note->value;
	} else {
		note->value = ctx->value;
	}

	fprintf(stderr, "debug: digit_note: basestr = \"%s\", valuestr = \"%s\"\n", basestr, valuestr);

	dynamic_wlist_destroy(tokens);

	return 1;

}

static int parse_musicinput_note(const char *notestr, note_t *note, track_ctx_t *ctx) {

	size_t len = strlen(notestr);
	int i = 0;
	int j = len - 1;

	char *basestr = NULL;
	char *valuestr = NULL;

	while (i < len && isalpha(notestr[i])) ++i;
	while (j > 0 && isdigit(notestr[j])) --j; 

	// the interval notestr[i:j] should now contain any transposition stuff

	if (j >= len-1) {
		// then there's no value. should be taken from the previously active note's value
		note->value = ctx->value;
		valuestr = NULL;
	} 
	else {
		// these are string pooled (string_allocator.c), so free() is a no-op
		valuestr = substring(notestr, j+1, len - j); 

		convert_string_to_float(valuestr, &note->value);
		ctx->value = note->value;
	}
	
	basestr = substring(notestr, 0, i);

	while (i < j) {
		if (notestr[i] != ',' && notestr[i] != '\'') { 
			SGEN_ERROR("syntax error: unknown char input \'%c\'!\n", notestr[i]);
			return 0; 
		}
		++i;
	}
	
	int pitchval = map_notename_to_int(basestr);
	if (pitchval < 0) {
		SGEN_ERROR("parse_note: syntax error: unresolved notename \"%s\"!\n", basestr);
		return 0;
	}

	
	int transp = count_transposition(notestr);
	ctx->transpose += transp;

	note->pitch = pitchval + ctx->transpose;

	return 1;
}

static int parse_note(const char *notestr, note_t *note, track_ctx_t *ctx) {

	// assuming the note is already allocated here!
//	fprintf(stderr, "Debug: ctx->transpose = %d\n", ctx->transpose);

	if (isdigit(notestr[0])) return parse_digit_note(notestr, note, ctx);

	else if (isalpha(notestr[0]))  return parse_musicinput_note(notestr, note, ctx);

	else {
		SGEN_ERROR("syntax error: invalid note name \"%s\"!\n", notestr);
		return 0;
	}

	return 1;

}

static int have_chord(char **cur_note) {
	char *loc = NULL;
	if ((loc = strchr(*cur_note, '<')) != NULL) {
		if (loc != *cur_note) {  // if '<' not first char of the notestr
			SGEN_ERROR("syntax error: (chord): unexpected char input before \'<\'!");
			return -1;
		}
		return 1;
	}
	return 0;
}

static int get_chord(note_t *note, char** cur_note, char** last_note, track_ctx_t *ctx) {

	char **chord_beg = cur_note;
	char **chord_end = NULL;

	// we can't skip the current note, since it might have the terminating '>' in it, e.g. <ais>

	// find note ending with '>'
	while (cur_note <= last_note) {
		char *loc;
		if ((loc = strchr(*cur_note+1, '<')) != NULL) {
			SGEN_ERROR("syntax error: (chord): found \'<\' inside \'<\' (chord nesting is not supported!) (note = \"%s\")\n", *cur_note);
		}

		if ((loc = strchr(*cur_note, '>')) != NULL) {
			// check if '>' is actually the last character (which it should be)
			if ((loc-(*cur_note)) != (strlen(*cur_note)) - 1) { 
				SGEN_ERROR("syntax error: (chord): unexpected input after \'>\'!\n");
				return -1; 
			}
			else  {
				fprintf(stderr, "debug: found suitable end note: \"%s\"\n", *cur_note);
				chord_end = cur_note;
				break;

			}
		}

		++cur_note;
	}

	if (cur_note == NULL || cur_note == last_note) {
		// don't know if this is actually reachable
		SGEN_ERROR("syntax error: (chord) no corresponding \'>\' encountered!\n");
		return -1;
	}

	if (cur_note == chord_beg) {

		// do just a regular single note parse
		// we have previously established that the last char is '>', then just remove it

		size_t len = strlen(*cur_note);
		(*cur_note)[len-1] = '\0'; 
		if (!parse_note((*chord_beg + 1), note, ctx)) {
			SGEN_ERROR("parse_note failed! note = \"%s\"\n", (*chord_beg + 1));
			return 0;
		}

		return 1;
	}

	// construct chord

	int num_notes = (chord_end - chord_beg) + 1;

	fprintf(stderr, "num_notes: %d\n", num_notes);

	note->children = malloc(num_notes*sizeof(note_t));
	note->num_children = num_notes;

	// exclude the beginning '<' from the first notename
	if (!parse_note((*chord_beg + 1), &note->children[0], ctx)) {
		// cleanup, return error
		SGEN_ERROR("parse_note failed! note = \"%s\"\n", (*chord_beg + 1));
		return 0;
	}

	note->children[0].children = NULL;
	note->children[0].num_children = 0;

	int j = 1;
	while (j < num_notes - 1) {
		if (!parse_note(*(chord_beg + j), &note->children[j], ctx)) {
			// cleanup, return error
			SGEN_ERROR("parse_note failed! note = \"%s\"\n", (*chord_beg + j));
			return 0;
		}
		note->children[j].children = NULL;
		note->children[j].num_children = 0;
		++j;
	}
	// CONSIDER: just removing the original '>' in-place, the note strings are useless after this stage anyway

	size_t last_len = strlen(*chord_end);
	char *last_dup_clean = strdup(*chord_end);
	last_dup_clean[last_len-1] = '\0'; // remove the terminating '>' 

	if (!parse_note(last_dup_clean, &note->children[j], ctx)) {
		// cleanup
		free(last_dup_clean);
		return 0;
	}

	note->children[j].children = NULL;
	note->children[j].num_children = 0;

	free(last_dup_clean);
	return num_notes;

}

note_t *convert_notestr_wlist_to_notelist(dynamic_wlist_t *notestr_wlist, size_t *num_notes) {

	// this is kinda wasteful, since there could be < > chords, but not too bad.
	// might want to realloc shrink once we're done with the parsing
	note_t *notes = malloc(notestr_wlist->num_items * sizeof(note_t));

	int err = 0;

	int n = 0;

	char **cur_note = notestr_wlist->items;
	char **last_note = notestr_wlist->items + notestr_wlist->num_items;

	track_ctx_t ctx;
	ctx.value = 4;
	ctx.transpose = 0;

	while (cur_note < last_note) {
		fprintf(stderr, "n = %d\n", n);
		if (err) { break; }
		if (!cur_note || !*cur_note) {
			SGEN_WARNING("NULL notestr encountered!\n");
			err = 1;
			continue;
		} 
		
		int ret = 0;

		if ((ret = have_chord(cur_note)) < 0) {
			SGEN_ERROR("have_chord returned < 0!\n");
			err = 1; break;
		} 
		else if (ret > 0) {
			int num_notes = get_chord(&notes[n], cur_note, last_note, &ctx);
			if (num_notes > 0) {
				cur_note += num_notes - 1;
			} else {
				SGEN_ERROR("get_chord returned <= 0! cur_note: \"%s\"\n", *cur_note);
				err = 1; break;
			}
		} 
		else {
			if (!parse_note(*cur_note, &notes[n], &ctx)) {
				SGEN_ERROR("parse_note returned 0!\n");
				err = 1; break;
			}
			notes[n].children = NULL;
			notes[n].num_children = 0;
		}

		++n;
		++cur_note;
	}



	if (err) {
		free(notes);
		notes = NULL;
		return NULL;
	}


	*num_notes = n;
	if (*num_notes != notestr_wlist->num_items) {
		note_t *shrink_to_fit = realloc(notes, n * sizeof(note_t));
		return shrink_to_fit;
	} else {
		return notes;
	}


}

static int read_track(expression_t *track_expr, track_t *t, sgen_ctx_t *c) {

	// default vals. TODO: replace this with something cuter
	t->bpm = c->tempo_bpm;
	t->channel = 0;
	t->loop = 0;
	t->active = 1;
	t->transpose = 0;
	t->reverse = 0;
	t->inverse = 0;
	t->eqtemp_steps = 12;	// 12-equal-temperament by default :P
	t->sound = sounds[0];
	t->envelope = &default_envelope;
	t->envelope_mode = ENV_FIXED;
	t->vibrato = NULL;

	if ((t->name = get_primitive_identifier(track_expr)) == NULL) { return 0; }

	dynamic_wlist_t *args = get_primitive_args(track_expr);
	if (!args) return 0;

	for (int i = 0; i < args->num_items; ++i) {

		char *iter = args->items[i];

		dynamic_wlist_t *parms = tokenize_wr_delim(iter, "=");
		if (parms->num_items < 2) { SGEN_ERROR("reading arg list failed; split(\"=\").length < 2!\""); continue; }
		char *prop = tidy_string(parms->items[0]);
		char *val = tidy_string(parms->items[1]);

		if (!prop || !val) { 
			SGEN_ERROR("error while parsing arg list (fmt: <prop>=<val>, got !prop || !val)\n"); 
			continue; 
		}

		printf("track %s: prop \"%s\" = \"%s\"\n", t->name, prop, val);

		for (int i = 0; i < num_track_prop_actions; ++i) {
			const track_prop_action_t *ta = &track_prop_actions[i];
			if (strcmp(prop, ta->prop) == 0) {
				if (!ta->action(val, t, c)) return 0;
			}
		}

		sa_free(prop);
		sa_free(val);

		dynamic_wlist_destroy(parms);

		++iter;
	}

	if (t->envelope_mode == ENV_RANDOM_PER_TRACK) {
		t->envelope = malloc(sizeof(envelope_t));
		*t->envelope = random_envelope();
	}

	dynamic_wlist_destroy(args);

	char* track_contents;
	if (!find_stuff_between('{', '}', track_expr->statement, &track_contents)) {
		return 0;
	}

	dynamic_wlist_t *notelist = tokenize_wr_delim(track_contents, "\t\n ");

	t->notes = malloc(notelist->num_items * sizeof(note_t));
	t->num_notes = notelist->num_items;


	t->notes = convert_notestr_wlist_to_notelist(notelist, &t->num_notes);

	if (!t->notes || t->num_notes < 1) return 0;

	for (int i = 0; i < t->num_notes; ++i) {
		if (t->envelope_mode == ENV_RANDOM_PER_NOTE) {
			t->notes[i].env = malloc(sizeof(envelope_t));
			*t->notes[i].env = random_envelope();
		} else {
			t->notes[i].env = t->envelope;
		}

	}

	printf("\n");

	return 1;

}

int construct_sgen_ctx(input_t *input, sgen_ctx_t *c) {

	for (int i = 0; i < input->num_active_exprs; ++i) {
		expression_t *expriter = &input->exprs[i];

		char *w = expriter->wlist->items[0];
		if (w[0] == '/' && w[1] == '/') continue; // ignore comments

		int unknown = 1;
		for (int i = 0; i < sizeof(keyword_action_pairs)/sizeof(keyword_action_pairs[0]); ++i) {
			if (strcmp(w, keyword_action_pairs[i].keyword) == 0){
				if (!keyword_action_pairs[i].action(expriter, c)) { return 0; }
				unknown = 0;
				break;
			}
		}
		if (unknown) {
			SGEN_ERROR("sgen: error: unknown keyword \"%s\"!\n", w); // fix weird error
			return 0;
		}

	}

	if (c->num_songs == 0) { 
		printf("sgen: no song(){...} blocks found -> no input -> no output -> exiting. (Syntax error in file?)\n"); 
		return 0; 
	}

	return 1;


}

int read_song(expression_t *arg, song_t *s, sgen_ctx_t *c) {

	if ((s->name = get_primitive_identifier(arg)) == NULL) { return 0; }

	char* tracks;
	if (!find_stuff_between('{', '}', arg->statement, &tracks)) return 0;
	s->active_track_ids = tokenize_wr_delim_tidy(tracks, ",");

	// defaults
	s->tempo_bpm = 120;
	s->tracks_constructed = 0;
	s->duration_s = c->duration_s; // seconds
	s->num_envelopes = 0;

	s->tracks = malloc(s->active_track_ids->num_items * sizeof(track_t));
	s->num_tracks = s->active_track_ids->num_items;

	int have_undefined = 0;

	for (int i = 0; i < s->active_track_ids->num_items; ++i) {
		int match = 0;
		char *tname = s->active_track_ids->items[i];
		for (int j = 0; j < c->num_tracks; ++j) {
			char *stname = c->tracks[j].name;
			if (strcmp(tname, stname) == 0) {
				s->tracks[i] = c->tracks[j];
				match = 1;
				break;
			}
		}
		if (!match) {
			++have_undefined;
			SGEN_ERROR("%s: use of undefined track \"%s\"!\n", s->name, tname);
			return 0;
		}
	}

	if (have_undefined) { 
		SGEN_ERROR("(fatal) undefined tracks used, exiting.\n");
		return 0;
	}

	return 1;
}

static float *do_fftw_stuff(float *in, long num) {
	fftwf_complex *fftwout;
	float *fout;

	fftwf_plan plan_backward;
	fftwf_plan plan_forward;

	long nc = (num/2)+1;

	fftwout = fftwf_malloc(sizeof(fftwf_complex)*nc);
	plan_forward = fftwf_plan_dft_r2c_1d(num, in, fftwout, FFTW_ESTIMATE);
	fftwf_execute(plan_forward);

	/*for (int i = 0; i < nc; ++i) {
	  float x = (float)(nc-1)/(float)nc;

	  fftwout[i][0] *= x;
	  fftwout[i][1] *= x;
	  }
	 */

	fout = fftwf_malloc(sizeof(float)*num);
	plan_backward = fftwf_plan_dft_c2r_1d(num, fftwout, fout, FFTW_ESTIMATE);
	fftwf_execute(plan_backward);

	fftwf_destroy_plan(plan_forward);
	fftwf_destroy_plan(plan_backward);

	fftwf_free(fftwout);

	return fout;
}

static int sgen_dump(output_t *output) {

	static const char *outfname = "output_test.wav";

	sgen_float32_buffer_t b = output->float32_buffer;	// short

	long total_num_samples = output->channels*b.num_samples_per_channel;

	size_t outbuf_size = total_num_samples*sizeof(short);
	short *out_buffer = malloc(outbuf_size);

	for (long i = 0; i < total_num_samples; ++i) {
		out_buffer[i] = (short)(0.5*SHORT_MAX*(b.buffer[i]));
	}

	printf("sgen: dumping buffer of %ld samples to file %s.\n", b.num_samples_per_channel, outfname);

	WAV_hdr_t wav_header = generate_WAV_header(output);

	FILE *ofp = fopen(outfname, "w"); // "w" = truncate
	if (!ofp) return 0;
	fwrite(&wav_header, sizeof(WAV_hdr_t), 1, ofp);

	fwrite(out_buffer, sizeof(short), total_num_samples, ofp);
	free(out_buffer);

	fclose(ofp);
	return 1;

}

static int track_synthesize(track_t *t, long num_samples_total, float *lbuf, float *rbuf) {

	// perhaps add multi-threading to this, as in all tracks be synthesized simultaneously in threads

	printf("sgen: synthesizing track %s\n", t->name);
	printf("props: loop = %d, active = %d, transpose = %d, channel = %d, bpm = %f\n", t->loop, t->active, t->transpose, t->channel, t->bpm);
	printf("num_samples_total = %ld\n", num_samples_total);
	if (!t->active) { return 0; }

	float samplerate = 44100.0;	// get this from the output_t struct

	long num_samples_max = samplerate*(60.0/t->bpm)*4; // the length of a whole note (semibreve)

	long lbuf_offset = 0;
	long rbuf_offset = 0;

	int note_index = 0;
	float freqs[64];

	float *n_samples = malloc(num_samples_max*sizeof(float));

	double eqtemp_coef = pow(2, 1.0/t->eqtemp_steps);
	long num_samples_prev = num_samples_max;

	while (lbuf_offset < num_samples_total && rbuf_offset < num_samples_total) {
		if (note_index >= t->num_notes) {
			if (!t->loop) { break; }
			else { note_index = 0; }
		}

		long num_samples_longest = 0;

		note_t *n = &t->notes[note_index];

		float dt = 1.0/samplerate;
		float A = n->num_children > 0 ? 1.0 : 1.0;// (1.0/n->num_children) : 1.0;

		freq_from_noteindex(n, t->transpose, eqtemp_coef, freqs);
		memset(n_samples, 0x0, num_samples_prev*sizeof(float)); // memset previous samples to 0

		if (n->num_children > 0) {
			for (int i = 0; i < n->num_children; ++i) {
				long num_samples_this = num_samples_max / n->children[i].value;
				num_samples_longest = num_samples_this > num_samples_longest ? num_samples_this : num_samples_longest;
				float time = 0;
				for (long j = 0; j < num_samples_this; ++j) {
					float ea = envelope_get_amplitude_noprecalculate(j, num_samples_this, n->env);
					float tv = t->vibrato ? t->vibrato->width * sin(t->vibrato->freq*time) : 0;
					n_samples[j] += n->env->parms[ENV_AMPLITUDE]*A*ea*t->sound.wform(freqs[i], time + tv, 0);
					time += dt;
				}
			}
		}

		else {
//			if (n->pitch == 0) goto increment;

			long num_samples_this = num_samples_max / n->value;
			num_samples_longest = num_samples_this > num_samples_longest ? num_samples_this : num_samples_longest;

			float time = 0;

			for (long j = 0; j < num_samples_this; ++j) {
				float ea = envelope_get_amplitude_noprecalculate(j, num_samples_this, n->env);
				float tv = t->vibrato ? t->vibrato->width * sin(t->vibrato->freq*time) : 0;
				n_samples[j] += n->env->parms[ENV_AMPLITUDE]*A*ea*t->sound.wform(freqs[0], time + tv, 0);
				time += dt;
			}
		}

		long n_offset_l = lbuf_offset;
		long n_offset_r = rbuf_offset;

		if (t->channel & 0x1) {
			long i = 0;
			while (i < num_samples_longest && n_offset_l < num_samples_total) {
				lbuf[n_offset_l] += n_samples[i];
				++i;
				++n_offset_l;
			}
		}

		if (t->channel & 0x2) {
			long i = 0;
			while (i < num_samples_longest && n_offset_r < num_samples_total) {
				rbuf[n_offset_r] += n_samples[i];
				++i;
				++n_offset_r;
			}
		}

		lbuf_offset += num_samples_longest;
		rbuf_offset += num_samples_longest;
		++note_index;

		num_samples_prev = num_samples_longest;

	}

	free(n_samples);

	printf("track_synthesize: done.\n\n");

	return 1;
}

static int song_synthesize(output_t *o, song_t *s) {

	long num_samples_per_channel = s->duration_s*o->samplerate;

	float *lbuf = malloc(num_samples_per_channel*sizeof(float));
	float *rbuf = malloc(num_samples_per_channel*sizeof(float));

	memset(lbuf, 0x0, num_samples_per_channel*sizeof(float));
	memset(rbuf, 0x0, num_samples_per_channel*sizeof(float));

	printf("sgen: song_synthesize: num_tracks = %d\n", s->num_tracks);
	printf("sgen: song_synthesize: num_samples_per_channel = %ld\n", num_samples_per_channel);

	for (int i = 0; i < s->num_tracks; ++i) {
		track_t *t = &s->tracks[i];

		long num_samples = 0;
		if (t->loop) num_samples = s->duration_s*o->samplerate;
		else if (t->duration_s > s->duration_s) {
			SGEN_WARNING("track duration is longer than song duration, truncating!\n");
			num_samples = s->duration_s*o->samplerate;
		}
		else num_samples = t->duration_s*o->samplerate;
		track_synthesize(t, num_samples, lbuf, rbuf);
	}

	long num_samples_merged = 2*num_samples_per_channel;
	float *merged = malloc(num_samples_merged*sizeof(float));

	// interleave l and r buffers XD

	for (long i = 0; i < num_samples_per_channel; ++i) {
		merged[2*i] = lbuf[i];
		merged[2*i+1] = rbuf[i];
	}

	free(lbuf); 
	free(rbuf);

	o->float32_buffer.buffer = merged;
	o->float32_buffer.num_samples_per_channel = num_samples_per_channel;


	return 1;
}

static int lexsort_expression_cmpfunc(const void* ea, const void *eb) {
	const expression_t *a = ea;
	const expression_t *b = eb;
	return strcmp(a->statement, b->statement);
}

int file_get_active_expressions(const char* filename, input_t *input) {

	FILE *fp = fopen(filename, "r");

	if (!fp) {
		char *strerrorbuf = strerror(errno);
		SGEN_ERROR("couldn't open file %s: %s\n", filename, strerrorbuf);
		return 0;
	}

	input->filename = sa_strdup(filename);
	input->filesize = get_filesize(fp);

	char *raw_buf = malloc(input->filesize);	// allocate this dynamically, don't use sa string pool
	fread(raw_buf, 1, input->filesize, fp);

	fclose(fp);

	dynamic_wlist_t *exprs_wlist = tokenize_wr_delim_tidy(raw_buf, ";");
	free(raw_buf);

	long num_exprs = exprs_wlist->num_items;

	expression_t *exprs = malloc(num_exprs*sizeof(expression_t));
	for (int i = 0; i < exprs_wlist->num_items; ++i) {
		exprs[i].statement = exprs_wlist->items[i];
		exprs[i].wlist = tokenize_wr_delim(exprs[i].statement, " \t");
	}

	dynamic_wlist_destroy(exprs_wlist);
	input->exprs = exprs;
	input->num_active_exprs = num_exprs;

	// didn't even know this existed in the c stdlib
	//qsort(input->exprs, input->num_active_exprs, sizeof(*input->exprs), lexsort_expression_cmpfunc);

	return 1;
}

input_t input_construct(const char* filename) {
	input_t input;
	input.error = 0;
	if (!file_get_active_expressions(filename, &input)) {
		input.error |= 0x1;
	}

	return input;
}

int main(int argc, char* argv[]) {

	static const char* sgen_version = "0.01";

	if (argc < 2) {
		fprintf(stderr,"sgen: No input files. Exiting.\n");
		return 0;
	}

	fprintf(stderr, "sgen-%s-perse. Written by Esa (2014).\n", sgen_version);

	srand(time(NULL));
	default_envelope = envelope_generate("default_envelope", 1.0, 0.1, 0.1, 5, 0.1, 2);

	char *input_filename = argv[1];
	input_t input = input_construct(input_filename);

	if (input.error != 0) { 
		SGEN_ERROR("(fatal) erroneous input! Exiting.\n");
		return 1;
	}

	output_t o;

	// defaults
	o.samplerate = 44100;
	o.bitdepth = 16;
	o.channels = 2;
	o.format = S16_FORMAT_LE_STEREO;

	sgen_timer_t t = timer_construct();
	timer_begin(&t);

	sgen_ctx_t c;
	memset(&c, 0x0, sizeof(c));

	c.duration_s = 10;

	char *time = timer_report(&t);
	if (!construct_sgen_ctx(&input, &c)) return 1;

	for (int i = 0; i < c.num_songs; ++i) {
		song_synthesize(&o, &c.songs[i]);
		sgen_dump(&o);
	}

	printf("sgen: took %s.\n", timer_report(&t));
	free(time);

	return 0;
}

