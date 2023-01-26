// +-------------------------------------------< PREPROCESSING >--------------------------------------------+

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

// +----------------------------------------------< INCLUDE >-----------------------------------------------+

#include <Windows.h>

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// +------------------------------------------< TYPE DEFINITION >-------------------------------------------+

typedef uint8_t byte_t;

// +------------------------------------------< GLOBAL VARIABLE >-------------------------------------------+

static const size_t WIDTH  = 512;
static const size_t HEIGHT = 512;

byte_t* inputImage;
byte_t* dilationEdgeImage;
byte_t* erosionEdgeImage;

// +----------------------------------------------< UTILITY >-----------------------------------------------+

byte_t* Normalization(byte_t* inputImage, byte_t* outputImage)
{
    assert(inputImage  != NULL);
    assert(outputImage != NULL);

    double maxValue = static_cast<double>(*std::max_element(inputImage, inputImage + WIDTH * HEIGHT));
    double minValue = static_cast<double>(*std::min_element(inputImage, inputImage + WIDTH * HEIGHT));

    for (int iy = 0; iy < HEIGHT; ++iy)
        for (int ix = 0; ix < WIDTH; ++ix)
            outputImage[iy * WIDTH + ix] = static_cast<byte_t>(255 * (inputImage[iy * WIDTH + ix] - minValue) / (maxValue - minValue));

    return outputImage;
}

byte_t* MaxEdgeRatioThreshold(byte_t* inputImage, byte_t* outputImage, const double edgeRatio = 0.2)
{
    assert(inputImage  != NULL);
    assert(outputImage != NULL);
    assert(edgeRatio > 0.0 && edgeRatio <= 1.0);
    
    uint32_t histogram[256] = { 0 };
    uint32_t histogramCount = 0;
    byte_t   threshold      = 0;

    for (unsigned int iy = 0; iy < HEIGHT; ++iy)
        for (unsigned int ix = 0; ix < WIDTH; ++ix)
            histogram[inputImage[iy * WIDTH + ix]]++;

    for (int brightness = 255; brightness >= 0; --brightness)
        if ((histogramCount += histogram[brightness]) > WIDTH * HEIGHT * edgeRatio)
        {
            threshold = brightness + 1;
            break;
        }

    for (int iy = 0; iy < HEIGHT; ++iy)
        for (int ix = 0; ix < WIDTH; ++ix)
            outputImage[iy * WIDTH + ix] = (inputImage[iy * WIDTH + ix] >= threshold) ? (0) : (255);

    return outputImage;
}

byte_t CalculateWindowMax(byte_t* image, POINT center, SIZE wsize)
{
    assert(image != NULL);
    assert(center.x >= wsize.cx / 2 && center.x < WIDTH - wsize.cx / 2);
    assert(center.y >= wsize.cy / 2 && center.y < HEIGHT - wsize.cy / 2);
    assert(wsize.cx % 2 == 1);
    assert(wsize.cy % 2 == 1);

    byte_t maxValue = 0;

    for (int wy = -wsize.cy / 2; wy <= wsize.cy / 2; ++wy)
        for (int wx = -wsize.cx / 2; wx <= wsize.cx / 2; ++wx)
            if (image[(center.y + wy) * WIDTH + (center.x + wx)] > maxValue)
                maxValue = image[(center.y + wy) * WIDTH + (center.x + wx)];

    return maxValue;
}

byte_t CalculateWindowMin(byte_t* image, POINT center, SIZE wsize)
{
    assert(image != NULL);
    assert(center.x >= wsize.cx / 2 && center.x < WIDTH - wsize.cx / 2);
    assert(center.y >= wsize.cy / 2 && center.y < HEIGHT - wsize.cy / 2);
    assert(wsize.cx % 2 == 1);
    assert(wsize.cy % 2 == 1);

    byte_t minValue = UCHAR_MAX;

    for (int wy = -wsize.cy / 2; wy <= wsize.cy / 2; ++wy)
        for (int wx = -wsize.cx / 2; wx <= wsize.cx / 2; ++wx)
            if (image[(center.y + wy) * WIDTH + (center.x + wx)] < minValue)
                minValue = image[(center.y + wy) * WIDTH + (center.x + wx)];

    return minValue;
}

// +----------------------------------------------< DILATION >----------------------------------------------+

byte_t* DilationEdge(byte_t* inputImage, byte_t* outputImage, SIZE wsize)
{
    assert(inputImage   != NULL);
    assert(outputImage  != NULL);
    assert(wsize.cx % 2 == 1);
    assert(wsize.cy % 2 == 1);

    memset(outputImage, 0, sizeof(byte_t) * WIDTH * HEIGHT);

    for (int iy = wsize.cy / 2; iy < HEIGHT - wsize.cy / 2; ++iy)
        for (int ix = wsize.cx / 2; ix < WIDTH - wsize.cx / 2; ++ix)
            outputImage[iy * WIDTH + ix] = CalculateWindowMax(inputImage, { ix, iy }, wsize) - inputImage[iy * WIDTH + ix];

    Normalization(outputImage, outputImage);

    return outputImage;
}

// +----------------------------------------------< EROSION >-----------------------------------------------+

byte_t* ErosionEdge(byte_t* inputImage, byte_t* outputImage, SIZE wsize)
{
    assert(inputImage   != NULL);
    assert(outputImage  != NULL);
    assert(wsize.cx % 2 == 1);
    assert(wsize.cy % 2 == 1);

    memset(outputImage, 0, sizeof(byte_t) * WIDTH * HEIGHT);

    for (int iy = wsize.cy / 2; iy < HEIGHT - wsize.cy / 2; ++iy)
        for (int ix = wsize.cx / 2; ix < WIDTH - wsize.cx / 2; ++ix)
            outputImage[iy * WIDTH + ix] = inputImage[iy * WIDTH + ix] - CalculateWindowMin(inputImage, { ix, iy }, wsize);

    Normalization(outputImage, outputImage);

    return outputImage;
}

// +------------------------------------------------< MAIN >------------------------------------------------+

int main(void)
{
    static const char* INPUT_RAW_FILE_NAME                = "Lena.raw";
    static const char* OUTPUT_DILATION_EDGE_RAW_FILE_NAME = "Lena_DilationEdge.raw";
    static const char* OUTPUT_EROSION_EDGE_RAW_FILE_NAME  = "Lena_ErosionEdge.raw";

    FILE* fileStream;

    inputImage        = new byte_t[WIDTH * HEIGHT];
    dilationEdgeImage = new byte_t[WIDTH * HEIGHT];
    erosionEdgeImage  = new byte_t[WIDTH * HEIGHT];
    
    fileStream = fopen(INPUT_RAW_FILE_NAME, "rb");
    fread(inputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);
    
    DilationEdge(inputImage, dilationEdgeImage, { 5, 5 });
    MaxEdgeRatioThreshold(dilationEdgeImage, dilationEdgeImage, 0.2);
    ErosionEdge(inputImage, erosionEdgeImage, { 5, 5 });
    MaxEdgeRatioThreshold(erosionEdgeImage, erosionEdgeImage, 0.2);

    fileStream = fopen(OUTPUT_DILATION_EDGE_RAW_FILE_NAME, "w+b");
    fwrite(dilationEdgeImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    fileStream = fopen(OUTPUT_EROSION_EDGE_RAW_FILE_NAME, "w+b");
    fwrite(erosionEdgeImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    delete[] inputImage;
    delete[] dilationEdgeImage;
    delete[] erosionEdgeImage;

    return 0;
}

// +------------------------------------------------< END >-------------------------------------------------+