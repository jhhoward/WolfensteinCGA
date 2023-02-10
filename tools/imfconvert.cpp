#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

struct AudioPacket
{
	uint8_t reg;
	uint8_t data;
	uint16_t delay;
};

struct Voice
{
	bool on;
	int fvalue;
	int block;
	float frequency;
	int sustain[2];
	int decay[2];
	int release[2];
	int attack[2];
	int waveform[2];
	int ampmod[2];
	int vib[2];
	int egt[2];
	int ksr[2];
	int mfm[2];
	int scaling[2];
	int level[2];
	
	bool noise;
	bool used;
	int feedback;
	int algorithm;
	
	int volume;
	float onTime;
	int currentOutputChannel;
};

struct WaveHeader
{
	uint32_t chunkID;
	uint32_t chunkSize;
	uint32_t format;
	uint32_t subchunk1ID;
	uint32_t subchunk1Size;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	uint32_t subchunk2ID;
	uint32_t subchunk2Size;
};

enum MixMethod
{
	Mix_RoundRobin,
	Mix_ReplaceLatest,
	Mix_PlayLatest,
	Mix_PlayLoudest,
	Mix_ReplaceLoudest,
	Mix_ByVoiceNumber
};

// PC speaker
//#define HAS_VOLUME 0
//#define HAS_NOISE_CHANNEL 0
//#define MAX_CHANNELS 1
//#define NOISE_CHANNEL MAX_CHANNELS
//MixMethod mixMethod = Mix_PlayLoudest; //Mix_RoundRobin;

// Tandy 3 voice
#define HAS_VOLUME 1
#define HAS_NOISE_CHANNEL 1
#define MAX_CHANNELS 3
#define NOISE_CHANNEL MAX_CHANNELS
MixMethod mixMethod = Mix_PlayLoudest;

//MixMethod mixMethod = Mix_RoundRobin;
//MixMethod mixMethod = Mix_PlayLatest;
//MixMethod mixMethod = Mix_ReplaceLatest; //Mix_ReplaceLoudest;
//MixMethod mixMethod = Mix_ReplaceLoudest;

#define NUM_VOICES 9

Voice voices[NUM_VOICES];

#define SAMPLE_RATE 44100

bool outputWav = true;
bool outputTandy = true;

void PopulateWaveHeader(WaveHeader& header, uint32_t dataSize)
{
	header.chunkID = 0x46464952; // "RIFF"
	header.chunkSize = 36 + dataSize;
	header.format = 0x45564157;	// "WAVE"
	header.subchunk1ID = 0x20746d66;	// "fmt "
	header.subchunk1Size = 16;
	header.audioFormat = 1;
	header.numChannels = 1;
	header.sampleRate = SAMPLE_RATE;
	header.byteRate = SAMPLE_RATE;
	header.blockAlign = 1;
	header.bitsPerSample = 8;
	header.subchunk2ID = 0x61746164; // "data"
	header.subchunk2Size = dataSize;
}


#define SOUND_FIRST_BYTE 0x80
#define ATTENUATION_MASK 0x10

#define NOISE_CHANNEL_MASK 0x60

#define TANDY_INTERRUPT_FREQUENCY 1000

struct TandyChannel
{
	union
	{
		int fvalue;
		int noiseType;
	};
	
	union
	{
		bool frequencyDirty;
		bool noiseTypeDirty;
	};	
	
	uint8_t attenuation;
	bool attenuationDirty;
};

struct TandySoundStatus
{
	TandyChannel channels[4];
	float accumulatedTime;
} tandySoundStatus;

FILE* tandyOutputFileStream;

int GetFValue(float frequency)
{
	if(frequency <= 0)
		return 0x3ff;
	//return 4770000 / (frequency * 32);
	//int result = (int)(3579540.0f / (frequency * 32));
	int result = (int)(4770000.0f / (frequency * 32));

	if(result > 0x3ff)
		return GetFValue(frequency * 2);
	return result;
}

float BinFrequency(float frequency)
{
	int fvalue = GetFValue(frequency);
	return (3579545.0f / (fvalue * 32));
}

void WriteFrequency(FILE* fs, int channel, int fvalue)
{
	// Frequency
	uint8_t channelBits = (channel & 3) << 5;
	
	uint8_t data;
	//data = SOUND_FIRST_BYTE | channelBits | ((uint8_t)(fvalue >> 6) & 0xf);
	data = SOUND_FIRST_BYTE | channelBits | ((uint8_t)(fvalue & 0xf));
	fwrite(&data, 1, 1, fs);
	
	data = (uint8_t)(fvalue >> 4) & 0x3f;
	fwrite(&data, 1, 1, fs);
}

void WriteAttenuation(FILE* fs, int channel, int attenuation)
{
	if(channel != NOISE_CHANNEL)
		attenuation = attenuation == 0xf ? 0xf : 0x5;
	
	uint8_t channelBits = (channel & 3) << 5;
	uint8_t data = SOUND_FIRST_BYTE | channelBits | ATTENUATION_MASK | (attenuation & 0xf);
	
	if(data == 0xfe)
	{
		printf("ERROR\n");
		exit(1);
	}
	
	fwrite(&data, 1, 1, fs);
}

void WriteNoiseType(FILE* fs, int noiseBits)
{
	// Noise type
	uint8_t data = SOUND_FIRST_BYTE | NOISE_CHANNEL_MASK | noiseBits;
	
	if(data == 0xfe)
	{
		printf("ERROR\n");
		exit(1);
	}
	fwrite(&data, 1, 1, fs);
}

void WriteWait(FILE* fs, float ms)
{
	int wait = (int)((ms * TANDY_INTERRUPT_FREQUENCY) / 1000);

	if(wait > 65535)
	{
		wait = 65535;
	}
	
	uint8_t data = 0xfe;
	fwrite(&data, 1, 1, fs);
	
	data = (uint8_t)(wait & 0xff);
	fwrite(&data, 1, 1, fs);
	
	data = (uint8_t)(wait >> 8);
	fwrite(&data, 1, 1, fs);
}

void Flush()
{
	if(tandySoundStatus.accumulatedTime > 0)
	{
		if(tandyOutputFileStream)
		{
			for(int c = 0; c < 4; c++)
			{
				if(c == NOISE_CHANNEL)
				{
					if(tandySoundStatus.channels[c].noiseTypeDirty)
					{
						WriteNoiseType(tandyOutputFileStream, tandySoundStatus.channels[c].noiseType);
						tandySoundStatus.channels[c].noiseTypeDirty = false;
					}
				}
				else
				{
					if(tandySoundStatus.channels[c].frequencyDirty)
					{
						WriteFrequency(tandyOutputFileStream, c, tandySoundStatus.channels[c].fvalue);
						tandySoundStatus.channels[c].frequencyDirty = false;
					}
				}
				if(tandySoundStatus.channels[c].attenuationDirty)
				{
					WriteAttenuation(tandyOutputFileStream, c, tandySoundStatus.channels[c].attenuation);
					tandySoundStatus.channels[c].attenuationDirty = false;
				}
			}
			
			WriteWait(tandyOutputFileStream, tandySoundStatus.accumulatedTime);
		}
		
		tandySoundStatus.accumulatedTime = 0;
	}
}

void EmitDelay(float ms)
{
	tandySoundStatus.accumulatedTime += ms;
}

void EmitNoiseType(int noiseType, int noiseShift)
{
	int noiseBits = ((noiseType & 1) << 2) | (noiseShift & 3);
	
	if(tandySoundStatus.channels[NOISE_CHANNEL].noiseType == noiseBits)
	{
		return;
	}
	
	Flush();
	
	tandySoundStatus.channels[NOISE_CHANNEL].noiseType = noiseBits;
	tandySoundStatus.channels[NOISE_CHANNEL].noiseTypeDirty = true;
}

void EmitFrequencyChange(int channel, float frequency)
{
	int fvalue = GetFValue(frequency);
		
	if(tandySoundStatus.channels[channel].fvalue == fvalue)
		return;

	Flush();

	tandySoundStatus.channels[channel].fvalue = fvalue;
	tandySoundStatus.channels[channel].frequencyDirty = true;
}

void EmitVolumeChange(int channel, int volume)
{
	int attenuation = 0xf - volume; // (volume == 0) ? 0xf : volume - 1;

	if(tandySoundStatus.channels[channel].attenuation == attenuation)
		return;
		
	Flush();

	tandySoundStatus.channels[channel].attenuation = attenuation;
	tandySoundStatus.channels[channel].attenuationDirty = true;
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		printf("Usage: %s [filename]\n", argv[0]);
		return 0;
	}
	
	FILE* fs = fopen(argv[1], "rb");
	if(!fs)
	{
		printf("Could not open %s\n", argv[1]);
		return 0;
	}
	
	for(int v = 0; v < NUM_VOICES; v++)
	{
		voices[v].currentOutputChannel = -1;
	}
	
	//voices[1].noise = true;
	//voices[8].noise = true;
	//voices[5].noise = true;
	//voices[8].noise = true;

	// GETTHEM
	//voices[1].noise = true;
	
	// WARMARCH
	//voices[1].noise = true;
	
	
	WaveHeader waveHeader;
	FILE* fout;

	if(outputWav)
	{
		fout = fopen("out.wav", "wb");
		fwrite(&waveHeader, sizeof(WaveHeader), 1, fout);
	}
	
	if(outputTandy)
	{
		tandyOutputFileStream = fopen("out.tdy", "wb");
	}
	
	uint16_t musicSize;
	fread(&musicSize, sizeof(uint16_t), 1, fs);
	
	if(musicSize)
	{
		printf("Size: %d\n", musicSize);
	}
	else
	{
		fseek(fs, 0, SEEK_SET);
	}
	
	int sampleCount = 0;
	int bytesRead = 0;
	float time = 0;

	EmitNoiseType(1, 0);
	
	while(!feof(fs))
	{
		AudioPacket packet;
		if(!fread(&packet, sizeof(AudioPacket), 1, fs))
		{
			break;
		}
		bytesRead += sizeof(AudioPacket);
		
		float ms = (packet.delay * 1000.0f) / 700;
		time += ms;
		
//		printf("Reg: %03x Data: %03x\tDelay: %d ms\n", packet.reg, packet.data, ms);
		if(packet.reg >= 0xb0 && packet.reg <= 0xb8)
		{
			// Key on / Octave / Frequency
			
			int voicenum = packet.reg - 0xb0;
			
			voices[voicenum].fvalue = ((packet.data & 0x3) << 8) | (voices[voicenum].fvalue & 0xff);
			voices[voicenum].on = (packet.data & 0x20) != 0;
			voices[voicenum].block = (packet.data & 0x1c) >> 2;
			
			// Music Frequency = (F-Num * 49716) / (2^(20-Block))
			voices[voicenum].frequency = (voices[voicenum].fvalue * 49716.0f) / (1 << (20 - voices[voicenum].block));
			
			if(voices[voicenum].on)
			{
				voices[voicenum].used = true;
				voices[voicenum].onTime = time;

				// Find an output channel
				if(!voices[voicenum].noise)
				{
					if(voices[voicenum].currentOutputChannel == -1)
					{
						for(int c = 0; c < MAX_CHANNELS; c++)
						{
							bool channelUsed = false;
							for(int i = 0; i < NUM_VOICES; i++)
							{
								if(voices[i].currentOutputChannel == c)
								{
									channelUsed = true;
									break;
								}
							}
							
							if(!channelUsed)
							{
								voices[voicenum].currentOutputChannel = c;
								break;
							}
						}
						
						// No channels available, check to see if we can override another one
						if(voices[voicenum].currentOutputChannel == -1)
						{
							int bestReplacementVoice = -1;
							for(int c = 0; c < MAX_CHANNELS; c++)
							{
								for(int i = 0; i < NUM_VOICES; i++)
								{
									if(voices[i].currentOutputChannel == c)
									{
										if(voices[voicenum].volume >= voices[i].volume)
										{
											if(bestReplacementVoice == -1 || voices[i].volume < voices[bestReplacementVoice].volume)
											{
												bestReplacementVoice = i;
											}
										}
									}
								}
							}
							
							if(bestReplacementVoice != -1)
							{
								voices[voicenum].currentOutputChannel = voices[bestReplacementVoice].currentOutputChannel;
								voices[bestReplacementVoice].currentOutputChannel = -1;
							}
						}
					}
					
					if(voices[voicenum].currentOutputChannel != -1)
					{
						EmitFrequencyChange(voices[voicenum].currentOutputChannel, voices[voicenum].frequency);
						EmitVolumeChange(voices[voicenum].currentOutputChannel, voices[voicenum].volume);
					}
				}
			}
			else
			{
				// Key off
				if(voices[voicenum].currentOutputChannel != -1)
				{
					EmitVolumeChange(voices[voicenum].currentOutputChannel, 0);
					voices[voicenum].currentOutputChannel = -1;
				}
			}
		
			
			if(0)
			{
				putchar('[');
				for(int v = 0; v < NUM_VOICES; v++)
				{
					if(voices[v].on)
					{
						putchar('#');
					}
					else
					{
						putchar(' ');
					}
				}
				printf("]  ");

				for(int v = 0; v < NUM_VOICES; v++)
				{
					if(voices[v].on)
					{
						printf("[%03.1f] ", voices[v].frequency);
					}
					else
					{
						printf("[     ] ");
					}
				}
				printf("\n");
			}
		}
		else if(packet.reg >= 0xa0 && packet.reg <= 0xa8)
		{
			// Frequency
			int voicenum = packet.reg - 0xa0;
			voices[voicenum].fvalue = (voices[voicenum].fvalue & 0x300) | packet.data;
			
			if(voices[voicenum].currentOutputChannel != -1)
			{
				EmitFrequencyChange(voices[voicenum].currentOutputChannel, voices[voicenum].frequency);
				EmitVolumeChange(voices[voicenum].currentOutputChannel, voices[voicenum].volume);
			}
		}
		else if(packet.reg >= 0x80 && packet.reg <= 0x95)
		{
			// Sustain / release
			int offset = packet.reg - 0x80;
			int voicenum = ((offset / 6) * 3) + (offset % 3);
			int op = (offset % 6) < 3 ? 0 : 1;
			if(voicenum < NUM_VOICES)
			{
				voices[voicenum].sustain[op] = (packet.data >> 4);
				voices[voicenum].release[op] = packet.data & 0xf;
				voices[voicenum].volume = voices[voicenum].sustain[0] > voices[voicenum].sustain[1] ? voices[voicenum].sustain[0] : voices[voicenum].sustain[1];
				
				if(voices[voicenum].currentOutputChannel != -1)
				{
					EmitVolumeChange(voices[voicenum].currentOutputChannel, voices[voicenum].volume);
				}
			}
		}
		else if(packet.reg >= 0x60 && packet.reg <= 0x75)
		{
			// Attack / decay
			int offset = packet.reg - 0x60;
			int voicenum = ((offset / 6) * 3) + (offset % 3);
			int op = (offset % 6) < 3 ? 0 : 1;
			if(voicenum < NUM_VOICES)
			{
				voices[voicenum].attack[op] = (packet.data >> 4);
				voices[voicenum].decay[op] = packet.data & 0xf;
			}
		}
		else if(packet.reg >= 0xe0 && packet.reg <= 0xf5)
		{
			// Wave form
			int offset = packet.reg - 0xe0;
			int voicenum = ((offset / 6) * 3) + (offset % 3);
			int op = (offset % 6) < 3 ? 0 : 1;
			if(voicenum < NUM_VOICES)
			{
				voices[voicenum].waveform[op] = (packet.data & 3);
				//printf("Voice %d op: %d waveform: %d\n", voicenum, op, voices[voicenum].waveform[op]);
			}
		}
		else if(packet.reg >= 0xc0 && packet.reg <= 0xc8)
		{
			// Feedback / algorithm
			int voicenum = packet.reg - 0xc0;
			if (voicenum < NUM_VOICES)
			{
				voices[voicenum].algorithm = packet.data & 1;
				voices[voicenum].feedback = (packet.data >> 1) & 7;
				//printf("Voice %d feedback: %d\n", voicenum, voices[voicenum].feedback);
			}
		}
		else if(packet.reg >= 0x20 && packet.reg <= 0x35)
		{
			// Amplitude Modulation / Vibrato / Envelope Generator Type / Keyboard Scaling Rate / Modulator Frequency Multiple			
			int offset = packet.reg - 0x20;
			int voicenum = ((offset / 6) * 3) + (offset % 3);
			int op = (offset % 6) < 3 ? 0 : 1;
			
			if(voicenum < NUM_VOICES)
			{
				voices[voicenum].ampmod[op] = packet.data >> 7;
				voices[voicenum].vib[op] = (packet.data >> 6) & 1;
				voices[voicenum].egt[op] = (packet.data >> 5) & 1;
				voices[voicenum].ksr[op] = (packet.data >> 4) & 1;
				voices[voicenum].mfm[op] = packet.data & 0xf;
				
				voices[voicenum].noise = voices[voicenum].mfm[0] >= 6 || voices[voicenum].mfm[1] >= 6;
				
				if(voices[voicenum].noise && voices[voicenum].currentOutputChannel != -1)
				{
					EmitVolumeChange(voices[voicenum].currentOutputChannel, 0);
					voices[voicenum].currentOutputChannel = -1;
				}

				//printf("Voice: %d op: %d ampmod: %d vib: %d egt: %d ksr: %d mfm: %d\n", voicenum, op, voices[voicenum].ampmod[op], voices[voicenum].vib[op], voices[voicenum].egt[op], voices[voicenum].ksr[op], voices[voicenum].mfm[op]);
			}
		}
		else if(packet.reg >= 0x40 && packet.reg <= 0x55)
		{
			// Level Key Scaling / Total Level
			int offset = packet.reg - 0x20;
			int voicenum = ((offset / 6) * 3) + (offset % 3);
			int op = (offset % 6) < 3 ? 0 : 1;
			
			if(voicenum < NUM_VOICES)
			{
				voices[voicenum].level[op] = packet.data & 0x3f;
				voices[voicenum].scaling[op] = (packet.data >> 6) & 3;
				//printf("Voice: %d op: %d level: %d\n", voicenum, op, voices[voicenum].level[op]);
			}
		}
		
		bool noiseOn = false;
		for(int v = 0; v < NUM_VOICES; v++)
		{
			if(voices[v].on && voices[v].noise)
			{
				noiseOn = true;
				break;
			}
		}
		EmitVolumeChange(NOISE_CHANNEL, noiseOn ? 2 : 0);
			
		
		EmitDelay(ms);
		
		#if 0
		if(outputWav)
		{
			int samplesToMake = (int)((SAMPLE_RATE * ms) / 1000);
			
			for(int n = 0; n < samplesToMake; n++)
			{
				uint8_t out = 127;
				float seconds = (float) sampleCount / SAMPLE_RATE;

				for(int v = 0; v < NUM_VOICES; v++)
				{
					voices[v].currentOutputChannel = -1;

					voices[v].noise = voices[v].mfm[0] >= 6 || voices[v].mfm[1] >= 6;
	#if HAS_NOISE_CHANNEL
					if(voices[v].noise)
					{
						voices[v].currentOutputChannel = NOISE_CHANNEL;
					}
	#endif
				}

				int start = 0;
				
				if(mixMethod == Mix_RoundRobin)
				{
					start = ((int)(seconds * 70)) % NUM_VOICES;
					//start = sampleCount % NUM_VOICES;
				}
				
				for(int c = 0; c < MAX_CHANNELS; c++)
				{
					int bestVoice = -1;
					for(int i = 0; i < NUM_VOICES; i++)
					{
						int v = (start + i) % NUM_VOICES;
						if(voices[v].on && voices[v].currentOutputChannel == -1)
						{
							if(mixMethod == Mix_RoundRobin || mixMethod == Mix_ByVoiceNumber)
							{
								bestVoice = v;
								break;
							}
							else
							{
								switch(mixMethod)
								{
									case Mix_ReplaceLatest:
									case Mix_PlayLatest:
									if(bestVoice == -1 || voices[v].onTime > voices[bestVoice].onTime)
									{
										bestVoice = v;
									}
									break;
									
									case Mix_ReplaceLoudest:
									case Mix_PlayLoudest:
									if(bestVoice == -1 || voices[v].volume > voices[bestVoice].volume)
									{
										bestVoice = v;
									}
									else if(voices[v].volume == voices[bestVoice].volume)
									{
										if(voices[v].onTime > voices[bestVoice].onTime)
										{
											bestVoice = v;
										}
									}
									break;
								}
							}
						}
					}	
					if(bestVoice != -1)
					{
						voices[bestVoice].currentOutputChannel = c;
					}
					//channelAllocation[c] = bestVoice;
				}
				
				if(mixMethod == Mix_ReplaceLatest || mixMethod == Mix_ReplaceLoudest)
				{
					for(int v = 0; v < NUM_VOICES; v++)
					{
						if(voices[v].currentOutputChannel == -1)
							voices[v].on = false;
					}				
				}
				
				int noiseStrength = 0;
				
				for(int v = 0; v < NUM_VOICES; v++)
				{
					bool canPlay = voices[v].currentOutputChannel != -1;
					
					if(voices[v].on && canPlay)
					{
						if(voices[v].currentOutputChannel != NOISE_CHANNEL)
						{
							float frequency = BinFrequency(voices[v].frequency);
							float phase = seconds * frequency;
							float f = sin(phase * 3.1415 * 2);
							float phasefrac = phase - (int) phase;
							
							/*
							if(f > 0)
							{
								#if HAS_VOLUME
								out += voices[v].volume + 1;
								#else
								out = 32;
								#endif
							}*/
							
							// Square wave
							if(phasefrac > 0.5f)
							{
								out += voices[v].volume + 1;							
							}
							
							// Triangle wave
							//out += (int)((voices[v].volume + 1) * phasefrac);
							
							// Sine wave
							//out += (int)(voices[v].volume * f);
							
							/*if(f > 0)
							{
								out += 14;
							}
							else
							{
								out -= 14;
							}*/
						}
						else
						{
							if(voices[v].volume > noiseStrength)
							{
								noiseStrength = (voices[v].volume);
							}
						}
					}
				}
				
				#if HAS_NOISE_CHANNEL
				if(noiseStrength)
				{
					out += (uint8_t)(rand() % (noiseStrength / 2 + 1));
				}
				#endif
							
				fwrite(&out, 1, 1, fout);
				sampleCount++;
			}
		}
		else
		{
			for(int v = 0; v < NUM_VOICES; v++)
			{
				voices[v].currentOutputChannel = -1;

				voices[v].noise = voices[v].mfm[0] >= 6 || voices[v].mfm[1] >= 6;
				if(voices[v].noise)
				{
					voices[v].currentOutputChannel = NOISE_CHANNEL;
				}
			}

			int start = 0;
			
			if(mixMethod == Mix_RoundRobin)
			{
				start = ((int)(time * 70)) % NUM_VOICES;
			}
				
			for(int c = 0; c < MAX_CHANNELS; c++)
			{
				int bestVoice = -1;
				for(int i = 0; i < NUM_VOICES; i++)
				{
					int v = (start + i) % NUM_VOICES;
					if(voices[v].on && voices[v].currentOutputChannel == -1)
					{
						if(mixMethod == Mix_RoundRobin || mixMethod == Mix_ByVoiceNumber)
						{
							bestVoice = v;
							break;
						}
						else
						{
							switch(mixMethod)
							{
								case Mix_ReplaceLatest:
								case Mix_PlayLatest:
								if(bestVoice == -1 || voices[v].onTime > voices[bestVoice].onTime)
								{
									bestVoice = v;
								}
								break;
								
								case Mix_ReplaceLoudest:
								case Mix_PlayLoudest:
								if(bestVoice == -1 || voices[v].volume > voices[bestVoice].volume)
								{
									bestVoice = v;
								}
								else if(voices[v].volume == voices[bestVoice].volume)
								{
									if(voices[v].onTime > voices[bestVoice].onTime)
									{
										bestVoice = v;
									}
								}
								break;
							}
						}
					}
				}	
				if(bestVoice != -1)
				{
					voices[bestVoice].currentOutputChannel = c;
				}
				//channelAllocation[c] = bestVoice;
			}
			
			if(mixMethod == Mix_ReplaceLatest || mixMethod == Mix_ReplaceLoudest)
			{
				for(int v = 0; v < NUM_VOICES; v++)
				{
					if(voices[v].currentOutputChannel == -1)
						voices[v].on = false;
				}				
			}
			
			float channelFrequency[3];
			int channelVolume[4];
			
			memset(channelFrequency, 0, 3 * sizeof(float));
			memset(channelVolume, 0, sizeof(int) * 4);
			
			for(int v = 0; v < NUM_VOICES; v++)
			{
				bool canPlay = voices[v].currentOutputChannel != -1;
				
				if(voices[v].on && canPlay)
				{
					if(voices[v].currentOutputChannel == NOISE_CHANNEL)
					{
						channelVolume[NOISE_CHANNEL] = 1;
						if(voices[v].volume > channelVolume[NOISE_CHANNEL])
						{
							channelVolume[NOISE_CHANNEL] = voices[v].volume;
						}
					}
					else
					{
						channelVolume[voices[v].currentOutputChannel] = voices[v].volume;
						channelFrequency[voices[v].currentOutputChannel] = voices[v].frequency;
					}
				}
			}			
			
			for(int c = 0; c < 3; c++)
			{
				WriteFrequency(fout, c, channelFrequency[c]);
			}
			for(int c = 0; c < 4; c++)
			{
				WriteVolume(fout, c, channelVolume[c]);
			}
			
			uint16_t wait = (uint16_t)(ms * 1000.0f / 1000.0f);
			WriteNoiseType(fout, 1, 0, wait);
		}
		#endif
		
		if(bytesRead >= musicSize)
		{
			break;
		}
	}
	
	/*for(int n = 0; n < SAMPLE_RATE; n++)
	{
		double val = sin(300.0 * n / SAMPLE_RATE);
		uint8_t out = (uint8_t)(val * 127);
		fwrite(&out, 1, 1, fout);
		sampleCount++;
	}*/

	if(outputWav)
	{
		fseek(fout, 0, SEEK_SET);
		PopulateWaveHeader(waveHeader, sampleCount);
		fwrite(&waveHeader, sizeof(WaveHeader), 1, fout);
	}
	
	fclose(fs);	
	
	fclose(fout);
		
	for(int v = 0; v < NUM_VOICES; v++)
	{
		if(!voices[v].used)
			continue;
		
		printf("Voice %d:\n", v);
		printf(" Feedback: %d Algorithm: %d\n", voices[v].feedback, voices[v].algorithm);
		for(int op = 0; op < 2; op++)
		{
			printf(" [%d] ", op + 1);
			printf("Attack: %d Decay: %d Sustain: %d Release: %d\n", voices[v].attack[op], voices[v].decay[op],  voices[v].sustain[op], voices[v].release[op]);
			printf("      ampmod: %d vib: %d egt: %d ksr: %d mfm: %d\n", voices[v].ampmod[op], voices[v].vib[op], voices[v].egt[op], voices[v].ksr[op], voices[v].mfm[op]);

			printf("\n");
		}
	}
	
	return 0;
}
