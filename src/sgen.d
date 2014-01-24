import std.stdio;
import std.cstream;
import std.file;
import std.array;
import std.conv;

import sgen.waveforms;

struct expression {
	char[] statement;
	char[][] words;
};

char[] join_string_list_with_delim(char[][] arg, string delim) {
	char[] joint = arg[0];
	for (int i = 1; i < arg.length; ++i) {
		joint = joint ~ delim ~ arg[i];
	}
	return joint; 
	
}

static expression[] decompose_into_clean_expressions(char[][] input) {
	expression[] expressions;
	int expr_begindex = 0;
	for (int i = 0; i < input.length; ++i) {
		if (std.string.indexOf(input[i], ';') != -1) {
			auto words = input[expr_begindex..(i)] ~ std.string.chop(input[i]);
			char[] words_joint = join_string_list_with_delim(words, " ");
			expressions ~= expression(words_joint, words);
			writeln(to!string(i) ~ ": \"" ~ words_joint ~ "\"");
			expr_begindex = i+1;
		}
	}

	return expressions;
}

string[] keywords = [
	"song", "track", "sample", "version", "tempo"
];

struct articulation; //nyi
struct filter; //nyi

struct note {
	
};

struct track {
	char[] name;
	int sound_index;
	int[] notes;
	int channel;
	int npb;
	this(char[] track_expr);
};

struct song {
	track[] tracks;
	float tempo_bpm;
	this(char[] song_expr);
};

enum SGEN_FORMATS {
	S16_FORMAT_LE_MONO = 0,
	S16_FORMAT_LE_STEREO = 1
};

struct output {
	int samplerate = 44100;
	int channels = 2;
	int bitdepth = 16;
	int format = SGEN_FORMATS.S16_FORMAT_LE_STEREO;
	short[] synthesize(song s);
	int dump_to_file(short[] data);
	int dump_to_stdout(short[] data);
}

static int find_stuff_between(char beg, char end, char[] input, out char[] output) {

	long block_beg = std.string.indexOf(input, beg);
	long block_end = std.string.indexOf(input, end);

	if (block_beg == -1 || block_end == -1 || (block_beg > block_end)) {
		derr.writefln("find_stuff_between: syntax error: {}");
		return 0;
	}

	output = input[(block_beg+1)..block_end];
	return 1;
	
}

static char[][] find_comma_sep_list_contents(char[] input) {
	char[][] output;

	foreach(str; input.split(",")) {
		output ~= std.string.strip(str);
	}
	return output;
}

int read_track(expression track_expr, ref track t) {
	// get enclosing {}

	t.name = track_expr.words[1];

	char[] track_contents;
	if (!find_stuff_between('{', '}', track_expr.statement, track_contents)) {
		return 0;
	}
	
	try {
		t.notes = to!(int[])(find_comma_sep_list_contents(track_contents));
	}
	catch (ConvException c) {
		derr.writefln("read_track: track " ~ t.name ~ ": ConvException caught: syntax error: unable to cast note id to integer.!");
		return 0;
	}
	
	writeln("read_track: " ~ t.name ~ ":");
	foreach(n; t.notes) write(to!string(n) ~ " ");
	writeln("");

	return 1;

}

int construct_song_struct(expression[] expressions, out song s) {
// find song block

	int song_found = 0;
	foreach(expr; expressions) {
		if (expr.words[0] == "song") {
			song_found = 1;
			char[] tracks;
			if (!find_stuff_between('{', '}', expr.statement, tracks)) return 0;
			auto tlist = find_comma_sep_list_contents(tracks);
			writeln("found song block with tracks: ");
			foreach(t; tlist) writeln(t);
		}
	}
	if (!song_found) { writeln("sgen: no song{...} block found -> no input -> no output -> exiting."); return 0; }

	foreach(expr; expressions) {
		auto w = expr.words[0];
		if (w == "track") {
			track t;
			if (!read_track(expr, t)) { derr.writefln("read_track failure. syntax error(?)"); }
			s.tracks ~= t;
		}
		else if (w == "tempo") {
			s.tempo_bpm = to!float(std.string.chop(expr.words[1]));
		}
		else { 
			derr.writefln("sgen: warning: unknown keyword \"" ~ w ~ "\"!");
//			return 0;
		}
	}

	return 1;
}

static float pitch_from_index(int index) {
	static const float twelve_equal_coeff = 1.05946309436; // 2^(1/12)
	static const float low_c = 32.7032;
	float pitch = (low_c * std.math.pow(twelve_equal_coeff, index));
	return pitch;
}

static int render_into_output_format(output o, float[] buffer) {
	// defaulting to S16_LE
	short[] out_buffer = new short[buffer.length];
	short short_max = 0x7FFF;
	for (long i = 0; i < buffer.length; ++i) {
		out_buffer[i] = cast(short)(0.5*short_max*(buffer[i]));
	}

	std.file.write("output_test.wavnoheader", out_buffer);

	writeln("out_buffer.length = " ~ to!string(out_buffer.length) ~ ".");

	return 1;

}

static int synthesize(ref song s) {
	float[] buffer = new float[2*10*44100];
	
	for (int i = 0; i < buffer.length; ++i) { buffer[i] = 0; }

	foreach(t; s.tracks) {
		t.npb = 16;
		float note_dur = (1.0/(t.npb*(s.tempo_bpm/60)));
		writefln("note_dur = " ~ to!string(note_dur) ~ " s.");
		long buffer_offset = 0;
		foreach(n; t.notes) {
			float pitch_hz = pitch_from_index((2*n));
			envelope e;
			float[] nbuf = note_synthesize(pitch_hz, note_dur, e, &waveform_sine);
			for (int i = 0; i < nbuf.length && buffer_offset < buffer.length; ++i) {
				buffer[buffer_offset] += nbuf[i];
				++buffer_offset;
			}
			if (buffer_offset >= buffer.length) { break; }
		}
	}

	output o;
	render_into_output_format(o, buffer);

	return 1;
}

int main() {
	static const char[] sgen_version = "0.01";

	string test_filename = "input_test.sg";
	auto file_contents = cast(char[])read(test_filename);
	printf("read %d bytes\n", file_contents.length);

	auto words = file_contents.split();

	printf("%d words found\n", words.length);

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
