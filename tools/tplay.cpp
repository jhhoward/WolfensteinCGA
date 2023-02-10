#include <stdio.h>
#include <stdint.h>
#include <i86.h>
#include <dos.h>
#include <conio.h>
#include <time.h>

#define SOUND_FIRST_BYTE 0x80
#define ATTENUATION_MASK 0x10

#define NOISE_CHANNEL_MASK 0x60

typedef void (__interrupt __far* INTFUNCPTR)(void);

INTFUNCPTR oldTimerInterrupt; 
volatile long int milliseconds;

void __interrupt __far TimerHandler(void)
{
	static unsigned long count = 0; // To keep track of original timer ticks
	++milliseconds;
	count += 1103;

	if (count >= 65536) // It is now time to call the original handler
	{
		count -= 65536;
		_chain_intr(oldTimerInterrupt);
	}
	else
		outp(0x20, 0x20); // Acknowledge interrupt
}

void InstallTimer()
{
	union REGS r;
	struct SREGS s;
	_disable();
	segread(&s);
	/* Save old interrupt vector: */
	r.h.al = 0x08;
	r.h.ah = 0x35;
	int86x(0x21, &r, &r, &s);
	oldTimerInterrupt = (INTFUNCPTR)MK_FP(s.es, r.x.bx);
	/* Install new interrupt handler: */
	milliseconds = 0;
	r.h.al = 0x08;
	r.h.ah = 0x25;
	s.ds = FP_SEG(TimerHandler);
	r.x.dx = FP_OFF(TimerHandler);
	int86x(0x21, &r, &r, &s);
	/* Set resolution of timer chip to 1ms: */
	outp(0x43, 0x36);
//	outp(0x40, (unsigned char)(1103 & 0xff));
//	outp(0x40, (unsigned char)((1103 >> 8) & 0xff));

	int hz = 1000;
	int rate = 1192030 / hz;

	outp(0x40, (unsigned char)(rate & 0xff));
	outp(0x40, (unsigned char)((rate >> 8) & 0xff));
	_enable();
}

void ShutdownTimer()
{
	union REGS r;
	struct SREGS s;
	_disable();
	segread(&s);
	/* Re-install original interrupt handler: */
	r.h.al = 0x08;
	r.h.ah = 0x25;
	s.ds = FP_SEG(oldTimerInterrupt);
	r.x.dx = FP_OFF(oldTimerInterrupt);
	int86x(0x21, &r, &r, &s);
	/* Reset timer chip resolution to 18.2...ms: */
	outp(0x43, 0x36);
	outp(0x40, 0x00);
	outp(0x40, 0x00);
	_enable();
}

 

int GetFValue(int frequency)
{
	return (int)(3579545.0f / (frequency * 32));
}

void PlayChannel(int channel, int frequency, int volume)
{
	int fvalue = GetFValue(frequency);
	
	// Frequency
	uint8_t channelBits = (channel & 3) << 5;
	outp(0xc0, SOUND_FIRST_BYTE | channelBits | ((uint8_t)(fvalue & 0xf)));
	//printf("%x ", SOUND_FIRST_BYTE | channelBits | (uint8_t)(fvalue >> 6) & 0xf);
	outp(0xc0, (uint8_t)(fvalue >> 4) & 0x3f);
	//printf("%x ", (fvalue >> 4) & 0x3f);
	
	// Attenuation
	int attenuation = 0xf - volume;
	outp(0xc0, SOUND_FIRST_BYTE | channelBits | ATTENUATION_MASK | (attenuation & 0xf));
	//printf("%x ", SOUND_FIRST_BYTE | channelBits | ATTENUATION_MASK | (attenuation & 0xf));
}

void SetChannelVolume(int channel, int volume)
{
	// Attenuation
	uint8_t channelBits = (channel & 3) << 5;
	int attenuation = 0xf - volume;
	outp(0xc0, SOUND_FIRST_BYTE | channelBits | ATTENUATION_MASK | (attenuation & 0xf));
}

void Wait(int delay)
{
	milliseconds = 0;
	while(milliseconds < delay) {}
}

int domain(int argc, char* argv[])
{
	if(argc != 2)
	{
		printf("Usage: %s [file]\n", argv[0]);
		

		int testFrequency = 440;
		int fvalue = GetFValue(testFrequency);
		printf("%d Hz = %d\n", testFrequency, fvalue);
		
		PlayChannel(0, testFrequency, 8);
		Wait(1000);
		/*for(int n = 430; n < 450; n++)
		{
			int fvalue = GetFValue(n);
			printf("%d Hz = %d\t", n, fvalue);
		
			float freq = 3579545.0f / (fvalue * 32);
			printf("%d = %f Hz\n", fvalue, freq);

			PlayChannel(0, n, 8);
			Wait(1000);
			PlayChannel(0, n, 0);
			Wait(500);
		}*/
		
		/*
		for(int i = 0; i < 5; i++)
		{
			// Attack
			PlayChannel(0, 440, 0);
			for(int n = 0; n < 16; n++)
			{
				SetChannelVolume(0, n);
				Wait(2);
			}
			// Decay
			PlayChannel(0, 440, 0);
			for(int n = 15; n >= 8; n--)
			{
				SetChannelVolume(0, n);
				Wait(10);
			}
			
			// Sustain
			Wait(100);
			
			// Release
			for(int n = 7; n >= 0; n--)
			{
				SetChannelVolume(0, n);
				Wait(30);
			}
			
			Wait(1000);
		}
		*/
		
		return 0;
	}

/*
	for(int c = 0; c < 3; c++)
	{
		for(int n = 100; n < 500; n++)
		{
			PlayChannel(c, n, 3);
			while(milliseconds < 5);
			milliseconds = 0;
		}
		PlayChannel(c, 1, 0);
	}
	*/
	FILE* fs = fopen(argv[1], "rb");
	if(!fs)
	{
		printf("Could not open %s\n", argv[1]);
	}
	
	while(!feof(fs))
	{
		//if(kbhit())
		//{
		//	break;
		//}
		uint8_t data;
		fread(&data, 1, 1, fs);
		
		if(data == 0xfe)
		{
			// This is a wait command
			uint16_t wait;
			fread(&wait, 2, 1, fs);

			milliseconds = 0;
			while(milliseconds < wait)
			{
				if(kbhit())
				{
					fclose(fs);
					return 0;
				}
			}		
		}
		else
		{
			outp(0xc0, data);
		}
	}
	
	fclose(fs);

	return 0;
}

int main(int argc, char* argv[])
{
	InstallTimer();
	domain(argc, argv);
	
	for(int n = 0; n < 4; n++)
	{
		SetChannelVolume(n, 0);
	}
	ShutdownTimer();
}
