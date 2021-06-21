#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if (defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__))
    #define WINDOWS
#endif

typedef unsigned char ubyte;

ubyte* getSource(const char* path);
int decodePNG(const char* filename);

int main(int argc, char** argv)
{
    //Verify that there is an argument
    if(argc < 2)
    {
        return -1;
    }

    char* filename = argv[1];
    char format[5];
    int len = strlen(filename);

    //Get file ending
    for(int i = len - 1; i >= len - 4; --i)
    {
        format[i - len + 4] = filename[i];
    }

    //Check if it's ".png"
    if(strcmp(format, ".png") != 0)
    {
        printf("Filename does not end in '.png'. Decoding may fail.\n");
    }

    decodePNG(filename);

    return 0;
}

struct ImageInfo
{
    ubyte* data;
    int length;
    int width;
    int height;
    char depth;
    char colorType;
    char compression;
    char filter;
    char interlace;
};
typedef struct ImageInfo ImageInfo;

int getMultiByteNum(ubyte* data, int index, int bytes)
{
    int n = 0;

    for(int i = 0; i < bytes; ++i)
    {
        int a = (int) pow(0x100, bytes - i - 1);
        n += data[index + i] * a;
    }

    return n;
}

int verifyCRC(char* data, int index, int chunkLen)
{

}

int getIHDR(ubyte* data, int index, ImageInfo* info)
{
    info->width = getMultiByteNum(data, index, 4);
    index += 4;
    printf("\nWidth:  %d\n", info->width);

    info->height = getMultiByteNum(data, index, 4);
    index += 4;
    printf("Height: %d\n", info->height);

    if(info->width <= 0 || info->height <= 0)
    {
        fprintf(stderr, "Invalid dimensions!\n");
        return -1;
    }

    info->depth = data[index];
    printf("Bit Depth:  %d\n", info->depth);
    if((info->depth & info->depth - 1) != 0 || info->depth > 16) //(x & x - 1) != 0 checks if more than one bit is on.
    {
        fprintf(stderr, "Invalid bit depth!\n");
        return -1;
    }

    info->colorType = data[index + 1];
    printf("Color Type: %d\n", info->colorType);
    if((info->colorType % 2 == 1 && info->colorType != 3) || info->colorType > 8) //Honestly proud of this, I didn't "borrow" this one
    {
        fprintf(stderr, "Invalid color type!\n");
        return -1;
    }
    
    switch(info->colorType)
    {
        case 2:
        case 4:
        case 6:
            if(info->depth != 8 && info->depth != 16)
            {
                fprintf(stderr, "Invalid color type/depth combination!\n");
                return -1;
            }
            break;
        case 3:
            if((info->depth & info->depth - 1) != 0 || info->depth > 8)
            {
                fprintf(stderr, "Invalid color type/depth combination!\n");
                return -1;
            }
            break;
    }

    info->compression = data[index + 2];
    printf("Compression Method: %d\n", info->compression);
    if(info->compression != 0)
    {
        fprintf(stderr, "Unknown compression type (only 0 supported).\n");
        return -1;
    }

    info->filter = data[index + 3];
    printf("Filter Method:      %d\n", info->filter);
    if(info->filter != 0)
    {
        fprintf(stderr, "Unknown filter method (only 0 supported).\n");
        return -1;
    }

    info->interlace = data[index + 4];
    printf("Interlace Method:   %d\n", info->interlace);
    if(info->interlace > 1)
    {
        fprintf(stderr, "Invalid interlace method.\n");
        return -1;
    }
}

int getChunk(ubyte* data, int index, int* nextindex, ImageInfo* info)
{
    char type[5];
    int chunkLen = 0;

    //Get data chunk length
    chunkLen = getMultiByteNum(data, index, 4);
    index += 4;

    //Get the type
    memcpy(type, data + index, 4);
    index += 4;

    if(strcmp(type, "IHDR") == 0)
    {
        getIHDR(data, index, info);
    }

    verifyCRC(data, index, chunkLen);

    *nextindex = index + chunkLen + 12; //length (4 bytes) + type (4 bytes) + CRC (4 bytes) = 12 bytes

    return 1; //1 means more chunks left
}

int decodePNG(const char* filename)
{
    printf("Decoding %s ...\n", filename);

    ubyte* raw = getSource(filename);
    if(raw == NULL)
    {
        return -1;
    }

    const char* signature = "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A";
    int index = 8;
    int nextindex;
    int code = 0;

    ImageInfo info;

    //Check signature
    if(strncmp(signature, raw, 8) != 0)
    {
        fprintf(stderr, "Invalid PNG signature.\n");
        return 1;
    }
    printf("Signature verified.\n");
   

    //Decode data
    do
    {
        code = getChunk(raw, index, &nextindex);
    } while (/* condition */);
    

    while((code = getChunk(raw, &index, &info)) == 1);

    free(raw);
    return code;
}

ubyte* getSource(const char* path)
{
	//----------INITIALIZATION----------

	FILE* file = NULL;
	int length = 0;

	//Open the file (system dependent)
#ifdef WINDOWS
	fopen_s(&file, path, "r");
#else
	file = fopen(path, "r");
#endif

	if(file == NULL)
	{
        fprintf(stderr, "Error: Could not open %s.\n", path);
		return NULL;
	}

	//----------RETRIEVAL----------

	//Get the length of the file
	while(fgetc(file) != EOF)
	{
		++length;
	}
	rewind(file); //Start from the beginning of the file

	//Allocate a new string with size "length"
	ubyte* source = (ubyte*) malloc(sizeof(ubyte) * length);

	if(source == NULL)
	{
        printf("Error: Could not allocate space for file contents.\n");
		return NULL;
	}

	//Copy text to string
	for(int i = 0; i < length; ++i)
	{
		source[i] = fgetc(file);
	}

	//----------CLEAN-UP----------

	//Close the file
	fclose(file);

	return source;
}