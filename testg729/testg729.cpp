// testg729.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <string.h>
#include <stdlib.h>

// define Wave format structure
typedef struct tWAVEFORMATEX
{
	short wFormatTag;  /* format type */
	short nChannels;   /* number of channels (i.e. mono, stereo...) */
	unsigned int nSamplesPerSec;     /* sample rate */
	unsigned int nAvgBytesPerSec;    /* for buffer estimation */
	short nBlockAlign;        /* block size of data */
	short wBitsPerSample;     /* number of bits per sample of mono data */
	short cbSize;             /* the count in bytes of the size of */
							  /* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX;

// read wave file
int wavread(char *fname, WAVEFORMATEX *wf, unsigned char ** wavedata, int & datalength)
{
	FILE* fp;
	char str[32];
	unsigned char *speech;
	unsigned int subchunk1size; // head size
	unsigned int subchunk2size; // speech data size
	int err;
								// check format type
	
	if ((err = fopen_s(&fp, fname, "rb")) != 0)
		{
		fprintf(stderr, "Can not open the wave file: %s.\n", fname);
		return -1;
	}
	fseek(fp, 8, SEEK_SET);
	fread(str, sizeof(char), 7, fp);
	str[7] = '\0';
	if (strcmp(str, "WAVEfmt")) {
		fprintf(stderr, "The file is not in WAVE format!\n");
		return -1;
	}

	// read format header
	fseek(fp, 16, SEEK_SET);
	fread((unsigned int*)(&subchunk1size), 4, 1, fp);
	fseek(fp, 20, SEEK_SET);
	fread(wf, subchunk1size, 1, fp);

	// read wave data
	fseek(fp, 20 + subchunk1size, SEEK_SET);
	fread(str, 1, 4, fp);
	str[4] = '\0';
	if (strcmp(str, "data")) {
		fprintf(stderr, "Locating data start point failed!\n");
		return -1;
	}
	fseek(fp, 20 + subchunk1size + 4, SEEK_SET);
	fread((unsigned int*)(&subchunk2size), 4, 1, fp);
	speech = (unsigned char*)malloc(sizeof(unsigned char)*subchunk2size);
	memset(speech, 0, subchunk2size);
	if (!speech) {
		fprintf(stderr, "Memory alloc failed!\n");
		return -1;
	}
	fseek(fp, 20 + subchunk1size + 8, SEEK_SET);
	fread(speech, 1, subchunk2size, fp);
	fclose(fp);

	*wavedata = speech;
	datalength = subchunk2size;
	return 0;
}

// wav文件头结构
typedef struct s_wave_header {
	char riff[4];
	unsigned long fileLength;
	char wavTag[4];
	char fmt[4];
	unsigned long size;
	unsigned short formatTag;
	unsigned short channel;
	unsigned long sampleRate;
	unsigned long bytePerSec;
	unsigned short blockAlign;
	unsigned short bitPerSample;
	char     data[4];
	unsigned long dataSize;
}wave_header;

// 生成wave文件头
void * createWaveHeader(int fileLength, short channel, int sampleRate, short bitPerSample)
{
	wave_header * header = (wave_header *)malloc(sizeof(wave_header));

	// RIFF
	header->riff[0] = 'R';
	header->riff[1] = 'I';
	header->riff[2] = 'F';
	header->riff[3] = 'F';

	// file length
	header->fileLength = fileLength + (44 - 8);

	// WAVE
	header->wavTag[0] = 'W';
	header->wavTag[1] = 'A';
	header->wavTag[2] = 'V';
	header->wavTag[3] = 'E';

	// fmt
	header->fmt[0] = 'f';
	header->fmt[1] = 'm';
	header->fmt[2] = 't';
	header->fmt[3] = ' ';

	header->size = 16;
	header->formatTag = 1;
	header->channel = channel;
	header->sampleRate = sampleRate;
	header->bitPerSample = bitPerSample;
	header->blockAlign = (short)(header->channel * header->bitPerSample / 8);
	header->bytePerSec = header->blockAlign * header->sampleRate;

	// data
	header->data[0] = 'd';
	header->data[1] = 'a';
	header->data[2] = 't';
	header->data[3] = 'a';

	// data size
	header->dataSize = fileLength;

	return (void *)header;
}

void wavwrite(char *fname, WAVEFORMATEX *wf, unsigned char * wavedata, int datalength)
{
	int err;

	FILE * fp;
	if ((err = fopen_s(&fp, fname, "wb")) ==0)
	{
		void * header = createWaveHeader(datalength, wf->nChannels, wf->nSamplesPerSec, wf->wBitsPerSample);
		fwrite(header, sizeof(wave_header), 1, fp);
		fwrite(wavedata, datalength, 1, fp);
		fclose(fp);

		if (header)
		{
			free(header);
		}
	}
}

extern "C" void G729aInitEncoder();
extern "C" void G729aEncoder(short *speech, unsigned char *bitstream, int nSize);
extern "C" void G729aInitDecoder();
extern "C" void G729aDecoder(unsigned char *bitstream, short *synth_short, int bfi, int nSize);

int main()
{
	unsigned char * wavedata = NULL;
	int datalength = 0;
	WAVEFORMATEX wf;
	int a = sizeof(wave_header);

	G729aInitEncoder();
	G729aInitDecoder();

	wavread("music.wav", &wf, &wavedata, datalength);
	int count = datalength / 160; // G.729以160字节为单位编码，输出10个字节
	unsigned char * buffer = (unsigned char *)malloc(count * 10);
	unsigned char * pout = (unsigned char *)malloc(datalength);
	int i = 0;
	for (i = 0; i < count; i++)
	{
		G729aEncoder((short*)(wavedata + i * 160), buffer + i * 10, 1);
		G729aDecoder(buffer + i * 10, (short*)(pout + i * 160), 0, 1);
	}

	wavwrite("test.wav", &wf, pout, datalength);

    return 0;
}

