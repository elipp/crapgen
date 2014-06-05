#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "waveforms.h"
#include "types.h"
#include "string_allocator.h"
#include "string_manip.h"
#include "dynamic_wlist.h"

static int read_track(expression_t *track_expr, track_t *t, song_t *s);
static int get_trackname(expression_t *track_expr, track_t *t);

int convert_string_to_double(const char* str, double *out) {

	char *endptr;
	*out = strtod(str, &endptr);		

	if (endptr && *endptr != '\0') {
		fprintf(stderr, "strtod warning: full double conversion of string \"%s\" couldn't be performed\n", str);
		return 0;
	}
	
	return 1;
}
	
typedef int (*keywordfunc)(expression_t*, song_t*);

int song_action(expression_t *arg, song_t *s) { return 1; } //dummy

int track_action(expression_t *arg, song_t *s) { 

	track_t t;
	if (!get_trackname(arg, &t)) { return 0; }
	int active = 0;
	for (int i = 0; i < s->active_track_ids->num_items; ++i) {
		if (strcmp(s->active_track_ids->items[i], t.name) == 0) {
			active = 1;
		}
	}
	if (!active) {
		printf("sgen: warning: track \"%s\" defined but not used in a song block!\n", t.name);
		return 0;
	}

	if (!read_track(arg, &t, s)) { fprintf(stderr, "read_track failure. syntax error(?)\n"); return 0; }

	s->tracks[s->tracks_constructed] = t;
	++s->tracks_constructed;

	return 1;
}

int sample_action(expression_t *arg, song_t *s) { printf("sgen: sample: NYI! :/\n"); return 1; }
int version_action(expression_t *arg, song_t *s) { return 1; }
int tempo_action(expression_t *arg, song_t *s) { 
	if (arg->wlist->num_items < 2) {
		fprintf(stderr, "sgen: error while parsing tempo directive. Defaulting to 120.\n");
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
		fprintf(stderr, "sgen: error while parsing duration directive. Defaulting to 10 seconds.\n");
		return 0;
	}
	char *endptr = NULL;
	char *val = arg->wlist->items[1];
	s->duration_s = strtod(val, &endptr);
	printf("sgen: found duration directive: \"%s\" s.\n", val);
	return 1; 
	
}
int samplerate_action(expression_t *arg, song_t *s) { return 1; }

static const struct { 
	const char* keyword; 
	keywordfunc action; 
} keyword_action_pairs[] = { 

{ "song", song_action}, 
{ "track", track_action }, 
{ "sample", sample_action },
{ "version", version_action },
{ "tempo", tempo_action },
{ "duration", duration_action },
{ "samplerate", samplerate_action }};

static inline int eswap_s32(int n) {
	return ((n>>24)&0x000000FF) |
		((n>>8)&0x0000FF00) |
		((n<<8)&0x00FF0000) |
		((n<<24)&0xFF000000);
}

static inline short eswap_s16(short n) {
	return (n>>8) | (n<<8);
}

WAV_hdr_t generate_WAV_header(output_t *o) {

	WAV_hdr_t hdr;

	hdr.ChunkID_BE = eswap_s32(0x52494646); // "RIFF"
	int bytes_per_sample = o->bitdepth/8;
	hdr.Subchunk2Size_LE = o->float32_buffer.num_samples_per_channel*o->channels*bytes_per_sample;

	hdr.ChunkSize_LE = 36 + hdr.Subchunk2Size_LE;
	hdr.Format_BE = eswap_s32(0x57415645); // "WAVE"
	hdr.Subchunk1ID_BE = eswap_s32(0x666d7420); // "fmt "
	hdr.Subchunk1Size_LE = 16; // 16 for PCM
	hdr.AudioFormat_LE = 1; // 1 for PCM
	hdr.NumChannels_LE = o->channels;
	hdr.SampleRate_LE = o->samplerate;
	hdr.ByteRate_LE = o->samplerate * o->channels * bytes_per_sample;
	hdr.BlockAlign_LE = o->channels * bytes_per_sample;
	hdr.BitsPerSample_LE = o->bitdepth;
	hdr.Subchunk2ID_BE = eswap_s32(0x64617461); // "data"
	return hdr;
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

static void find_stuff_between_errmsg(char beg, char end, int error) {
	error *= -1;
	if (error & 0x1) {
		fprintf(stderr, "find_stuff_between: error: beginning delimiter char (\'%c\') not found in input string.\n", beg);
	}
	if (error & 0x2) {
		if (error == 0x2) {
			fprintf(stderr, "find_stuff_between: error: unmatched delimiter \'%c\'! (expected a \'%c\'\n", beg, end);
		}
		else {
			fprintf(stderr, "find_stuff_between: error: ending delimiter char (\'%c\') not found in input string.\n", end);
		}
	}
	if (error & 0x4) {
		fprintf(stderr, "find_stuff_between: error: ending token (\'%c\') encountered before beginning token (\'%c\')!\n", end, beg);
	}

}

static int find_stuff_between(char beg, char end, char* input, char** output) {

	char *block_beg = strchr(input, beg); 
	char *block_end = strrchr(input, end); 

	int error = 0;

	if (!block_beg && !block_end) {
		return 0;
	}

	if (!block_beg) {
		error |= 0x1;
	}
	if (!block_end) { 
		error |= 0x2;
	}

	if (block_beg >= block_end) {
		error |= 0x4;
	}

	if (error) {
		fprintf(stderr, "find_stuff_between: erroneous input:\"\n%s\n\", delims = %c, %c. error code %x\n", input, beg, end, error);
		error *= -1;
		find_stuff_between_errmsg(beg, end, error);
		return error;
	}

	long b = block_beg - input;
	long nc = block_end - block_beg - 1;

	*output = substring(input, b+1, nc); // input[(block_beg+1)..block_end];
	return 1;

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

	char* track_args;
	if (find_stuff_between('(', ')', track_expr->statement, &track_args) <= 0) { return 0; }

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

		printf("track %s: prop \"%s\" = \"%s\"\n", t->name, prop, val);
		
		char *endptr;
		if (strcmp(prop, "beatdiv") == 0) {
			double v = 4;
			convert_string_to_double(val, &v);
			t->notes_per_beat = v;
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
			// check errno
		}
		else {
			fprintf(stderr, "read_track: warning: unknown track arg \"%s\", ignoring", prop);
		}

		sa_free(prop);
		sa_free(val);

		dynamic_wlist_destroy(s);

		++iter;
	}

	t->note_dur_s = (1.0/(t->notes_per_beat*(s->tempo_bpm/60.0)));
	t->duration_s = t->num_notes * t->note_dur_s;
	fprintf(stderr, "t.note_dur_s: %f, t.duration_s = %f\n", t->note_dur_s, t->duration_s);


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
			fprintf(stderr, "read_track: track %s: syntax error in note \"%s\"!", t->name, note_conts);
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

		t->notes[index].transpose = t->transpose;
//		fprintf(stderr, "note str: \"%s\", num_values = %ld\n", iter, t->notes[index].num_values);

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
			s->active_track_ids = tokenize_wr_delim_tidy(tracks, ",");

			printf("found song block with tracks: ");
			dynamic_wlist_print(s->active_track_ids);
		}
	}
	if (!song_found) { 
		printf("sgen: no song(){...} blocks found -> no input -> no output -> exiting.\n"); 
		return 0; 
	}

	// defaults
	s->tempo_bpm = 120;
	s->tracks_constructed = 0;
	s->duration_s = 10; // seconds


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
		if (note_index >= t->num_notes - 1) {
			if (!t->loop) { break; }
			else { note_index = 0; }
	//			fprintf(stderr, "setting note_index = 0\n");
		}

		note_t *n = &t->notes[note_index];
		float *nbuf = note_synthesize(n, &waveform_triangle); 

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

	fprintf(stderr, "sgen: song_synthesize: num_samples_per_channel = %ld\n", num_samples_per_channel);

	for (int i = 0; i < s->num_tracks; ++i) {
		track_t *t = &s->tracks[i];

		long num_samples = 0;
		if (t->loop) num_samples = s->duration_s*o->samplerate;
		else if (t->duration_s > s->duration_s) {
			fprintf(stderr, "sgen: warning: track duration is longer than song duration, truncating!\n");
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
		fprintf(stderr, "sgen: file_get_active_expressions: couldn't open file %s: %s\n", filename, strerrorbuf);
		return 0;
	}

	input->filename = sa_strdup(filename);
	input->filesize = get_filesize(fp);

	char *raw_buf = malloc(input->filesize);	// allocate this dynamically, don't use sa string pool
	fread(raw_buf, 1, input->filesize, fp);

	fclose(fp);

	dynamic_wlist_t *exprs_wlist = tokenize_wr_delim_tidy(raw_buf, ";");
	//dynamic_wlist_print(exprs_wlist);

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

int main(int argc, char* argv[]) {

	static const char* sgen_version = "0.01";

	if (argc < 2) {
		fprintf(stderr, "sgen: No input files. Exiting.\n");
		return 0;
	}

	char *input_filename = argv[1];
	input_t input = input_construct(input_filename);
	
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

