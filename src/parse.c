#include "parse.h" 

char* get_primitive_identifier(expression_t *track_expr) {
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

dynamic_wlist_t *get_primitive_args(expression_t *prim_expr) {

	char *argstr;
	if (find_stuff_between('(', ')', prim_expr->statement, &argstr) <= 0) { return NULL; }
	
	dynamic_wlist_t *wr = tokenize_wr_delim(argstr, ",");
	sa_free(argstr);

	return wr;
}


static int map_notename_to_int(const char *notestr) {
	
	if (!notestr) return -1;
	if (str_isall(notestr, &isdigit)) return str_to_int_b10(notestr); 

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



static int count_transposition(const char *notestr) {

	size_t len = strlen(notestr);
	int up = 0, down = 0;
	for (int i = 0; i < len; ++i) {
		if (notestr[i] == '\'') ++up;
		else if (notestr[i] == ',') ++down;
	}
	return (up - down)*12;

}

static int parse_value_string(const char* valuestr, float *value_out) {

	size_t len = strlen(valuestr);
	int i = len - 1, num_dots = 0;

	while (i > 0 && valuestr[i] == '.') { --i; ++num_dots; }

	float base_value = 0;
	char *value_without_dots = substring(valuestr, 0, len-num_dots);

	if (strcmp(value_without_dots, "#") == 0) {
		*value_out = 2 << (rand() % 4);
		fprintf(stderr, "got #, value_out = %f\n", *value_out);
		return 1;
	}

	int ret = convert_string_to_float(value_without_dots, &base_value); 
	// won't do a full conversion if we have dots in the end, but no biggie

	if (ret < 0) {
		return 0;
	}

	// TODO: figure out a better way to calculate this :D this sux. use a series?
	
	float accum = 1.0/base_value;
	float s = 2;

	for (i = 1; i <= num_dots; ++i) {
		accum += 1.0/(base_value*s);
		s *= 2;
	}
	
	*value_out = 1.0/accum;

	return 1;


}

static int validate_digit_note(const char *notestr) {

	double d = 0;
	int ret = convert_string_to_double(notestr, &d);

	switch(ret) {
		case -1:
			return 0; //invalid, couldn't perform even partial conversion
			break;
		case 0: {
				// the only valid non-digit characters after the digit part (as of now) are '\'' and ','
				dynamic_wlist_t *tokens = tokenize_wr_delim(notestr, "\',");
				if (tokens->num_items == 1) {
					// then there's most definitely erroneous characters
					wlist_destroy(tokens);
					return 0;
				} else {
					for (int i = 1; i < tokens->num_items; ++i) {
						if (tokens->items[i] != NULL) { // there should only be NULL tokens if it's correct
							wlist_destroy(tokens);
							return 0;
						}
					}
				}
				wlist_destroy(tokens);
			}
			break;
		default:
			return 1;
			break;
	}

	return 1;

}

static int parse_random(const char *notestr, note_t *note, track_ctx_t *ctx) {
	size_t len = strlen(notestr);
	if (len > 1) {
		if (notestr[1] == '#') {
			note->value = rand() % 40; 
			ctx->value = note->value;
		}
		else { 
			convert_string_to_float(notestr+1, &note->value);
			ctx->value = note->value;
		}
	} else {
		note->value = ctx->value;
	}

	note->pitch = rand() % 40;

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
		wlist_destroy(tokens);
		return -1;
	}


	int transp = count_transposition(basestr);	
	ctx->transpose += transp;

	// we know at this stage that the string can be (at least partially) strtod'd (because of validate_digit_note)

	float pitch;
	convert_string_to_float(basestr, &pitch);
	note->pitch = pitch + ctx->transpose;

	if (valuestr) {
		if (!parse_value_string(valuestr, &note->value)) return 0;
		// make new value the current value :P
		ctx->value = note->value;
	} else {
		note->value = ctx->value;
	}

//	fprintf(stderr, "debug: digit_note: basestr = \"%s\", valuestr = \"%s\"\n", basestr, valuestr);

	wlist_destroy(tokens);

	return 1;

}

static int parse_musicinput_note(const char *notestr, note_t *note, track_ctx_t *ctx) {

	size_t len = strlen(notestr);
	int i = 0;
	int j = len - 1;

	char *basestr = NULL;
	char *valuestr = NULL;

	while (i < len && isalpha(notestr[i])) ++i;
	while (j > 0 && (isdigit(notestr[j]) || notestr[j] == '.' || notestr[j] == '#')) --j; 

	// the interval notestr[i:j] should now contain any transposition stuff

	if (j >= len-1) {
		// then there's no value. should be taken from the previously active note's value
		note->value = ctx->value;
		valuestr = NULL;
	} 
	else {
		// these are string pooled (string_allocator.c), so free() is a no-op
		valuestr = substring(notestr, j+1, len - j); 
		if (!parse_value_string(valuestr, &note->value)) return 0;
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

	else if (notestr[0] == '#') {
		return parse_random(notestr, note, ctx);
	}

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
//				fprintf(stderr, "debug: found suitable end note: \"%s\"\n", *cur_note);
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

//	fprintf(stderr, "num_notes: %d\n", num_notes);

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




static int lexsort_expression_cmpfunc(const void* ea, const void *eb) {
	const expression_t *a = ea;
	const expression_t *b = eb;
	return strcmp(a->statement, b->statement);
}

dynamic_wlist_t *get_relevant_lines(const char* input) {

	dynamic_wlist_t *wl = wlist_create();
	char *exprbuf, *saveptr;

	char *buf = sa_strdup(input);

	for (exprbuf = strtok_r(buf, "\n", &saveptr); exprbuf != NULL; exprbuf = strtok_r(NULL, "\n", &saveptr)) {

		if (str_isall(exprbuf, &isspace)) continue;
		char *stripped = strip(exprbuf); // these two are partly redundant..

		if (strlen(stripped) >= 2) {
			if (strncmp(stripped, "//", 2) != 0) { wlist_append(wl, stripped); }
		}
		else {
			// for completeness, even though an expression of length 1 is always invalid (except ";")
			wlist_append(wl, stripped);
		}
		sa_free(stripped);
	}


	sa_free(buf);

	return wl;
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

	dynamic_wlist_t *lines = get_relevant_lines(raw_buf); 
	free(raw_buf);

	// CONSIDER: this is kinda lazy, could just join lines that don't end with ';' together
	char *relevant = wlist_join_with_delim(lines, " ");
	wlist_destroy(lines);

	dynamic_wlist_t *exprs_wlist = tokenize_wr_delim(relevant, ";");

	expression_t *exprs = malloc(exprs_wlist->num_items*sizeof(expression_t));
	for (int i = 0; i < exprs_wlist->num_items; ++i) {
		exprs[i].statement = exprs_wlist->items[i];
		exprs[i].wlist = tokenize_wr_delim(exprs[i].statement, " \t");
	}

	wlist_destroy(exprs_wlist);
	input->exprs = exprs;
	input->num_active_exprs = exprs_wlist->num_items;

	// didn't even know this existed in the c stdlib
	//qsort(input->exprs, input->num_active_exprs, sizeof(*input->exprs), lexsort_expression_cmpfunc);

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

input_t input_construct(const char* filename) {
	input_t input;
	input.error = 0;
	if (!file_get_active_expressions(filename, &input)) {
		input.error |= 0x1;
	}

	return input;
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

int read_track(expression_t *track_expr, track_t *t, sgen_ctx_t *c) {

	// default vals. TODO: replace this with something cuter
	t->bpm = c->tempo_bpm;
	t->channel = 0;
	t->loop = 0;
	t->active = 1;
	t->transpose = 0;
	t->reverse = 0;
	t->inverse = 0;
	t->eqtemp_steps = 12;	
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

		wlist_destroy(parms);

		++iter;
	}

	if (t->envelope_mode == ENV_RANDOM_PER_TRACK) {
		t->envelope = malloc(sizeof(envelope_t));
		*t->envelope = random_envelope();
	}

	wlist_destroy(args);

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
