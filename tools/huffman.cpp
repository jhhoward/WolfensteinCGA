typedef struct
{
  uint16_t bit0,bit1;	// 0-255 is a character, > is a pointer to a node
} huffnode;

typedef struct
{
	int16_t width,height;
} pictabletype;


huffnode	grhuffman[255];

typedef uint8_t byte;
typedef uint16_t word;

unsigned huffbits[256];
unsigned long huffstring[256];
long counts[256];

huffnode nodearray[256];	// 256 nodes is worst case

void HuffExpand(byte *source, byte *dest, int32_t length, huffnode *hufftable);

void CountBytes (unsigned char *start, long length)
{
  long i;

  while (length--)
    counts[*start++]++;
}

int HuffFind(int value, huffnode* hufftable)
{
	for(int n = 0; n < 255; n++)
	{
		if(hufftable[n].bit0 == value || hufftable[n].bit1 == value)
		{
			return n;
		}
	}
	
	// Shouldn't ever get here
	printf("Bad huffman table\n");
	exit(1);
	return -1;
}

int32_t HuffCompress(uint8_t* source, int32_t length, uint8_t* dest, int32_t destLength, huffnode* hufftable)
{
	uint8_t outputWrite = 0;
	uint8_t outputMask = 1;
	int32_t outputLength = 0;
	
	for(int n = 0; n < length; n++)
	{
		uint8_t byteToCompress = source[n];

		int search = byteToCompress;
		int bits[256];
		int numBits = 0;
		
		int index = -1;// = HuffFind(search, hufftable);
		while(index != 254)
		{
			index = HuffFind(search, hufftable);
			if(hufftable[index].bit0 == search)
			{
				bits[numBits++] = 0;
			}
			else
			{
				bits[numBits++] = 1;
			}
			search = index + 256;
		}
		
		for(int x = numBits - 1; x >= 0; x--)
		{
			if(bits[x])
			{
				outputWrite |= outputMask;
			}
			
			if(outputMask == 0x80)
			{
				dest[outputLength++] = outputWrite;
				outputMask = 0x1;
				outputWrite = 0;
				if (outputLength >= destLength)
				{
					printf("Out of buffer space!\n");
					exit(1);
				}

			}
			else outputMask <<= 1;
		}
	}
	
	if(outputMask != 0x1)
	{
		dest[outputLength++] = outputWrite;
	}
	return outputLength;
}

bool TestRecompress(uint8_t* uncompressed, int32_t length, uint8_t* refCompressed, int32_t refCompressedLength, huffnode* table)
{
	uint8_t* buffer = new uint8_t[length];
	int32_t compressedLength = HuffCompress(uncompressed, length, buffer, length, table);
	
	printf("Reference size: %d bytes Recompressed: %d bytes\n", refCompressedLength, compressedLength);
	
	//if(compressedLength == refCompressedLength)
	{
		bool isMatching = true;
		for(int n = 0; n < compressedLength && n < refCompressedLength; n++)
		{
			if(buffer[n] != refCompressed[n])
			{
				printf("Byte at index %d differs\n", n);
				isMatching = false;
				return false;
			}
		}

		if (compressedLength == refCompressedLength)
		{
			printf("It's a match!\n");
			return true;
		}
	}

	uint8_t* uncompressedBuffer = new uint8_t[length];
	HuffExpand(buffer, uncompressedBuffer, length, table);
	for (int n = 0; n < length; n++)
	{
		if (uncompressedBuffer[n] != uncompressed[n])
		{
			printf("Byte at index %d differs\n", n);
			return false;
		}
	}
	
	return false;
}

void HuffExpand(byte *source, byte *dest, int32_t length, huffnode *hufftable)
{
    byte *end;
    huffnode *headptr, *huffptr;

    if(!length || !dest)
    {
        printf("length or dest is null!");
		exit(1);
        return;
    }

    headptr = hufftable+254;        // head node is always node 254

    int written = 0;

    end=dest+length;

    byte val = *source++;
    byte mask = 1;
    word nodeval;
    huffptr = headptr;
    while(1)
    {
        if(!(val & mask))
            nodeval = huffptr->bit0;
        else
            nodeval = huffptr->bit1;
        if(mask==0x80)
        {
            val = *source++;
            mask = 1;
        }
        else mask <<= 1;

        if(nodeval<256)
        {
            *dest++ = (byte) nodeval;
            written++;
            huffptr = headptr;
            if(dest>=end) break;
        }
        else
        {
            huffptr = hufftable + (nodeval - 256);
        }
    }
}

void TraceNode (int nodenum,int numbits,unsigned long bitstring)
{
  unsigned bit0,bit1;

  bit0 = nodearray[nodenum].bit0;
  bit1 = nodearray[nodenum].bit1;

  numbits++;

  if (bit0 <256)
  {
    huffbits[bit0]=numbits;
    huffstring[bit0]=bitstring;		// just added a zero in front
    if (huffbits[bit0]>24 && counts[bit0])
    {
      puts("Error: Huffman bit string went over 32 bits!");
      exit(1);
    }
  }
  else
    TraceNode (bit0-256,numbits,bitstring);

  if (bit1 <256)
  {
    huffbits[bit1]=numbits;
    huffstring[bit1]=bitstring+ (1ul<<(numbits-1));	// add a one in front
    if (huffbits[bit1]>24 && counts[bit1])
    {
      puts("Error: Huffman bit string went over 32 bits!");
      exit(1);
    }
  }
  else
    TraceNode (bit1-256,numbits,bitstring+(1ul<<(numbits-1)));
}

void Huffmanize (void)
{

//
// codes are either bytes if <256 or nodearray numbers+256 if >=256
//
  unsigned value[256],code0,code1;
//
// probablilities are the number of times the code is hit or $ffffffff if
// it is allready part of a higher node
//
  unsigned long prob[256],low,workprob;

  int i,worknode,bitlength;
  unsigned long bitstring;


//
// all possible leaves start out as bytes
//
  for (i=0;i<256;i++)
  {
    value[i]=i;
    prob[i]=counts[i];
  }

//
// start selecting the lowest probable leaves for the ends of the tree
//

  worknode = 0;
  while (1)	// break out of when all codes have been used
  {
    //
    // find the two lowest probability codes
    //

    code0=0xffff;
    low = 0x7ffffffff;
    for (i=0;i<256;i++)
      if (prob[i]<low)
      {
	code0 = i;
	low = prob[i];
      }

    code1=0xffff;
    low = 0x7fffffff;
    for (i=0;i<256;i++)
      if (prob[i]<low && i != code0)
      {
	code1 = i;
	low = prob[i];
      }

    if (code1 == 0xffff)
    {
      if (value[code0]<256)
	  {
		  printf("Wierdo huffman error: last code wasn't a node!");
		  exit(1);
	  }
      if (value[code0]-256 != 254)
	  {
		printf("Wierdo huffman error: headnode wasn't 254!");
		exit(1);
	  }
      break;
    }

    //
    // make code0 into a pointer to work
    // remove code1 (make 0xffffffff prob)
    //
    nodearray[worknode].bit0 = value[code0];
    nodearray[worknode].bit1 = value[code1];

    value[code0] = 256 + worknode;
    prob[code0] += prob[code1];
    prob[code1] = 0xffffffff;
    worknode++;
  }

//
// done with tree, now build table recursively
//

  TraceNode (254,0,0);

}

long HuffCompress (unsigned char* source, long length,
  unsigned char *dest)
{
  long outlength;
  uint32_t string;
  unsigned biton,bits;
  unsigned char bytedata;

  outlength = biton = 0;

  *(long*)dest=0;		// so bits can be or'd on

  while (length--)
  {
    bytedata = *source++;
    bits = huffbits[bytedata];
    string = huffstring[bytedata] << biton;
    *(long*)(dest+1)=0;	// so bits can be or'd on
    *(long*)dest |= string;
    biton += bits;		// advance this many bits
    dest+= biton/8;
    biton&=7;			// stay under 8 shifts
    outlength+=bits;
  }

  return (outlength+7)/8;
}
