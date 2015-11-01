#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <time.h>
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
#include "actions.h"
#include "parse.h"


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

int construct_sgen_ctx(input_t *input, sgen_ctx_t *c) {

	for (int i = 0; i < input->num_active_exprs; ++i) {
		expression_t *expriter = &input->exprs[i];

		char *w = expriter->wlist->items[0];
		if (w[0] == '/' && w[1] == '/') continue; // ignore comments

		int unknown = 1;
		for (int i = 0; i < num_keyword_action_pairs; ++i) {
			if (strcmp(w, keyword_action_pairs[i].keyword) == 0){
				if (!keyword_action_pairs[i].action(expriter, c)) { return 0; }
				unknown = 0;
				break;
			}
		}
		if (unknown) {
			SGEN_ERROR("sgen: error: unknown keyword \"%s\"!\n", w); // fix weird error
			wlist_print(expriter->wlist);
			return 0;
		}

	}

	if (c->num_songs == 0) { 
		printf("sgen: no song(){...} blocks found -> no input -> no output -> exiting. (Syntax error in file?)\n"); 
		return 0; 
	}

	return 1;


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



int main(int argc, char* argv[]) {

	static const char* sgen_version = "0.01";

	if (argc < 2) {
		fprintf(stderr,"sgen: No input files. Exiting.\n");
		return 0;
	}

	fprintf(stderr, "sgen-%s-perse. Written by Esa (2014).\n", sgen_version);

	srand(time(NULL));

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

