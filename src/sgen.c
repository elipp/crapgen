#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "waveforms.h"
#include "types.h"
#include "string_allocator.h"

static char *strip(const char*);
static char *tidy_string(const char*);
static char *substring(const char* str, int beg_pos, int nc);
char *join_wlist_with_delim(dynamic_wlist_t *wl, const char *delim);
static dynamic_wlist_t *tokenize_wr_delim(const char* input, const char* delim);
void dynamic_wlist_print(dynamic_wlist_t *wl);
dynamic_wlist_t *dynamic_wlist_tidy(dynamic_wlist_t *wl);
int dynamic_wlist_append(dynamic_wlist_t* wlist, const char* word);

static char *substring(const char* str, int beg_pos, int nc) {

	if (!str) { return NULL; }
	size_t str_len = strlen(str);
	if (!str_len) { return NULL; }

	if (str_len < beg_pos + nc) {
		nc = str_len - beg_pos;
	}

	char *sub = sa_alloc(nc+1);

	strncpy(sub, str+beg_pos, nc);

	sub[nc] = '\0';

	return sub;
}


static char *strip(const char* input) {

	if (!input) { return NULL; }
	size_t str_len = strlen(input);

	if (str_len <= 0) { return NULL; }

	int beg = 0;
	while (beg < str_len) {
		if (input[beg] == ' ' || input[beg] == '\t' || input[beg] == '\n') ++beg;
		else break;
	}

	int end = str_len-1;
	while (end > 0) {
		if (input[end] == ' ' || input[end] == '\t' || input[end] == '\n') --end;
		else break;
	}
	
	long len = end - beg + 1;
	if (len < 1) {
		fprintf(stderr, "sgen: strip: input string len < 1!\n");

		return NULL;
	}

	char *r = substring(input, beg, len);
	return r;
}

static char *tidy_string(const char* input) {
	
	char *tidy = strip(input);
	if (!tidy) { return NULL; }

	char *chrpos = strchr(tidy, '\n');
	while (chrpos != NULL) {
		*chrpos = ' ';
		chrpos = strchr(chrpos+1, '\n');
	}

	dynamic_wlist_t *t = tokenize_wr_delim(tidy, " \t");
	char *r = join_wlist_with_delim(t, " ");	// this should get rid of all duplicate whitespace (two or more consecutive)

	sa_free(tidy);
	return r;

}


char *join_wlist_with_delim(dynamic_wlist_t *wl, const char *delim) {
	size_t total_length = 0;

	for (long i = 0; i < wl->num_items; ++i) {
		char *iter = wl->items[i];
		total_length += strlen(iter);
	}

	size_t joint_size = total_length + wl->num_items + 2;
	char* joint = sa_alloc(joint_size);
	memset(joint, 0x0, joint_size);

	for (int i = 0; i < wl->num_items-1; ++i) {
		char *iter = wl->items[i];
		strcat(joint, iter);
		strcat(joint, delim);
	}
	strcat(joint, wl->items[wl->num_items-1]);
	joint[joint_size - 1] = '\0';

//	dynamic_wlist_print(wl);
//	fprintf(stderr, "joint: \"%s\"\n", joint);

	return joint; 

}

static const char* keywords[] = 
{ "song", "track", "sample", "version", "tempo" };

dynamic_wlist_t *dynamic_wlist_create() {
	dynamic_wlist_t *wl = malloc(sizeof(dynamic_wlist_t));
	wl->num_items = 0;
	wl->capacity = 8;
	wl->items = malloc(wl->capacity*sizeof(char*)); 
	memset(wl->items, 0x0, wl->capacity*sizeof(char*));

	return wl;
}

void dynamic_wlist_destroy(dynamic_wlist_t *wl) {
	for (int i = 0; i < wl->num_items; ++i) {
		sa_free(wl->items[i]);
	}
	free(wl->items);
	free(wl);
}

dynamic_wlist_t *dynamic_wlist_tidy(dynamic_wlist_t *wl) {
	dynamic_wlist_t *t = dynamic_wlist_create();
	for (int i = 0; i < wl->num_items; ++i) {
		dynamic_wlist_append(t, tidy_string(wl->items[i]));
	}
	return t;
}

void dynamic_wlist_print(dynamic_wlist_t *wl) {
	printf("wlist contents:\n");
	for (int i = 0; i < wl->num_items; ++i) {
		printf(" \"%s\"\n", wl->items[i]);
	}
	printf("------------------------\n");
}

int dynamic_wlist_append(dynamic_wlist_t* wlist, const char* word) {

	if (!word) return 0; 

	++wlist->num_items;
	if (wlist->num_items > wlist->capacity) {
		wlist->capacity *= 2;
		wlist->items = realloc(wlist->items, wlist->capacity*sizeof(char*));
		if (!wlist->items) { return 0; }
	}

	wlist->items[wlist->num_items-1] = sa_strdup(word);

	return 1;
}

static int find_stuff_between(char beg, char end, char* input, char** output) {

	char *block_beg = strchr(input, beg); 
	char *block_end = strrchr(input, end); 

	if (!block_beg || !block_end) { 
//		fprintf(stderr, "find_stuff_between: error: token not found. \ninput: \"%s\", delims: beg: %c, end: %c\n", input, beg, end);
		return 0;
	}

	else if (block_beg >= block_end) {
		fprintf(stderr, "find_stuff_between: syntax error: beg = %c, end = %c\n", beg, end);
		return -1;
	}

	long b = block_beg - input;
	long nc = block_end - block_beg - 1;

	*output = substring(input, b+1, nc); // input[(block_beg+1)..block_end];
	return 1;

}

static dynamic_wlist_t *tokenize_wr_delim(const char* input, const char* delim) {
	dynamic_wlist_t *wl = dynamic_wlist_create();
	char *exprbuf; //, *saveptr;

	char *buf = sa_strdup(input);

	for (exprbuf = strtok(buf, delim); exprbuf != NULL; exprbuf = strtok(NULL, delim)) {
		dynamic_wlist_append(wl, exprbuf);
	}

	sa_free(buf);

	return wl;
}

static int get_trackname(expression_t *track_expr, track_t *t) {
	char* index = NULL;

	char *w = track_expr->wlist->items[1];
	if ((index = strchr(w, '(')) != NULL) {
		t->name = tidy_string(substring(w, 0, index - w));
		return 1;
	}
	else {
		fprintf(stderr, "read_track: syntax error: must use argument list (\"()\", even if empty) before track contents (\"{}\").");
		return 0;
	}


}

int read_track(expression_t *track_expr, track_t *t) {

	// default vals
	t->npb = 4;
	t->channel = 0;
	t->loop = 0;
	t->active = 1;
	t->transpose = 0;

	char* track_args;
	if (!find_stuff_between('(', ')', track_expr->statement, &track_args)) { return 0; }

	dynamic_wlist_t *args = tokenize_wr_delim(track_args, ",");

	for (int i = 0; i < args->num_items; ++i) {
		
		char *iter = args->items[i];

		dynamic_wlist_t *s = tokenize_wr_delim(iter, "=");
		if (s->num_items < 2) { fprintf(stderr, "read_track: reading arg list failed; split(\"=\").length < 2!\""); continue; }
		char *prop = tidy_string(s->items[0]);
		char *val = tidy_string(s->items[1]);

		if (!prop || !val) { 
			fprintf(stderr, "read_track: error while parsing arg list (fmt: <prop>=<val>, got !prop || !val)\n"); 
			continue; 
		}

		printf("track \"%s\": prop \"%s\" = \"%s\"\n", t->name, prop, val);
		
		char *endptr;
		if (strcmp(prop, "beatdiv") == 0) {
			t->npb = strtod(val, &endptr);		
			if (endptr && *endptr != '\0') {
				fprintf(stderr, "strtod warning: full double conversion of string \"%s\" couldn't be performed\n", val);
			}
		}
		else if (strcmp(prop, "channel") == 0) {
			// channel = to!int(s[1]);		
			if (strcmp(val, "left") == 0 || strcmp(val, "l") == 0) {
				t->channel |= 0x1;
			}
			else if (strcmp(val, "right") == 0 || strcmp(val, "r") == 0) {
				t->channel |= 0x2;
			}
			else {
				fprintf(stderr, "read_track: args: unrecognized channel option \"%s\". defaulting to stereo (l+r)", val);
			}
		}
		else if (strcmp(prop, "sound") == 0) {
			//			sound = to!int(s[1]);
		}
		else if (strcmp(prop, "reverse") == 0) {
			// nyi
		}
		else if (strcmp(prop, "inverse") == 0) {
			// nyi
		}
		else if (strcmp(prop, "equal_temperament_steps") == 0) {
			// enables non-standard scales to be used
			// nyi
		}
		else if (strcmp(prop, "loop") == 0) {
			if (strcmp(val, "true") == 0) t->loop = 1;
		}

		else if (strcmp(prop, "active") == 0) {
			if (strcmp(val, "false") == 0 || strcmp(val, "0") == 0) t->active = 0;
		}

		else if (strcmp(prop, "transpose") == 0)  {
			t->transpose = strtod(val, &endptr);
		}
		else {
			fprintf(stderr, "read_track: warning: unknown track arg \"%s\", ignoring", prop);
		}

		sa_free(prop);
		sa_free(val);

		dynamic_wlist_destroy(s);

		++iter;
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

	for (int i = 0; i < notelist->num_items; ++i) {
		char *iter = notelist->items[i];
		char *note_conts;
		int ret = -1;
		char *end_ptr;

		if ((ret = find_stuff_between('<', '>', iter, &note_conts)) < 0) {
			// syntax error
			fprintf(stderr, "read_track: track \"%s\": syntax error in note \"%s\"!", t->name, note_conts);
			return 0;
		}
		else if (ret == 0) {
			t->notes[index].values = malloc(sizeof(int));
			t->notes[index].values[0] = (int)strtol(iter, &end_ptr, 10);
			t->notes[index].num_values = 1;
			
		} else {
			dynamic_wlist_t *notes = tokenize_wr_delim(note_conts, " \t");
			t->notes[index].values = malloc(notes->num_items*sizeof(int));
			for (int i = 0; i < notes->num_items; ++i) {
				t->notes[index].values[i] = (int)strtol(notes->items[i], &end_ptr, 10);
			}
			t->notes[index].num_values = notes->num_items;
			dynamic_wlist_destroy(notes);
		}

		t->notes[index].transpose = t->transpose;
		fprintf(stderr, "note str: \"%s\", num_values = %ld\n", iter, t->notes[index].num_values);

		++index;
	}

//	printf("read_track: %s", t->name);

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
			dynamic_wlist_t *t = tokenize_wr_delim(tracks, ",");
			s->active_track_ids = dynamic_wlist_tidy(t); 
			dynamic_wlist_destroy(t);
			printf("found song block with tracks: ");
			dynamic_wlist_print(s->active_track_ids);
		}
	}
	if (!song_found) { 
		printf("sgen: no song(){...} blocks found -> no input -> no output -> exiting.\n"); 
		return 0; 
	}

	s->tempo_bpm = 120;

	char *strtol_endptr = NULL;

	s->tracks = malloc(s->active_track_ids->num_items * sizeof(track_t));
	s->num_tracks = s->active_track_ids->num_items;

	long tracknum = 0;
	for (int i = 0; i < input->num_active_exprs; ++i) {
		expression_t *expriter = &input->exprs[i];

		char *w = expriter->wlist->items[0];
		if (w[0] == '#') continue; // ignore comments

		if (strcmp(w, "track") == 0) {
			track_t t;
			if (!get_trackname(expriter, &t)) { return 0; }
			int active = 0;
			for (int i = 0; i < s->active_track_ids->num_items; ++i) {
				if (strcmp(s->active_track_ids->items[i], t.name) == 0) {
					active = 1;
				}
			}
			if (!active) {
				printf("sgen: warning: track \"%s\" defined but not used in a song block!\n", t.name);
				continue;
			}

			if (!read_track(expriter, &t)) { fprintf(stderr, "read_track failure. syntax error(?)"); }
			
			s->tracks[tracknum] = t;
			++tracknum;
		}
		else if (strcmp(w, "tempo") == 0) {
			if (expriter->wlist->num_items < 2) {
				fprintf(stderr, "sgen: error while parsing tempo directive. Defaulting to 120.\n");

			}
			char *val = expriter->wlist->items[1];
			s->tempo_bpm = strtol(val, &strtol_endptr, 10);
			fprintf(stderr, "found tempo directive: \"%s\" bpm.\n", val);
		}
		else if (strcmp(w, "song") == 0) {
			// do nuthin, has already been done
		}
		else { 
			printf("sgen: warning: unknown keyword \"%s\"!\n", w);
			//			return 0;
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
			fprintf(stderr, "sgen: song: error: use of undefined track \"%s\"!\n", tname);
			//break;
		}
	}

	if (have_undefined) { 
		fprintf(stderr, "sgen: construct_song_struct: undefined tracks used.\n");
		return 0;
	}

	return 1;
}

static int sgen_dump(sgen_float32_buffer_t *fb, output_t *output) {
	// defaulting to S16_LE
	
	static const char *outfname = "output_test.wavnoheader";

	size_t outbuf_size = fb->num_samples*sizeof(short);
	short *out_buffer = malloc(outbuf_size);
	short short_max = 0x7FFF;

	for (long i = 0; i < fb->num_samples; ++i) {
		out_buffer[i] = (short)(0.5*short_max*(fb->buffer[i]));
	}

	printf("sgen: dumping buffer of %ld samples to file %s.\n", fb->num_samples, outfname);

	FILE *ofp = fopen(outfname, "w"); // "w" = truncate
	if (!ofp) return 0;

	fwrite(out_buffer, sizeof(short), fb->num_samples, ofp);
	free(out_buffer);

	fclose(ofp);
	return 1;

}

static int track_synthesize(track_t *t, float bpm, float *lbuf, float *rbuf) {
	printf("DEBUG: sgen: synthesizing track \"%s\"\n", t->name);
	printf("props: loop = %d, active = %d, transpose = %d, channel = %d, npb = %f\n", t->loop, t->active, t->transpose, t->channel, t->npb);
	if (!t->active) { return 0; }

	const long num_samples = 10*44100;

	float note_dur = (1.0/(t->npb*(bpm/60)));

	long num_samples_per_note = ceil(44100*note_dur);
	printf("note_dur: %f, num_samples_per_note = %ld\n", note_dur, num_samples_per_note);

	long lbuf_offset = 0;
	long rbuf_offset = 0;

	int note_index = 0;
	while (lbuf_offset < num_samples && rbuf_offset < num_samples) {
		if (note_index >= t->num_notes - 1) {
			if (!t->loop) { break; }
			else { note_index = 0; }
	//			fprintf(stderr, "setting note_index = 0\n");
		}

		note_t *n = &t->notes[note_index];
		envelope_t e;
		float *nbuf = note_synthesize(n, note_dur, e, &waveform_triangle); 
		// TODO: replace the gratuitous malloc in note_synthesize with a shared buffer, since the size is always the same

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

static sgen_float32_buffer_t song_synthesize(output_t *o, song_t *s) {

	const long num_samples = 10*44100; // TODO: get real length from song_t

	float *lbuf = malloc(num_samples*sizeof(float));
	float *rbuf = malloc(num_samples*sizeof(float));

	memset(lbuf, 0x0, num_samples*sizeof(float));
	memset(rbuf, 0x0, num_samples*sizeof(float));

	for (int i = 0; i < s->num_tracks; ++i) {
		track_t *t = &s->tracks[i];
		track_synthesize(t, s->tempo_bpm, lbuf, rbuf);
	}

	float *merged = malloc(2*num_samples*sizeof(float));
	for (long i = 0; i < num_samples; ++i) {
		merged[2*i] = lbuf[i];
		merged[2*i+1] = rbuf[i];

	//	fprintf(stderr, "%f, %f\n", merged[2*i], merged[2*i + 1]);
	}

	free(lbuf); 
	free(rbuf);
	
	sgen_float32_buffer_t f;
	f.buffer = merged;
	f.num_samples = num_samples;

	return f;
}

static long get_filesize(FILE *fp) {
	long size = 0;
	long prevoff = ftell(fp);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, prevoff, SEEK_SET);
	return size;
}

int file_get_active_expressions(const char* filename, input_t *input) {

	FILE *fp = fopen(filename, "r");

	if (!fp) {
		char *strerrorbuf = strerror(errno);
		fprintf(stderr, "sgen: file_get_active_expressions: couldn't open file %s: %s", filename, strerrorbuf);
		return 0;
	}

	input->filename = sa_strdup(filename);
	input->filesize = get_filesize(fp);

	char *raw_buf = malloc(input->filesize);	// allocate this dynamically, don't use sa string pool
	fread(raw_buf, input->filesize, 1, fp);

	fclose(fp);

	//dynamic_wlist_t *exprs_wlist = dynamic_wlist_create();
	dynamic_wlist_t *exprs_dirty = tokenize_wr_delim(raw_buf, ";");
	dynamic_wlist_t *exprs_wlist = dynamic_wlist_tidy(exprs_dirty);
	dynamic_wlist_destroy(exprs_dirty);

/*
	char *exprbuf = NULL;
	char *saveptr = NULL;

	for (exprbuf = strtok(raw_buf, ";"); exprbuf != NULL; exprbuf = strtok(NULL, ";")) {
		char *tidy = tidy_string(exprbuf);
		if (tidy && tidy[0] != '#') { 
			dynamic_wlist_append(exprs_wlist, tidy);		
		} else {
			if (!tidy) fprintf(stderr, "tidy = NULL!\n");
		}
		sa_free(tidy);
	}
	*/



	dynamic_wlist_print(exprs_wlist);

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

int main() {

	static const char* sgen_version = "0.01";

	const char* test_filename = "input_test.sg";
	input_t input = input_construct(test_filename);
	
	if (input.error != 0) { 
		fprintf(stderr, "sgen: fatal: erroneous input! exiting.\n");
		return 1;
	}

	if (strcmp(input.exprs[0].wlist->items[0], "version") != 0) {
		fprintf(stderr, "sgen: error: missing version directive from the beginning!");
		return 1;
	} else {
		char *file_version = input.exprs[0].wlist->items[1];
		if (strcmp(file_version, sgen_version) != 0) {
			fprintf(stderr, "sgen: fatal: compiler/input file version mismatch! (compiler: \"%s\", file \"%s\")\n", sgen_version, file_version);
			return 1;
		}
	}

	song_t s;
	output_t output;
	if (!construct_song_struct(&input, &s)) return 1;

	sgen_float32_buffer_t b = song_synthesize(&output, &s);

	sgen_dump(&b, &output);

	return 0;
}

