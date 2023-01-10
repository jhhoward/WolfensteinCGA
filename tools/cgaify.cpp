#include "lodepng.cpp"
#include <stdint.h>
#include <stdio.h>

using namespace std;

#define NUM_PATTERNS (sizeof(patterns) / 4)
uint8_t patterns[] =
{
	0, 0, 0, 0,
	0, 1, 0, 0,
	0, 1, 0, 1,
	1, 1, 0, 1,
	1, 1, 1, 1,
	1, 1, 3, 1,
	3, 1, 3, 1,
	3, 1, 3, 3,
	0, 2, 0, 0,
	0, 2, 0, 2,
	2, 2, 0, 2,
	2, 2, 2, 2,
	2, 2, 3, 2,
	3, 2, 3, 2,
	3, 2, 3, 3,
	3, 3, 3, 3,
	0, 0, 0, 0,
	0, 3, 0, 0,
	0, 3, 0, 3,
	3, 3, 0, 3,
	1, 2, 1, 2,
};

uint8_t patterns2[] = 
{
	0, 0, 0, 0,
	0, 1, 0, 1,
	1, 1, 1, 1,
	3, 1, 3, 1,
	0, 2, 0, 2,
	2, 2, 2, 2,
	3, 2, 3, 2,
	0, 3, 0, 3,
	3, 3, 3, 3,
	1, 2, 1, 2,
};

uint8_t patterns3[] =
{
	0, 0, 0, 0,
	0, 1, 0, 1,
	1, 1, 1, 1,
	1, 1, 3, 1,
	3, 1, 3, 1,
	3, 1, 3, 3,
	0, 2, 0, 2,
	2, 2, 2, 2,
	2, 2, 3, 2,
	3, 2, 3, 2,
	3, 2, 3, 3,
	3, 3, 3, 3,
	0, 0, 0, 0,
	0, 3, 0, 3,
	1, 2, 1, 2,
};


uint8_t patternsRGB[NUM_PATTERNS * 3];
uint8_t patternsRGBWeights[NUM_PATTERNS];


uint8_t palette[] =
{
	0, 0, 0x0,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0xff
};

uint8_t paletteBGRY[] =
{
	0, 0, 0xaa,
	0x55, 0xff, 0x55,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0x55
};

uint8_t paletteCRW[] =
{
	0, 0, 0,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0xff, 0xff
};

uint8_t palette1[] =
{
	0, 0, 0,
	0, 0xaa, 0xaa,
	0xaa, 0, 0,
	0xaa, 0xaa, 0xaa
};

uint8_t compositePatternRGB[256 * 3];
uint8_t compositePatternRGBWeights[256];

uint8_t compositePalette[] =
{
	0x00, 0x00, 0x00,
	0x00, 0x6e, 0x31,
	0x31, 0x09, 0xff,
	0x00, 0x8a,	0xff,
	0xa7, 0x00, 0x31,
	0x76, 0x76, 0x76,
	0xec, 0x11, 0xff,
	0xbb, 0x92, 0xff,
	0x31, 0x5a, 0x00,
	0x00, 0xdb, 0x00,
	0x76, 0x76, 0x76,
	0x45, 0xf7, 0xbb,
	0xec, 0x63, 0x00,
	0xbb, 0xe4, 0x00,
	0xff, 0x7f, 0xbb,
	0xff, 0xff, 0xff
};

uint8_t cgaPatternRGB[256 * 3];
uint8_t cgaPatternRGBWeights[256];

uint8_t cgaPalette[] =
{
	0,	0,	0,
	0,	0,	0xaa,
	0,	0xaa,	0,
	0,	0xaa,	0xaa,
	0xaa,	0,	0,
	0xaa,	0,	0xaa,
	0xaa,	0x55,	0,
	0xaa,	0xaa,	0xaa,
	0x55,	0x55,	0x55,
	0x55,	0x55,	0xff,
	0x55,	0xff,	0x55,
	0x55,	0xff,	0xff,
	0xff,	0x55,	0x55,
	0xff,	0x55,	0xff,
	0xff,	0xff,	0x55,
	0xff,	0xff,	0xff
};

uint8_t convertLUT[256];
uint8_t convertLUTComposite[256];

void GammaAdjust(uint8_t* rgb)
{
	double exponent = 1.5;
	double r = pow(rgb[0] / 255.0, exponent);
	double g = pow(rgb[1] / 255.0, exponent);
	double b = pow(rgb[2] / 255.0, exponent);
	
	rgb[0] = (uint8_t)(r * 255.0);
	rgb[1] = (uint8_t)(g * 255.0);
	rgb[2] = (uint8_t)(b * 255.0);
}

int FindClosestPaletteEntry(uint8_t* rgb, uint8_t* pal, int palSize, uint8_t* weights = NULL)
{
	int closest = -1;
	int closestDistance = 0;
	
	for(int n = 0; n < palSize; n++)
	{
		int distance = 0;
		
		for(int i = 0; i < 3; i++)
		{
			int diff = (rgb[i] - pal[n * 3 + i]);
			distance += diff * diff;
		}
		
		if(weights)
		{
			distance *= weights[n];
		}
		
		if(distance < 0)
		{
			continue;
		}
		
		if(closest == -1 || distance < closestDistance)
		{
			closest = n;
			closestDistance = distance;
		}
	}
	
	return closest;
}

void GeneratePatternsRGB()
{
	for(int n = 0; n < NUM_PATTERNS; n++)
	{
		int red = 0, green = 0, blue = 0;
		
		for(int i = 0; i < 4; i++)
		{
			uint8_t p = patterns[n * 4 + i];
			red += palette[p * 3];
			green += palette[p * 3 + 1];
			blue += palette[p * 3 + 2];
		}
		patternsRGB[n * 3] = (uint8_t)(red / 4);
		patternsRGB[n * 3 + 1] = (uint8_t)(green / 4);
		patternsRGB[n * 3 + 2] = (uint8_t)(blue / 4);
		
		GammaAdjust(patternsRGB + n * 3);
		
		int weight = 1;
		if(patterns[n * 4] != patterns[n * 4 + 2] || patterns[n * 4 + 1] != patterns[n * 4 + 3])
		{
		//	weight ++;
		}
		if(patterns[n * 4] != patterns[n * 4 + 1])
		{
			//weight ++;
		}			
		patternsRGBWeights[n] = weight;
	}
	
	vector<uint8_t> palettePixels;
	for(int n = 0; n < NUM_PATTERNS; n++)
	{
		palettePixels.push_back(patternsRGB[n * 3]);
		palettePixels.push_back(patternsRGB[n * 3 + 1]);
		palettePixels.push_back(patternsRGB[n * 3 + 2]);
		palettePixels.push_back(255);
	}
	lodepng::encode("palette.png", palettePixels, NUM_PATTERNS, 1);
}

void GenerateCompositePaletteRGB()
{
	int numCompositePatterns = 0;
	
	for(int x = 0; x < 16; x++)
	{
		for(int y = 0; y < 16; y++)
		{
			int index = y * 16 + x;
			int red = (compositePalette[x * 3] + compositePalette[y * 3]) / 2;
			int green = (compositePalette[x * 3 + 1] + compositePalette[y * 3 + 1]) / 2;
			int blue = (compositePalette[x * 3 + 2] + compositePalette[y * 3 + 2]) / 2;
			compositePatternRGB[index * 3] = (uint8_t) red;
			compositePatternRGB[index * 3 + 1] = (uint8_t) green;
			compositePatternRGB[index * 3 + 2] = (uint8_t) blue;
			compositePatternRGBWeights[index] = (x == y) ? 1 : 3;
			
			int pairDistance = 0;
			for(int n = 0; n < 3; n++)
			{
				int dist = compositePalette[x * 3 + n] - compositePalette[y * 3 + n];
				pairDistance += dist * dist;
			}
			if(pairDistance > 240 * 240)
			{
				compositePatternRGBWeights[index] = -1;	
			}
			else
			{
				numCompositePatterns++;
			}
		}
	}
	
	printf("Num composite patterns: %d\n", numCompositePatterns);
	

	vector<uint8_t> palettePixels;
	for(int n = 0; n < 256; n++)
	{
		palettePixels.push_back(compositePatternRGB[n * 3]);
		palettePixels.push_back(compositePatternRGB[n * 3 + 1]);
		palettePixels.push_back(compositePatternRGB[n * 3 + 2]);
		palettePixels.push_back(255);
	}
	lodepng::encode("compopalette.png", palettePixels, 16, 16);
}

void GenerateCGAPaletteRGB()
{
	for(int x = 0; x < 16; x++)
	{
		for(int y = 0; y < 16; y++)
		{
			int index = y * 16 + x;
			int red = (cgaPalette[x * 3] + cgaPalette[y * 3]) / 2;
			int green = (cgaPalette[x * 3 + 1] + cgaPalette[y * 3 + 1]) / 2;
			int blue = (cgaPalette[x * 3 + 2] + cgaPalette[y * 3 + 2]) / 2;
			cgaPatternRGB[index * 3] = (uint8_t) red;
			cgaPatternRGB[index * 3 + 1] = (uint8_t) green;
			cgaPatternRGB[index * 3 + 2] = (uint8_t) blue;
			
			cgaPatternRGBWeights[index] = (x == y) ? 1 : 3;
		}
	}

	vector<uint8_t> palettePixels;
	for(int n = 0; n < 256; n++)
	{
		palettePixels.push_back(cgaPatternRGB[n * 3]);
		palettePixels.push_back(cgaPatternRGB[n * 3 + 1]);
		palettePixels.push_back(cgaPatternRGB[n * 3 + 2]);
		palettePixels.push_back(255);
	}
	lodepng::encode("cgapalette.png", palettePixels, 16, 16);
}

void GenerateLUT()
{
	vector<uint8_t> pixels;
	unsigned width, height;

	if(lodepng::decode(pixels, width, height, "wolfpal.png"))
	{
		printf("Could not load wolfpal.png\n");
		exit(0);
	}
	
	if(width != 16 || height != 16)
	{
		printf("Expected palette to be 16x16\n");
		exit(0);
	}
	
	for(int y = 0; y < height; y++)
	{
		for(int x = 0; x < width; x++)
		{
			int index = y * width + x;
			uint8_t rgb[3];
			rgb[0] = pixels[index * 4];
			rgb[1] = pixels[index * 4 + 1];
			rgb[2] = pixels[index * 4 + 2];
			
			int match = FindClosestPaletteEntry(rgb, patternsRGB, NUM_PATTERNS, patternsRGBWeights);
			uint8_t pattern = 0;
			
			for(int n = 0; n < 4; n++)
			{
				pattern <<= 2;
				pattern |= (uint8_t)(patterns[match * 4 + n]);
			}
			convertLUT[index] = pattern;
			
			int compositeMatch = FindClosestPaletteEntry(rgb, compositePatternRGB, 256, compositePatternRGBWeights);
			pattern = (uint8_t)(compositeMatch);
//			int compositeMatch = FindClosestPaletteEntry(rgb, compositePalette, 16);
//			pattern = (uint8_t)(compositeMatch | (compositeMatch << 4));
			convertLUTComposite[index] = pattern;
		}
	}
}

void GenerateDataFile(const char* filename, uint8_t* data, int dataLength, uint8_t* conversionTable)
{
	uint8_t* newData = new uint8_t[dataLength];
	memcpy(newData, data, dataLength);
	data = newData;
	
	uint16_t* ptr = (uint16_t*) data;
	uint16_t numChunks = *ptr++;
	uint16_t spriteStart = *ptr++;
	uint16_t soundStart = *ptr++;

	uint32_t* chunkOffsets = (uint32_t*)(ptr);
	ptr += numChunks * 2;
	uint16_t* chunkLengths = ptr;
	ptr += numChunks;
	
	for(int n = 0; n < numChunks; n++)
	{
		uint8_t* chunkPtr = data + chunkOffsets[n];
		uint32_t chunkLength = chunkLengths[n];
		if(n < spriteStart)
		{
			// This is a texture
			for(int x = 0; x < chunkLength; x++)
			{
				chunkPtr[x] = conversionTable[chunkPtr[x]];
			}
		}
		else if(n < soundStart)
		{
			// This is a sprite
			uint16_t* spritePtr = (uint16_t*)(chunkPtr);
			uint16_t leftpix = *spritePtr++;
			uint16_t rightpix = *spritePtr++;

			if (leftpix >= 64 || rightpix >= 64 || leftpix > rightpix)
			{
				continue;
			}
			uint16_t firstTableDataOffset = *spritePtr;
			int numTables = rightpix - leftpix + 1;
			
			for(int x = 2 * numTables + 4; x < firstTableDataOffset; x++)
			{
				chunkPtr[x] = conversionTable[chunkPtr[x]];
			}
		}
	}
	
	FILE* fs = fopen(filename, "wb");
	fwrite(data, 1, dataLength, fs);
	fclose(fs);
}

int main(int argc, char** argv)
{
	GeneratePatternsRGB();
	GenerateCompositePaletteRGB();
	GenerateCGAPaletteRGB();

	GenerateLUT();
	
	if(argc == 2)
	{
		FILE* fs = fopen(argv[1], "rb");
		//FILE* fo = fopen("CSWAP.WL6", "wb");
		//FILE* fx = fopen("XSWAP.WL6", "wb");
		
		if(fs)
		{
			fseek(fs, 0, SEEK_END);
			long dataSize = ftell(fs);
			uint8_t* data = new uint8_t[dataSize];
			fseek(fs, 0, SEEK_SET);
			fread(data, 1, dataSize, fs);
			
			GenerateDataFile("XSWAP.WL6", data, dataSize, convertLUTComposite);
			GenerateDataFile("CSWAP.WL6", data, dataSize, convertLUT);
			
			fclose(fs);
			
			/*int index = 0;
			uint8_t data;
			
			while(!feof(fs))
			{
				if(fread(&data, 1, 1, fs))
				{
					uint8_t cdata = data;
					uint8_t xdata = data;
					if(index >= 64 * 64 && index < (106+1) * 64 * 64)
					{
						cdata = convertLUT[data];
						xdata = convertLUTComposite[data];
					}
					fwrite(&cdata, 1, 1, fo);
					fwrite(&xdata, 1, 1, fx);
				}
				index++;
			}
			
			fclose(fs);
			fclose(fo);
			fclose(fx);*/
		}
		
	}
	return 0;
}

int main2(int argc, char** argv)
{
	GeneratePatternsRGB();
	GenerateCompositePaletteRGB();
	GenerateCGAPaletteRGB();
	
	vector<uint8_t> pixels;
	unsigned width, height;
	
	if(argc < 2)
	{
		printf("Not enough arguments\n");
		return -1;
	}
	
	if(lodepng::decode(pixels, width, height, argv[1]))
	{
		printf("Could not load %s\n", argv[1]);
		return -1;
	}
	
	{
		vector<uint8_t> compositeOutputPixels;
		vector<uint8_t> outputPixels;
		unsigned outputWidth, outputHeight;
		
		outputWidth = 4 * (width / 4);
		outputHeight = height;
		
		for(int y = 0; y < outputHeight; y++)
		{
			for(int x = 0; x < outputWidth; x += 4)
			{
				uint8_t rgb[3];
				int index = (y - (y & 1)) * width + x;
				rgb[0] = pixels[index * 4];
				rgb[1] = pixels[index * 4 + 1];
				rgb[2] = pixels[index * 4 + 2];
				int match = FindClosestPaletteEntry(rgb, patternsRGB, NUM_PATTERNS, patternsRGBWeights);
				
				/*
				for(int n = 0; n < 4; n++)
				{
					outputPixels.push_back(patternsRGB[match * 3]);
					outputPixels.push_back(patternsRGB[match * 3 + 1]);
					outputPixels.push_back(patternsRGB[match * 3 + 2]);
					outputPixels.push_back(255);
				}*/
				
				for(int n = 0; n < 4; n++)
				{
					int p = patterns[match * 4 + n];
					outputPixels.push_back(palette[p * 3]);
					outputPixels.push_back(palette[p * 3 + 1]);
					outputPixels.push_back(palette[p * 3 + 2]);
					outputPixels.push_back(255);
				}
				
				int compositeMatch = FindClosestPaletteEntry(rgb, compositePatternRGB, 256, compositePatternRGBWeights);
				int first = compositeMatch & 0xf;
				int second = (compositeMatch >> 4) & 0xf;
				
				//first = second = FindClosestPaletteEntry(rgb, compositePalette, 16);

				for(int n = 0; n < 2; n++)
				{
					compositeOutputPixels.push_back(compositePalette[first * 3]);
					compositeOutputPixels.push_back(compositePalette[first * 3 + 1]);
					compositeOutputPixels.push_back(compositePalette[first * 3 + 2]);
					compositeOutputPixels.push_back(255);
				}
				for(int n = 0; n < 2; n++)
				{
					compositeOutputPixels.push_back(compositePalette[second * 3]);
					compositeOutputPixels.push_back(compositePalette[second * 3 + 1]);
					compositeOutputPixels.push_back(compositePalette[second * 3 + 2]);
					compositeOutputPixels.push_back(255);
				}
			}
		}
		
		
		lodepng::encode("out.png", outputPixels, outputWidth, outputHeight);
		lodepng::encode("cout.png", compositeOutputPixels, outputWidth, outputHeight);
	}

	{
		vector<uint8_t> outputPixels;
		vector<uint8_t> compositeOutputPixels;

		unsigned outputWidth = width, outputHeight = height;
		
		for(int y = 0; y < outputHeight; y++)
		{
			for(int x = 0; x < outputWidth; x++)
			{
				uint8_t rgb[3];
				int index = y * width + x;
				rgb[0] = pixels[index * 4];
				rgb[1] = pixels[index * 4 + 1];
				rgb[2] = pixels[index * 4 + 2];
				int match = FindClosestPaletteEntry(rgb, patternsRGB, NUM_PATTERNS, patternsRGBWeights);
				int patternIndex = (x) & 3;
				
				if(y & 1)
				{
					patternIndex = (patternIndex + 1) & 3;
				}
				
				int p = patterns[match * 4 + patternIndex];
				outputPixels.push_back(palette[p * 3]);
				outputPixels.push_back(palette[p * 3 + 1]);
				outputPixels.push_back(palette[p * 3 + 2]);
				outputPixels.push_back(255);
				
				if((x & 1) == 0)
				{
					rgb[0] = (uint8_t)((pixels[index * 4] + pixels[index * 4 + 4]) / 2);
					rgb[1] = (uint8_t)((pixels[index * 4 + 1] + pixels[index * 4 + 5]) / 2);
					rgb[2] = (uint8_t)((pixels[index * 4 + 2] + pixels[index * 4 + 6]) / 2);
					int compositeMatch = FindClosestPaletteEntry(rgb, compositePalette, 16);
					
					for(int n = 0; n < 2; n++)
					{
						compositeOutputPixels.push_back(compositePalette[compositeMatch * 3]);
						compositeOutputPixels.push_back(compositePalette[compositeMatch * 3 + 1]);
						compositeOutputPixels.push_back(compositePalette[compositeMatch * 3 + 2]);
						compositeOutputPixels.push_back(255);
					}
				}
			}
		}
		lodepng::encode("out2.png", outputPixels, outputWidth, outputHeight);
		lodepng::encode("cout2.png", compositeOutputPixels, outputWidth, outputHeight);

	}

	{
		vector<uint8_t> outputPixels;
		vector<uint8_t> outputPixels2;
		unsigned outputWidth, outputHeight;
		
		outputWidth = 4 * (width / 4);
		outputHeight = height;
		
		for(int y = 0; y < outputHeight; y++)
		{
			for(int x = 0; x < outputWidth; x += 4)
			{
				uint8_t rgb[3];
				int index = (y - (y & 1)) * width + x;
				rgb[0] = pixels[index * 4];
				rgb[1] = pixels[index * 4 + 1];
				rgb[2] = pixels[index * 4 + 2];
				int match = FindClosestPaletteEntry(rgb, cgaPatternRGB, 256, cgaPatternRGBWeights);
				int first, second;
				
				if(y & 1)
				{
					second = match & 0xf;
					first = (match >> 4) & 0xf;
				}
				else
				{
					first = match & 0xf;
					second = (match >> 4) & 0xf;
				}
				
				for(int n = 0; n < 2; n++)
				{
					outputPixels.push_back(cgaPalette[first * 3]);
					outputPixels.push_back(cgaPalette[first * 3 + 1]);
					outputPixels.push_back(cgaPalette[first * 3 + 2]);
					outputPixels.push_back(255);
					outputPixels.push_back(cgaPalette[second * 3]);
					outputPixels.push_back(cgaPalette[second * 3 + 1]);
					outputPixels.push_back(cgaPalette[second * 3 + 2]);
					outputPixels.push_back(255);
				}
				
				{
					uint8_t rgb2[3];
					rgb2[0] = pixels[index * 4 + 4];
					rgb2[1] = pixels[index * 4 + 5];
					rgb2[2] = pixels[index * 4 + 6];
					int match3 = FindClosestPaletteEntry(rgb, cgaPalette, 16);
					int match4 = FindClosestPaletteEntry(rgb2, cgaPalette, 16);
					
					if(match3 == match4)
					{
						for(int n = 0; n < 2; n++)
						{
							outputPixels2.push_back(cgaPalette[first * 3]);
							outputPixels2.push_back(cgaPalette[first * 3 + 1]);
							outputPixels2.push_back(cgaPalette[first * 3 + 2]);
							outputPixels2.push_back(255);
							outputPixels2.push_back(cgaPalette[second * 3]);
							outputPixels2.push_back(cgaPalette[second * 3 + 1]);
							outputPixels2.push_back(cgaPalette[second * 3 + 2]);
							outputPixels2.push_back(255);
						}
					}
					else
					{
						for(int n = 0; n < 2; n++)
						{
							outputPixels2.push_back(cgaPalette[match3 * 3]);
							outputPixels2.push_back(cgaPalette[match3 * 3 + 1]);
							outputPixels2.push_back(cgaPalette[match3 * 3 + 2]);
							outputPixels2.push_back(255);
						}						
						for(int n = 0; n < 2; n++)
						{
							outputPixels2.push_back(cgaPalette[match4 * 3]);
							outputPixels2.push_back(cgaPalette[match4 * 3 + 1]);
							outputPixels2.push_back(cgaPalette[match4 * 3 + 2]);
							outputPixels2.push_back(255);
						}						
					}
				}
			}
		}
		
		
		lodepng::encode("tout.png", outputPixels, outputWidth, outputHeight);
		lodepng::encode("tout2.png", outputPixels2, outputWidth, outputHeight);

	}
	
	return 0;
}