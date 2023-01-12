#include "lodepng.cpp"
#include <stdint.h>
#include <stdio.h>
#include "huffman.cpp"

using namespace std;

#define USE_ALL_DITHERS 1

enum CgaMode
{
	CGA_RGB,
	CGA_COMPOSITE,
	TANDY160,
	NUM_GFX_MODES
};

const char* headFilename[] =
{
	"CGAHEAD.WL6",
	"COMHEAD.WL6",
	"TGAHEAD.WL6"
};

const char* gfxFilename[] =
{
	"CGAGRAPH.WL6",
	"COMGRAPH.WL6",
	"TGAGRAPH.WL6"
};

#define NUM_PATTERNS (sizeof(patterns) / 4)
uint8_t patterns[] =
{
#if USE_ALL_DITHERS
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
#else
	0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,
#endif
};

uint8_t patternsShifted[] =
{
#if USE_ALL_DITHERS
	0, 0, 0, 0,
	0, 0, 0, 1,
	1, 0, 1, 0,
	0, 1, 1, 1,
	1, 1, 1, 1,
	3, 1, 1, 1,
	1, 3, 1, 3,
	3, 3, 3, 1,
	0, 0, 0, 2,
	2, 0, 2, 0,
	0, 2, 2, 2,
	2, 2, 2, 2,
	3, 2, 2, 2,
	2, 3, 2, 3,
	3, 3, 3, 2,
	3, 3, 3, 3,
	0, 0, 0, 0,
	0, 0, 0, 3,
	3, 0, 3, 0,
	0, 3, 3, 3,
	2, 1, 2, 1
#else
	0, 0, 0, 0,
	1, 1, 1, 1,
	2, 2, 2, 2,
	3, 3, 3, 3,
#endif

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
uint8_t convertLUTTandy[256];

uint8_t wolfPalette[256 * 3];

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
			
			int pairDistance = 0;
			for(int n = 0; n < 3; n++)
			{
				int dist = cgaPalette[x * 3 + n] - cgaPalette[y * 3 + n];
				pairDistance += dist * dist;
			}
			if(pairDistance > 240 * 240)
			{
				cgaPatternRGBWeights[index] = -1;	
			}
			
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
			
			wolfPalette[index * 3] = pixels[index * 4];
			wolfPalette[index * 3 + 1] = pixels[index * 4 + 1];
			wolfPalette[index * 3 + 2] = pixels[index * 4 + 2];
			
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
			
			int tandyMatch = FindClosestPaletteEntry(rgb, cgaPatternRGB, 256, cgaPatternRGBWeights);
			pattern = (uint8_t)(tandyMatch);
			convertLUTTandy[index] = pattern;
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

#define STARTPICS    3
#define NUMPICS      132

struct GraphChunk
{
	uint8_t* data;
	uint32_t dataSize;
	uint32_t uncompressedSize;
	uint8_t* uncompressedData;
	uint32_t headerOffset;
};

void ProcessGraphics(CgaMode gfxMode)
{
	FILE* dictionaryFile = fopen("VGADICT.WL6", "rb");
	if(!dictionaryFile)
	{
		printf("Could not open dictionary\n");
		exit(1);
	}
	fread(grhuffman, sizeof(grhuffman), 1, dictionaryFile);
	fclose(dictionaryFile);

	FILE* headFile = fopen("VGAHEAD.WL6", "rb");
	
	if(!headFile)
	{
		printf("Could not open head\n");
		exit(1);
	}

	fseek(headFile, 0, SEEK_END);
	long headerSize = ftell(headFile);
    fseek(headFile, 0, SEEK_SET);
	printf("Header file size: %d bytes\n", headerSize);
	
	if(headerSize % 3)
	{
		printf("File size not multiple of 3?\n");
		exit(1);
	}
	
	int numChunks = (headerSize / 3) - 1;
	
	GraphChunk* chunks = new GraphChunk[numChunks];
	
	printf("Num chunks: %d\n", numChunks);
	
	for(int n = 0; n < numChunks + 1; n++)
	{
		uint32_t offset = 0;
		
		for(int i = 0; i < 3; i++)
		{
			uint8_t offsetPart;
			fread(&offsetPart, 1, 1, headFile);
			offset |= (offsetPart << (i * 8));
		}
		
		if(n < numChunks)
		{
			chunks[n].headerOffset = offset;
		}
		if(n > 0)
		{
			chunks[n - 1].dataSize = offset - chunks[n - 1].headerOffset;
		}
		
		printf("Chunk %d: offset: %x\n", n, offset);
	}
	
	fclose(headFile);
	
	FILE* graphicsFile = fopen("VGAGRAPH.WL6", "rb");
	fseek(graphicsFile, 0, SEEK_END);
	long graphicsLength = ftell(graphicsFile);
    fseek(graphicsFile, 0, SEEK_SET);
	
	uint8_t* graphicsData = new uint8_t[graphicsLength];
	fread(graphicsData, 1, graphicsLength, graphicsFile);
	fclose(graphicsFile);
	
	
	for(int n = 0; n < numChunks; n++)
	{
		chunks[n].data = graphicsData + chunks[n].headerOffset;

		if (n < STARTPICS + NUMPICS)
		{
			chunks[n].uncompressedSize = *(uint32_t*)(chunks[n].data);
			chunks[n].uncompressedData = new uint8_t[chunks[n].uncompressedSize];

			HuffExpand(chunks[n].data + 4, chunks[n].uncompressedData, chunks[n].uncompressedSize, grhuffman);
			
			//TestRecompress(chunks[n].uncompressedData, chunks[n].uncompressedSize, chunks[n].compressedData, chunks[n].compressedSize - 4, grhuffman);
		}
		else
		{
			chunks[n].uncompressedSize = NULL;
			chunks[n].uncompressedData = NULL;
		}
	}
	
	printf("Graphics size: %d bytes\n", graphicsLength);
	
	pictabletype* pictable = (pictabletype*)(chunks[0].uncompressedData);
	
	for(int n = STARTPICS; n < STARTPICS + NUMPICS; n++)
	{
		pictabletype* picmetadata = &pictable[n - STARTPICS];
		printf("%d : %d x %d (%d bytes)\n", n, picmetadata->width, picmetadata->height, chunks[n].uncompressedSize);
		
		//if(n == 0)
		if(0)
		{
			vector<uint8_t> rgbData;
			rgbData.resize(picmetadata->width * picmetadata->height * 4);
			uint8_t* picData = chunks[n].uncompressedData;
			
			int inputIndex = 0;
			for(int plane = 0; plane < 4; plane++)
			{
				for(int y = 0; y < picmetadata->height; y++)
				{
					for(int x = 0; x < picmetadata->width / 4; x++)
					{
						int outputIndex = 4 * (y * picmetadata->width + (x * 4) + plane);
						uint8_t indexColour = picData[inputIndex];
						uint8_t* rgb = &wolfPalette[indexColour * 3];
						int patternMatch = FindClosestPaletteEntry(rgb, patternsRGB, NUM_PATTERNS, patternsRGBWeights);
						int patternIndex = 0;

						if (y & 1)
						{
							patternIndex = patternsShifted[patternMatch * 4 + plane];
						}
						else
						{
							patternIndex = patterns[patternMatch * 4 + plane];
						}

						//patternIndex = FindClosestPaletteEntry(rgb, palette, 4);
						
						rgbData[outputIndex] = palette[patternIndex * 3];
						rgbData[outputIndex + 1] = palette[patternIndex * 3 + 1];
						rgbData[outputIndex + 2] = palette[patternIndex * 3 + 2];
						rgbData[outputIndex + 3] = 255;
						
						inputIndex++;
						
						
					}
				}
			}
			
			char filename[50];
			sprintf(filename, "pic%d.png", n - STARTPICS);
			lodepng::encode(filename, rgbData, picmetadata->width, picmetadata->height);
		}

		{
			uint8_t* picData = chunks[n].uncompressedData;
			uint8_t* newPicData = new uint8_t[picmetadata->width * picmetadata->height];
			for (int y = 0; y < picmetadata->height; y++)
			{
				for (int x = 0; x < picmetadata->width / 4; x++)
				{
					uint8_t pixels = 0;

					for (int plane = 0; plane < 4; plane++)
					{
						int planeIndex = (picmetadata->height * picmetadata->width / 4) * plane + y * (picmetadata->width / 4) + x;
						uint8_t inputPixel = picData[planeIndex];

						uint8_t* rgb = &wolfPalette[inputPixel * 3];

						if(gfxMode == TANDY160)
						{
							if((plane & 1) == 0)
							{
								uint8_t rgbmerge[3];
								int planeIndex2 = (picmetadata->height * picmetadata->width / 4) * (plane + 1) + y * (picmetadata->width / 4) + x;
								uint8_t inputPixel2 = picData[planeIndex2];
								uint8_t* rgb2 = &wolfPalette[inputPixel2 * 3];
								
								rgbmerge[0] = (rgb[0] + rgb2[0]) / 2;
								rgbmerge[1] = (rgb[1] + rgb2[1]) / 2;
								rgbmerge[2] = (rgb[2] + rgb2[2]) / 2;

								int patternMatch = FindClosestPaletteEntry(rgbmerge, cgaPalette, 16);
								if(plane == 0)
								{
									pixels |= (patternMatch << 4);
								}
								else if(plane == 2)
								{
									pixels |= (patternMatch);
								}
							}
							
						}
						else if (gfxMode == CGA_RGB)
						{
							int patternMatch = FindClosestPaletteEntry(rgb, patternsRGB, NUM_PATTERNS, patternsRGBWeights);
							int patternIndex = 0;

							if (y & 1)
							{
								patternIndex = patternsShifted[patternMatch * 4 + plane];
							}
							else
							{
								patternIndex = patterns[patternMatch * 4 + plane];
							}

							//patternIndex = FindClosestPaletteEntry(rgb, palette, 4);

							pixels |= patternIndex << ((3 - plane) * 2);
						}
						else if(gfxMode == CGA_COMPOSITE)
						{
							int patternMatch = FindClosestPaletteEntry(rgb, compositePalette, 16);
							uint8_t doubled = patternMatch | (patternMatch << 4);
							
							uint8_t mask = 0xc0 >> (plane * 2);
							pixels |= (mask & doubled);
							/*if(plane == 0)
							{
								pixels |= (patternMatch << 4);
							}
							else if(plane == 2)
							{
								pixels |= patternMatch;
							}*/
							
						}
						else
						{
							int patternMatch = FindClosestPaletteEntry(rgb, compositePatternRGB, 256, compositePatternRGBWeights);
							uint8_t mask = 0x2 << ((3 - plane) * 2);
							pixels |= (mask & patternMatch);
						}
					}

					newPicData[y * (picmetadata->width / 4) + x] = pixels;
				}
			}

			int bufferSpace = picmetadata->width * picmetadata->height * 4;
			uint8_t* newCompressedData = new uint8_t[bufferSpace];
			uint32_t* ptr = (uint32_t*)(newCompressedData);
			*ptr = picmetadata->width * picmetadata->height;
			long compressedDataSize = HuffCompress(newPicData, (picmetadata->width / 4) * picmetadata->height, newCompressedData + 4, bufferSpace - 4, grhuffman);

			if (0)
			{
				if (compressedDataSize != chunks[n].dataSize - 4)
				{
					printf("Compressed size differs. Original: %d bytes New: %d bytes\n", chunks[n].dataSize - 4, compressedDataSize);
				}

				for (int x = 0; x < chunks[n].dataSize; x++)
				{
					if (chunks[n].data[x] != newCompressedData[x])
					{
						if (x >= compressedDataSize + 4)
						{
							printf("Byte %x Original: %x New: !\n", x, chunks[n].data[x]);
						}
						else
						{
							printf("Byte %x Original: %x New: %x\n", x, chunks[n].data[x], newCompressedData[x]);
						}
					}
				}
			}

			chunks[n].data = newCompressedData;
			chunks[n].dataSize = compressedDataSize + 4;


			//HuffExpand(newCompressedData + 4, picData, picmetadata->width * picmetadata->height, grhuffman);
		}
	}

	FILE* graphicsFileOut = fopen(gfxFilename[gfxMode], "wb");
	uint32_t totalGraphicsFileLength = 0;
	for (int n = 0; n < numChunks; n++)
	{
		fwrite(chunks[n].data, 1, chunks[n].dataSize, graphicsFileOut);
		chunks[n].headerOffset = totalGraphicsFileLength;
		totalGraphicsFileLength += chunks[n].dataSize;
	}
	fclose(graphicsFileOut);

	FILE* dictFileOut = fopen("CGADICT.WL6", "wb");
	fwrite(grhuffman, sizeof(grhuffman), 1, dictFileOut);
	fclose(dictFileOut);

	FILE* graphicsHeadOut = fopen(headFilename[gfxMode], "wb");
	for (int n = 0; n < numChunks; n++)
	{
		uint32_t offset = chunks[n].headerOffset;

		for (int n = 0; n < 3; n++)
		{
			uint8_t writeOut = (uint8_t)(offset & 0xff);
			fwrite(&writeOut, 1, 1, graphicsHeadOut);
			offset >>= 8;
		}
	}
	{
		uint32_t offset = totalGraphicsFileLength;

		for (int n = 0; n < 3; n++)
		{
			uint8_t writeOut = (uint8_t)(offset & 0xff);
			fwrite(&writeOut, 1, 1, graphicsHeadOut);
			offset >>= 8;
		}
	}
	fclose(graphicsHeadOut);
}

int main(int argc, char** argv)
{
	GeneratePatternsRGB();
	GenerateCompositePaletteRGB();
	GenerateCGAPaletteRGB();

	GenerateLUT();

	for(int n = 0; n < NUM_GFX_MODES; n++)
	{
		ProcessGraphics((CgaMode) n);
	}
	
	//if(argc == 2)
	{
		//FILE* fs = fopen(argv[1], "rb");
		//FILE* fo = fopen("CSWAP.WL6", "wb");
		//FILE* fx = fopen("XSWAP.WL6", "wb");
		FILE* fs = fopen("VSWAP.WL6", "rb");
		
		if(fs)
		{
			fseek(fs, 0, SEEK_END);
			long dataSize = ftell(fs);
			uint8_t* data = new uint8_t[dataSize];
			fseek(fs, 0, SEEK_SET);
			fread(data, 1, dataSize, fs);
			
			GenerateDataFile("XSWAP.WL6", data, dataSize, convertLUTComposite);
			GenerateDataFile("CSWAP.WL6", data, dataSize, convertLUT);
			GenerateDataFile("TSWAP.WL6", data, dataSize, convertLUTTandy);
			
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