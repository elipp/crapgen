import std.stdio;
import std.cstream;
import std.file;
import std.array;
import std.conv;
import std.c.string;

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
	bool loop;
	int[] notes;
	int active;
	int channel; // 0x1 == left, 0x2 == right, 0x3 == both
	float npb;
	this(char[] track_expr);
};

struct song {
	track[] tracks;
	char[][] active_tracks;
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

static int get_trackname(expression track_expr, ref track t) {
	long index = 0;
	if ((index = std.string.indexOf(track_expr.words[1], '(')) != -1) {
		t.name = std.string.strip(track_expr.words[1][0..index]);
		return 1;
	}
	else {
		derr.writefln("read_track: syntax error: must use argument list (\"()\") before track contents (\"{}\").");
		return 0;
	}


}

int read_track(expression track_expr, ref track t) {

// default vals
	t.npb = 4;
	t.channel = 0;
	t.loop = false;
	t.active = 1;

	char[] track_args;
	if (!find_stuff_between('(', ')', track_expr.statement, track_args)) { return 0; }

	auto args = find_comma_sep_list_contents(track_args);

	foreach (a; args) {
		auto s = a.split("=");
		if (s.length < 2) { derr.writefln("read_track: reading arg list failed; split(\"=\").length < 2!\""); continue; }
		auto val = std.string.strip(s[1]);

		switch(std.string.strip(s[0])) {
			case "beatdiv":
				t.npb = to!float(s[1]);		// add try catch
				writefln("track " ~ t.name ~ ": found beatdiv arg: %f", t.npb);
				break;
			case "channel":
	//			channel = to!int(s[1]);		
				if (val == "left" || val == "l") {
					t.channel |= 0x1;
				}
				else if (val == "right" || val == "r") {
					t.channel |= 0x2;
				}
				else {
					derr.writefln("read_track: args: unrecognized channel option \"" ~ s[1] ~ "\". defaulting to both (l+r)");
				}
				break;
			case "sound":
	//			sound = to!int(s[1]);
				writeln("track " ~ t.name ~ ": found sound arg (nyi)");
				break;
			case "reverse":
				// nyi
			case "inverse":
				// nyi
			case "equal_temperament_steps":
				// enables non-standard scales to be used
				// nyi
			case "loop":
				if (val == "true") t.loop = true;
				writefln("track " ~ t.name ~ ": looping enabled");
				break;

			case "active":
				if (val == "false" || val == "0") t.active = 0;
				writefln("track " ~ t.name ~ ": active=false");
				break;

			default:
				derr.writeln("read_track: warning: unknown track arg \"" ~ s[0] ~ "\", ignoring");
		}

	}

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
			s.active_tracks = find_comma_sep_list_contents(tracks);
			writeln("found song block with tracks: ");
			foreach(t; s.active_tracks) writeln(t);
		}
	}
	if (!song_found) { writeln("sgen: no song{...} block found -> no input -> no output -> exiting."); return 0; }

	foreach(expr; expressions) {
		auto w = expr.words[0];
		if (w == "track") {
			track t;
			if (!get_trackname(expr, t)) { return 0; }
			int active = 0;
			foreach (a; s.active_tracks) {
				if (std.string.strip(a) == t.name) {
					active = 1;
				}
			}

			if (!active) {
				writefln("sgen: warning: track " ~ t.name ~ " defined but not used in a song block!");
				continue;
			}

			if (!read_track(expr, t)) { derr.writefln("read_track failure. syntax error(?)"); }
			s.tracks ~= t;
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
	const long num_samples = 10*44100;
	float[] left_channel_buffer = new float[num_samples];
	float[] right_channel_buffer = new float[num_samples];
	
	std.c.string.memset(cast(void*)left_channel_buffer, 0, left_channel_buffer.length*float.sizeof);
	std.c.string.memset(cast(void*)right_channel_buffer, 0, right_channel_buffer.length*float.sizeof);

	foreach(t; s.tracks) {
		if (!t.active) { continue; }
		float note_dur = (1.0/(t.npb*(s.tempo_bpm/60)));
		writefln("note_dur = " ~ to!string(note_dur) ~ " s.");

		long num_samples_per_note = to!long(std.math.ceil(44100*note_dur));

		long l_buffer_offset = 0;
		long r_buffer_offset = 0;

		int note_index = 0;
		while (l_buffer_offset < left_channel_buffer.length &&
		       r_buffer_offset < right_channel_buffer.length) {

		       if (note_index >= t.notes.length - 1) {
		       		if (!t.loop) { break; }
				else { note_index = 0; }
			}
			
			int n = t.notes[note_index];

			if (n == 0) { // a zero represents a rest
				l_buffer_offset += num_samples_per_note;
				r_buffer_offset += num_samples_per_note;
				continue; 
			}	 
			float pitch_hz = pitch_from_index((2*n+10));
			envelope e;
			float[] nbuf = note_synthesize(pitch_hz, note_dur, e, &waveform_triangle);

			if (t.channel & 0x1) {
				long i = 0;
				while (i < nbuf.length && l_buffer_offset < left_channel_buffer.length) {
					left_channel_buffer[l_buffer_offset] += nbuf[i];
					++i;
					++l_buffer_offset;
				}
			}

			if (t.channel & 0x2) {
				long i = 0;
				while (i < nbuf.length && r_buffer_offset < right_channel_buffer.length) {
					right_channel_buffer[r_buffer_offset] += nbuf[i];
					++i;
					++r_buffer_offset;
				}
			}

			++note_index;

		}
	}

	float[] merged = new float[2*num_samples];
	for (long i = 0; i < num_samples; ++i) {
		merged[2*i] = left_channel_buffer[i];
		merged[2*i+1] = right_channel_buffer[i];
	}

	output o;
	render_into_output_format(o, merged);

/*	auto file = std.stdio.File("waveform.dat", "w");
	for (long i = 0; i < buffer.length; ++i) {
		file.writefln("%d\t%f", i, buffer[i]);
	}

	file.close(); */

	return 1;
}

char[][] read_file_filter_comments(string filename) {

	auto file_contents = appender!(char[]);
	auto file = std.stdio.File(filename, "r");
	foreach (line; file.byLine()) {
		auto line_stripped = std.string.strip(line);
		if (!line_stripped.length) { continue; }
		else if (line_stripped[0] != '#') {
			file_contents.put(line_stripped ~ "\n");
		}
	}
	auto words = file_contents.data.split();
	file.close();

	return words;

}

int main() {
	static const char[] sgen_version = "0.01";

	string test_filename = "input_test.sg";
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
