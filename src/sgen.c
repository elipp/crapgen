#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <time.h>

#include "utils.h"
#include "types.h"
#include "waveforms.h"
#include "string_allocator.h"
#include "string_manip.h"
#include "dynamic_wlist.h"
#include "envelope.h"
#include "WAV.h"
#include "track.h"

static int read_track(expression_t *track_expr, track_t *t, song_t *s);
static char *get_primitive_identifier(expression_t *prim_expr);

typedef int (*keywordfunc)(expression_t*, song_t*);

int song_action(expression_t *arg, song_t *s) { return 1; } //dummy

int track_action(expression_t *arg, song_t *s) { 

	track_t t;
	if ((t.name = get_primitive_identifier(arg)) == NULL) { return 0; }
	int active = 0;
	for (int i = 0; i < s->active_track_ids->num_items; ++i) {
		if (strcmp(s->active_track_ids->items[i], t.name) == 0) {
			active = 1;
		}
	}
	if (!active) {
		SGEN_WARNING("track \"%s\" defined but not used in a song block!\n", t.name);
		return 0;
	}

	if (!read_track(arg, &t, s)) { SGEN_ERROR("syntax error(?)\n"); return 0; }

	s->tracks[s->tracks_constructed] = t;
	++s->tracks_constructed;

	return 1;
}

int sample_action(expression_t *arg, song_t *s) { printf("sgen: sample: NYI! :/\n"); return 1; }
int tempo_action(expression_t *arg, song_t *s) { 
	if (arg->wlist->num_items < 2) {
		SGEN_ERROR("error while parsing tempo directive. Defaulting to 120.\n");
		return 0;
	}

	char *endptr = NULL;
	char *val = arg->wlist->items[1];
	s->tempo_bpm = strtol(val, &endptr, 10);
	printf("sgen: found tempo directive: \"%s\" bpm.\n", val);

	return 1; 
}

int duration_action(expression_t *arg, song_t *s) { 
	if (arg->wlist->num_items < 2) {
		SGEN_ERROR("invalid duration directive. Defaulting to 10 seconds.\n");
		return 0;
	}
	char *endptr = NULL;
	char *val = arg->wlist->items[1];
	s->duration_s = strtod(val, &endptr);
	printf("sgen: found duration directive: \"%s\" s.\n", val);
	return 1; 
	
}
int samplerate_action(expression_t *arg, song_t *s) { return 1; }

int envelope_action(expression_t *arg, song_t *s) { 
	if (arg->wlist->num_items < 2) {
		SGEN_ERROR("invalid envelope input.\n");
		return 0;
	}
	if (s->num_envelopes < 1) { s->envelopes = malloc(sizeof(envelope_t)); }
	++s->num_envelopes;

	s->envelopes = realloc(s->envelopes, s->num_envelopes*sizeof(envelope_t));	// TODO: this sux, lazy as hell 
	char *envname;

	if ((envname = get_primitive_identifier(arg)) == NULL) { return 0; }

	// parse args

	char *envelope_args;
	if (find_stuff_between('(', ')', arg->statement, &envelope_args) <= 0) { return 0; }

	dynamic_wlist_t *args = tokenize_wr_delim(envelope_args, ",");
	sa_free(envelope_args);

	float parms[ENV_NUM_PARMS];
	
	if (args->num_items < 6) {
		SGEN_WARNING("expected exactly %d (ie. Amplitude [0;1], Attack, Decay, Sustain, Sustain Level on a scale of [0;1] & Release) numeric arguments, got %d\n", ENV_NUM_PARMS, (int)args->num_items);
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

	s->envelopes[s->num_envelopes-1] = *e;

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
{ "envelope", envelope_action } };


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


static int read_track(expression_t *track_expr, track_t *t, song_t *s) {

	// default vals
	t->notes_per_beat = 1;
	t->channel = 0;
	t->loop = 0;
	t->active = 1;
	t->transpose = 0;
	t->reverse = 0;
	t->inverse = 0;
	t->equal_temperament_steps = 12;
	t->sound = sounds[0];	// default, see waveforms.c
	t->envelope = &default_envelope;
	t->envelope_mode = ENV_FIXED;

	char* track_args;
	if (find_stuff_between('(', ')', track_expr->statement, &track_args) <= 0) { return 0; }

	dynamic_wlist_t *args = tokenize_wr_delim(track_args, ",");

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
				if (!ta->action(val, t, s)) return 0;
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
	
	char* track_contents;
	if (!find_stuff_between('{', '}', track_expr->statement, &track_contents)) {
		return 0;
	}

	dynamic_wlist_t *notelist_dirty = tokenize_wr_delim(track_contents, ",");

	dynamic_wlist_t *notelist = dynamic_wlist_tidy(notelist_dirty);
	dynamic_wlist_destroy(notelist_dirty);

	t->notes = malloc(notelist->num_items * sizeof(note_t));
	t->num_notes = notelist->num_items;
	long index = 0;

	t->note_dur_s = (1.0/(t->notes_per_beat*(s->tempo_bpm/60.0)));
	t->duration_s = t->num_notes * t->note_dur_s;
	printf("t.note_dur_s: %f, t.duration_s = %f\n", t->note_dur_s, t->duration_s);


	for (int i = 0; i < notelist->num_items; ++i) {
		char *iter = notelist->items[i];
		char *note_conts;
		int ret = -1;
		char *end_ptr;

		if ((ret = find_stuff_between('<', '>', iter, &note_conts)) < 0) {
			// syntax error
			SGEN_ERROR("track %s: syntax error in note \"%s\"!", t->name, note_conts);
			return 0;
		}
		else if (ret == 0) {
			t->notes[index].values = malloc(sizeof(int));
			t->notes[index].values[0] = (int)strtol(iter, &end_ptr, 10);
			t->notes[index].num_values = 1;
			t->notes[index].duration_s = t->note_dur_s;
			
		} else {
			dynamic_wlist_t *notes = tokenize_wr_delim(note_conts, " \t");
			t->notes[index].values = malloc(notes->num_items*sizeof(int));
			for (int i = 0; i < notes->num_items; ++i) {
				t->notes[index].values[i] = (int)strtol(notes->items[i], &end_ptr, 10);
			}
			t->notes[index].num_values = notes->num_items;
			t->notes[index].duration_s = t->note_dur_s;
			dynamic_wlist_destroy(notes);
		}
		
		if (t->envelope_mode == ENV_RANDOM_PER_NOTE) {
			t->notes[index].env = malloc(sizeof(envelope_t));
			*t->notes[index].env = random_envelope();
		} else {
			t->notes[index].env = t->envelope;
		}

		t->notes[index].transpose = t->transpose;
//		fprintf(stderr, "note str: \"%s\", num_values = %ld\n", iter, t->notes[index].num_values);

		++index;
	}

	printf("\n");

	return 1;

}

int construct_song_struct(input_t *input, song_t *s) {
	// find song block

	expression_t *exprs = input->exprs;

	int song_found = 0;
	for (int i = 0; i < input->num_active_exprs; ++i) {
		expression_t *e = &exprs[i];
		if (strcmp(e->wlist->items[0], "song") == 0) {
			song_found = 1;
			char* tracks;
			if (!find_stuff_between('{', '}', e->statement, &tracks)) return 0;
			s->active_track_ids = tokenize_wr_delim_tidy(tracks, ",");

			printf("found song block with tracks: ");
			dynamic_wlist_print(s->active_track_ids);
		}
	}
	if (!song_found) { 
		printf("sgen: no song(){...} blocks found -> no input -> no output -> exiting. (Syntax error in file?)\n"); 
		return 0; 
	}

	// defaults
	s->tempo_bpm = 120;
	s->tracks_constructed = 0;
	s->duration_s = 10; // seconds
	s->num_envelopes = 0;

	s->tracks = malloc(s->active_track_ids->num_items * sizeof(track_t));
	s->num_tracks = s->active_track_ids->num_items;

	for (int i = 0; i < input->num_active_exprs; ++i) {
		expression_t *expriter = &input->exprs[i];

		char *w = expriter->wlist->items[0];
		if (w[0] == '#') continue; // ignore comments

		int unknown = 1;
		for (int i = 0; i < sizeof(keyword_action_pairs)/sizeof(keyword_action_pairs[0]); ++i) {
			if (strcmp(w, keyword_action_pairs[i].keyword) == 0){
				if (!keyword_action_pairs[i].action(expriter, s)) { return 0; }
				unknown = 0;
				break;
			}
		}
		if (unknown) {
			printf("sgen: warning: unknown keyword \"%s\"!\n", w);
		}

	}

	int have_undefined = 0;

	for (int i = 0; i < s->active_track_ids->num_items; ++i) {
		int match = 0;
		char *tname = s->active_track_ids->items[i];
		for (int j = 0; j < s->num_tracks; ++j) {
			char *stname = s->tracks[j].name;
			if (strcmp(tname, stname) == 0) {
				match = 1;
				break;
			}
		}
		if (!match) {
			++have_undefined;
			SGEN_ERROR(" use of undefined track \"%s\"!\n", tname);
			//break;
		}
	}

	if (have_undefined) { 
		SGEN_ERROR("(fatal) undefined tracks used, exiting.\n");
		return 0;
	}

	return 1;
}

static int sgen_dump(output_t *output) {
	// defaulting to S16_LE
	
	static const char *outfname = "output_test.wav";
	
	sgen_float32_buffer_t b = output->float32_buffer;	// short

	long total_num_samples = output->channels*b.num_samples_per_channel;

	size_t outbuf_size = total_num_samples*sizeof(short);
	short *out_buffer = malloc(outbuf_size);
	short short_max = 0x7FFF;

	for (long i = 0; i < total_num_samples; ++i) {
		out_buffer[i] = (short)(0.5*short_max*(b.buffer[i]));
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

static int track_synthesize(track_t *t, long num_samples, float *lbuf, float *rbuf) {
	printf("sgen: DEBUG: synthesizing track %s\n", t->name);
	printf("props: loop = %d, active = %d, transpose = %d, channel = %d, notes_per_beat = %f\n", t->loop, t->active, t->transpose, t->channel, t->notes_per_beat);
	printf("num_samples = %ld\n", num_samples);
	if (!t->active) { return 0; }
	
	long num_samples_per_note = ceil(44100*t->note_dur_s);
	printf("note_dur: %f, num_samples_per_note = %ld\n\n", t->note_dur_s, num_samples_per_note);

	long lbuf_offset = 0;
	long rbuf_offset = 0;

	int note_index = 0;


	while (lbuf_offset < num_samples && rbuf_offset < num_samples) {
		if (note_index >= t->num_notes) {
			if (!t->loop) { break; }
			else { note_index = 0; }
		}

		note_t *n = &t->notes[note_index];
		float *nbuf = note_synthesize(n, t->sound.wform);

		long n_offset_l = lbuf_offset;
		long n_offset_r = rbuf_offset;

		if (t->channel & 0x1) {
			long i = 0;
			while (i < num_samples_per_note && n_offset_l < num_samples) {
				lbuf[n_offset_l] += nbuf[i];
				++i;
				++n_offset_l;
			}
		}

		if (t->channel & 0x2) {
			long i = 0;
			while (i < num_samples_per_note && n_offset_r < num_samples) {
				rbuf[n_offset_r] += nbuf[i];
				++i;
				++n_offset_r;
			}
		}

		lbuf_offset += num_samples_per_note;
		rbuf_offset += num_samples_per_note;

		free(nbuf);

		++note_index;
	}
	return 1;
}

static int song_synthesize(output_t *o, song_t *s) {

	long num_samples_per_channel = s->duration_s*o->samplerate;

	float *lbuf = malloc(num_samples_per_channel*sizeof(float));
	float *rbuf = malloc(num_samples_per_channel*sizeof(float));

	memset(lbuf, 0x0, num_samples_per_channel*sizeof(float));
	memset(rbuf, 0x0, num_samples_per_channel*sizeof(float));

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
	qsort(input->exprs, input->num_active_exprs, sizeof(*input->exprs), lexsort_expression_cmpfunc);

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

	srand(time(NULL));

	default_envelope = envelope_generate("default_envelope", 1.0, 0.1, 0.1, 5, 0.1, 2);

	char *input_filename = argv[1];
	input_t input = input_construct(input_filename);
	
	printf("sgen v. %s. Written by Esa (2014)\n", sgen_version);

	if (input.error != 0) { 
		SGEN_ERROR("(fatal) erroneous input! Exiting.\n");
		return 1;
	}

	song_t s;
	output_t o;

	// defaults
	o.samplerate = 44100;
	o.bitdepth = 16;
	o.channels = 2;
	o.format = S16_FORMAT_LE_STEREO;

	if (!construct_song_struct(&input, &s)) return 1;

	if (!song_synthesize(&o, &s)) return 1;
	sgen_dump(&o);

	return 0;
}

