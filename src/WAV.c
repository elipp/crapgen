#include "types.h"
#include "WAV.h"

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



