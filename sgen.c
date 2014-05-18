#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

//#include sgen.waveforms;

struct dynamic_wlist_t {
	char **words;
	long num_words;
	long capacity;
};

struct expression_t {
	char* statement;
	dynamic_wlist_t *wlist;
};


char *join_wlist_with_delim(dynamic_wlist_t *wl, char *delim) {
	size_t total_length = 0;

	for (long i = 0; i < wl->num_words; ++i) {
		char *iter = wl->words[i];
		total_length += strlen(iter);
		++num_words;
	}

	size_t joint_size = total_length + wl->num_words + 1;
	char* joint = malloc(joint_size);
	memset(joint, 0x0, joint_size);

	for (int i = 0; i < num_words; ++i) {
		char *iter = wl->words[i];
		strcat(joint, iter);
		strcat(joint, delim);
	}
	return joint; 

}

static const char* keywords[] = 
{ "song", "track", "sample", "version", "tempo" };

struct articulation; //nyi
struct filter_t; //nyi

struct note_t {
	int *values;
	long num_values;
};

struct track_t {
	char* name;
	int sound_index;
	bool loop;
	note_t *notes;
	int active;
	int transpose;
	int channel; // 0x1 == left, 0x2 == right, 0x3 == both
	float npb;
//	this(char[] track_expr);
};

struct song_t {
	track_t* tracks;
	int num_tracks;
	dynamic_wlist_t *active_tracks;
	float tempo_bpm;
//	this(char[] song_expr);
};

enum SGEN_FORMATS {
	S16_FORMAT_LE_MONO = 0,
	S16_FORMAT_LE_STEREO = 1
};

struct output_t {
	int samplerate;// = 44100;
	int channels;//  = 2;
	int bitdepth;// = 16;
	int format;// = SGEN_FORMATS.S16_FORMAT_LE_STEREO;
	int dump_to_file(short* data);
	int dump_to_stdout(short* data);
}

short* synthesize(output_t* output, song_t* s);

struct dynamic_wlist_t *dynamic_wlist_create() {
	struct dynamic_wlist_t *wl = malloc(sizeof(dynamic_wlist_t));
	wl->num_words = 0;
	wl->capacity = 8;
	wl->words = malloc(wl->capacity*sizeof(char*)); 
	memset(wl->words, 0x0, wl->capacity*sizeof(char*));

	return wl;
}

struct dynamic_wlist_t *dynamic_wlist_destroy(dynamic_wlist_t *wl) {
	for (int i = 0; i < wl->num_words; ++i) {
		free(wl->words[i]);
	}
	free(wl->words);
	free(wl);
}

int dynamic_wlist_append(dynamic_wlist_t* wlist, const char* word) {

	if (!word) { return 0; }

	++wlist->num_words;
	if (wlist->num_words > wlist->capacity) {
		wlist->capacity *= 2;
		wlist->words = realloc(wlist->words, wlist->capacity*sizeof(char*));
		if (!wlist->words) { return 0; }
	}
	wlist->words[wlist->num_words-1] = strdup(word);
	return 1;
}

static int find_stuff_between(char beg, char end, char* input, char** output) {

	long block_beg = strchr(input, beg); //std.string.indexOf(input, beg);
	long block_end = strchr(input, end); //std.string.indexOf(input, end);

	if (block_beg == -1 && block_end == -1) { return 0; }
	if (block_beg == -1 || block_end == -1 || (block_beg > block_end)) {
		fprintf(stderr, "find_stuff_between: syntax error: beg = %c, end = %c\n", beg, end);
		return -1;
	}

	*output = substring(input, block_beg+1, (block_end - block_beg)); // input[(block_beg+1)..block_end];
	return 1;

}

static dynamic_wlist_t *tokenize_wr_delim(char* input, const char* delim) {
	dynamic_wlist_t *wl = dynamic_wlist_create();
	char *exprbuf, *saveptr;

	for (exprbuf = strtok_r(buf, delim, &saveptr); exprbuf != NULL; exprbuf = strtok_r(NULL, delim, &saveptr)) {
		dynamic_wlist_append(wl, tidy_string(exprbuf));
	}
	return wl;
}

static int get_trackname(expression_t *track_expr, track_t *t) {
	char* index = NULL;

	char *w = track_expr->wlist->words[1];
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
	t->loop = false;
	t->active = 1;
	t->transpose = 0;

	char* track_args;
	if (!find_stuff_between('(', ')', track_expr->statement, &track_args)) { return 0; }

	dynamic_wlist_t *args = tokenize_wr_delim(track_args, ",");

	char *iter = args->words[0];
	
	while (iter != NULL) {
		dynamic_wlist_t *s = tokenize_wr_delim(iter, "=");
		if (s->num_words < 2) { fprintf(stderr, "read_track: reading arg list failed; split(\"=\").length < 2!\""); continue; }
		char *prop = tidy_string(s->words[0]);
		char *val = tidy_string(s->words[1]);

		if (!prop || !val) { fprintf(stderr, "read_track: error while parsing arg list (fmt: <prop>=<val>, got !prop || !val)\n"); continue: }

		printf("DEBUG: track \"%s\": prop \"%s\" = \"%s\"\n", t->name, prop, val);
		
		char *endptr;
		if (strcmp(prop, "beatdiv") == 0) {
			t.npb = stdtod(val, &endptr);		
			break;
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
			if (strcmp(val, "true") == 0) t.loop = true;
		}

		else if (strcmp(prop, "active") == 0) {
			if (strcmp(val, "false") == 0 || strcmp(val, "0") == 0) t.active = 0;
		}

		else if (strcmp(prop, "transpose") == 0)  {
			t.transpose = strtod(val, &endptr);
		}
		else {
			fprintf(stderr, "read_track: warning: unknown track arg \"%s\", ignoring", prop);
		}

		free(prop);
		free(val);

		dynamic_wlist_destroy(s);

		++iter;
	}

	char* track_contents;
	if (!find_stuff_between('{', '}', track_expr->statement, &track_contents)) {
		return 0;
	}

	dynamic_wlist_t *notelist = tokenize_wr_delim(track_contents, ",");

	char *iter = notelist->words[0];

	t->notes = malloc(notelist->num_words * sizeof(note_t*));
	long index = 0;

	while (iter != NULL) {
		char *note_conts;
		int ret = -1;
		char *end_ptr;

		if ((ret = find_stuff_between('<', '>', iter, &note_conts)) < 0) {
			// syntax error
			fprintf(stderr, "read_track: track \"%s\": syntax error in note \"%s\"!", t->name, note_conts);
			return 0;
		}
		else if (ret == 0) {
			t->notes[index] = malloc(sizeof(int));
			*t->notes[index] = (int)strtol(iter, &end_ptr, 10);
			t->notes[index].size = 1;
			
		} else {
			dynamic_wlist_t *notes = tokenize_wr_delim(iter, " ");
			t->notes[index] = malloc(notes->num_words*sizeof(int));
			for (int i = 0; i < notes->num_words; ++i) {
				t->notes[index][i] = (int)strtol(iter, &end_ptr, 10);
			}
			t->notes[index].size = notes->num_words;
		}

		++iter;
		++index;
	}

//	printf("read_track: %s", t->name);

	return 1;

}

int construct_song_struct(expression_t *exprs, song *s) {
	// find song block

	int song_found = 0;
	for (int i = 0; i < exprs->words->num_words; ++i) {
		dynamic_wlist_t *witer = exprs->words[i];
		if (strcmp(witer->words[0], "song" == 0)) {
			song_found = 1;
			char* tracks;
			if (!find_stuff_between('{', '}', witer->statement, &tracks)) return 0;
			s->active_tracks = find_comma_sep_list_contents(tracks);
			printf("found song block with tracks: ");
			//foreach(t; s.active_tracks) writeln(t);
		}
	}
	if (!song_found) { printf("sgen: no song(){...} blocks found -> no input -> no output -> exiting."); return 0; }

	foreach(expr; expressions) {
		char *w = expr->wlist->words[0];
		if (strcmp(w, "track") == 0) {
			track t;
			if (!get_trackname(expr, t)) { return 0; }
			int active = 0;
			for (int i = 0; i < s->active_tracks->num_words; ++i) {
				if (strcmp(s->active_tracks->words[i], t.name) == 0) {
					active = 1;
				}
			}
			if (!active) {
				printf("sgen: warning: track \"%s\" defined but not used in a song block!", t.name);
				continue;
			}

			if (!read_track(expr, &t)) { fprintf(stderr, "read_track failure. syntax error(?)"); }
			s.tracks ~= t;
			++s->num_tracks;
		}
		else if (w == "tempo") {
			try {
				s.tempo_bpm = to!float(std.string.chop(expr.words[1]));
			}
			catch (ConvException c) {
				derr.writefln("tempo: unable to convert string \"" ~ expr.words[1] ~ "\" to a meaningful tempo value. Defaulting to 120 bpm.\n");
				s.tempo_bpm = 120;	// default tempo
			}
			writefln("found tempo directive: " ~ expr.words[1] ~ " bpm.");
		}
		else if (w == "song") {
			// do nuthin, has already been done
		}
		else { 
			derr.writefln("sgen: warning: unknown keyword \"" ~ w ~ "\"!");
			//			return 0;
		}
	}

	int have_undefined = 0;
	foreach (a; s.active_tracks) {
		int match = 0;
		foreach(t; s.tracks) {
			if (std.string.strip(t.name) == std.string.strip(a)) {
				match = 1;
				break;
			}
		}
		if (!match) {
			derr.writefln("song: error: use of undefined track \"" ~ a ~ "\".");
			return 0;
		}
	}

	return 1;
}
static float *pitch_from_noteindex(note_t* note, float transpose) {
	static const float twelve_equal_coeff = 1.05946309436; // 2^(1/12)
	static const float low_c = 32.7032;
	float *pitches = malloc(note->num_values*sizeof(float));

	for (int i = 0; i < note->num_values; ++i) {
		pitches[i] = low_c * std.math.pow(twelve_equal_coeff, note->values[i] + transpose);
	}

	return pitches;
}

static int render_into_output_format(output_t *o, float *buffer, size_t num_samples) {
	// defaulting to S16_LE
	size_t outbuf_size = num_samples*sizeof(short);
	short *out_buffer = malloc(outbuf_size);
	short short_max = 0x7FFF;
	for (long i = 0; i < num_samples; ++i) {
		out_buffer[i] = (short)(0.5*short_max*(buffer[i]));
	}

	FILE *ofp = fopen("output_test.wavnoheader", "w"); // "w" = truncate
	if (!ofp) return 0;

	fwrite(out_buffer, num_samples, sizeof(short), ofp);
	fclose(ofp);
	return 1;

}

static int synthesize(song_t *s) {
	const long num_samples = 10*44100;
	float *lbuf = malloc(num_samples*sizeof(float));
	float *rbuf = malloc(num_samples*sizeof(float))

	memset(lbuf, 0x0, num_samples*sizeof(float));
	memset(rbuf, 0x0, num_samples*sizeof(float));

	for (int i = 0; i < s->num_tracks; ++i) {
		track_t *t = s->tracks[i];
		if (!t->active) { continue; }

		float note_dur = (1.0/(t->npb*(s->tempo_bpm/60)));

		long num_samples_per_note = ceil(44100*note_dur);

		long lbuf_offset = 0;
		long rbuf_offset = 0;

		int note_index = 0;
		while (lbuf_offset < num_samples && rbuf_offset < num_samples) {
			if (note_index >= t->notes->num_values - 1) {
				if (!t.loop) { break; }
				else { note_index = 0; }
			}

			note_t *n = t->notes[node_index];
			float *pitches_hz = pitch_from_noteindex(n, t->transpose);
//			envelope_t e;
			float *nbuf = note_synthesize(pitches_hz, note_dur, e, &waveform_triangle);

			long n_offset_l = l_buffer_offset;
			long n_offset_r = r_buffer_offset;

			if (t.channel & 0x1) {
				long i = 0;
				while (i < n->num_values && n_offset_l < num_samples) {
					lbuf[n_offset_l] += nbuf[i];
					++i;
					++n_offset_l;
				}
			}

			if (t.channel & 0x2) {
				long i = 0;
				while (i < n->num_values && n_offset_r < num_samples) {
					rbuf[n_offset_r] += nbuf[i];
					++i;
					++n_offset_r;
				}
			}

			lbuf_offset += num_samples_per_note;
			rbuf_offset += num_samples_per_note;

			++note_index;
		}
	}

	float *merged = new float[2*num_samples];
	for (long i = 0; i < num_samples; ++i) {
		merged[2*i] = lbuf[i];
		merged[2*i+1] = rbuf[i];
	}

	output_t *o;
	render_into_output_format(o, merged);
/*
	auto leftfile = std.stdio.File("waveform_left.dat", "w+");
	for (long i = 0; i < left_channel_buffer.length; ++i) {
		leftfile.writefln("%d\t%f", i, left_channel_buffer[i]);
	}

	leftfile.close(); 

	auto rightfile = std.stdio.File("waveform_right.dat", "w+");
	for (long i = 0; i < right_channel_buffer.length; ++i) {
		rightfile.writefln("%d\t%f", i, right_channel_buffer[i]);
	}

	rightfile.close(); 
	*/


	return 1;
}

static long get_filesize(FILE *fp) {
	long size = 0;
	long prevoff = ftell(fp);
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, prevoff, SEEK_SET);
	return size;
}

char *substring(const char* str, int beg_pos, int nc) {

	if (!str) { return NULL; }
	size_t str_len = strlen(str);
	if (!str_len) { return NULL; }

	if (str_len < beg_pos + nc) {
		nc = str_len - beg_pos;
	}

	char *sub = malloc(nc+1);

	strncpy(sub, str+beg_pos, nc);

	sub[nc] = '\0';

	return sub;
}


static char *strip(const char* input) {

	if (!input) { return NULL; }
	size_t str_len = strlen(input);

	int beg = 0;
	for (; beg < str_len; ) {
		if (input[beg] == ' ' || input[beg] == '\t' || input[beg] == '\n') ++beg;
		else break;
	}

	int end = str_len-1;
	for (; end > 0; ) {
		if (input[end] == ' ' || input[end] == '\t' || input[end] == '\n') --end;
		else break;
	}
	
	int len = end - beg + 1;
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

	return tidy;

}

dynamic_wlist_t* read_file_filter_comments(const char* filename) {

	FILE *fp = fopen(filename, "r");

	if (!fp) {
		char *strerrorbuf = strerror(errno);
		fprintf(stderr, "sgen: read_file_filter_comments: couldn't open file %s: %s", filename, strerrorbuf);
		return NULL;
	}

	long filesize = get_filesize(fp);
	char *buf = malloc(filesize);
	fread(buf, filesize, 1, fp);

	dynamic_wlist_t *exprs_wlist = dynamic_wlist_create();

	char *exprbuf = NULL;
	char *saveptr = NULL;
	for (exprbuf = strtok_r(buf, ";", &saveptr); exprbuf != NULL; exprbuf = strtok_r(NULL, ";", &saveptr)) {
		char *tidy = tidy_string(exprbuf);
		if (tidy && tidy[0] != '#') { 
			dynamic_wlist_append(exprs_wlist, tidy);		
		}
		free(tidy);
	}

	return exprs_wlist;
}

int main() {
	static const char* sgen_version = "0.01";

	const char* test_filename = "input_test.sg";
	auto words = read_file_filter_comments(test_filename);
	auto exprs = decompose_into_clean_expressions(words);

	if (exprs[0].words[0] != "version") {
		derr.writefln("sgen: warning: missing version directive from the beginning!");
		return 1;
	} else {
		auto file_ver = exprs[0].words[1];
		if (file_ver != sgen_version) {
			derr.writefln("sgen: fatal: compiler/input file version mismatch! (" ~ sgen_version ~ " != " ~ file_ver ~ ")!"); return 1;
		}
	}

	song s;

	if (!construct_song_struct(exprs, s)) return 1;

	synthesize(s);

	return 0;
}

