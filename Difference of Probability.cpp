// +-------------------------------------------< PREPROCESSING >--------------------------------------------+

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

// +----------------------------------------------< INCLUDE >-----------------------------------------------+

#include <Windows.h>

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// +------------------------------------------< TYPE DEFINITION >-------------------------------------------+

typedef uint8_t  byte_t;
typedef uint32_t lbyte_t;

// +------------------------------------------< GLOBAL VARIABLE >-------------------------------------------+

static const size_t WIDTH  = 512;
static const size_t HEIGHT = 512;

byte_t* inputImage;
byte_t* outputImage;

// +----------------------------------------------< UTILITY >-----------------------------------------------+

double CalculateIntegralWindowAverage(lbyte_t* integralImage, POINT center, SIZE wsize)
{
    assert(integralImage != NULL);
    assert(center.x >= wsize.cx / 2 && center.x < WIDTH - wsize.cx / 2);
    assert(center.y >= wsize.cy / 2 && center.y < HEIGHT - wsize.cy / 2);
    assert(wsize.cx % 2 == 1);
    assert(wsize.cy % 2 == 1);

    long integralSum = integralImage[(center.y + wsize.cy / 2) * WIDTH + (center.x + wsize.cx / 2)];

    if (center.x > wsize.cx / 2)
        integralSum -= integralImage[(center.y + wsize.cy / 2) * WIDTH + (center.x - wsize.cx / 2 - 1)];
    if (center.y > wsize.cy / 2)
        integralSum -= integralImage[(center.y - wsize.cy / 2 - 1) * WIDTH + (center.x + wsize.cx / 2)];
    if (center.x > wsize.cx / 2 && center.y > wsize.cy / 2)
        integralSum += integralImage[(center.y - wsize.cy / 2 - 1) * WIDTH + (center.x - wsize.cx / 2 - 1)];

    return integralSum / (wsize.cx * wsize.cy);
}

lbyte_t* CreateIntegralImage(byte_t* inputImage, lbyte_t* integralImage)
{
    assert(inputImage    != NULL);
    assert(integralImage != NULL);

    for (int iy = 0; iy < HEIGHT; ++iy)
        integralImage[iy * WIDTH] = inputImage[iy * WIDTH];

    for (int iy = 0; iy < HEIGHT; ++iy)
        for (int ix = 1; ix < WIDTH; ++ix)
            integralImage[iy * WIDTH + ix] = inputImage[iy * WIDTH + ix] + integralImage[iy * WIDTH + (ix - 1)];

    for (int ix = 0; ix < WIDTH; ++ix)
        for (int iy = 1; iy < HEIGHT; ++iy)
            integralImage[iy * WIDTH + ix] += integralImage[(iy - 1) * WIDTH + ix];

    return integralImage;
}

byte_t* Normalization(double* inputImage, byte_t* outputImage)
{
    assert(inputImage  != NULL);
    assert(outputImage != NULL);

    double maxValue = *std::max_element(inputImage, inputImage + WIDTH * HEIGHT);
    double minValue = *std::min_element(inputImage, inputImage + WIDTH * HEIGHT);

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

// +-------------------------------------------------< DP >-------------------------------------------------+

byte_t* DPEdge(byte_t* inputImage, byte_t* outputImage, SIZE wsize)
{
    assert(inputImage   != NULL);
    assert(outputImage  != NULL);
    assert(wsize.cx % 2 == 1);
    assert(wsize.cy % 2 == 1);

    lbyte_t* integralImage = new lbyte_t[WIDTH * HEIGHT];
    double*  DPImage       = new double[WIDTH * HEIGHT];

    memset(outputImage, 255, sizeof(byte_t) * WIDTH * HEIGHT);
    memset(DPImage, 0, sizeof(double) * WIDTH * HEIGHT);

    CreateIntegralImage(inputImage, integralImage);

    for (int iy = wsize.cy / 2; iy < HEIGHT - wsize.cy / 2; ++iy)
        for (int ix = wsize.cx / 2; ix < WIDTH - wsize.cx / 2; ++ix)
            DPImage[iy * WIDTH + ix] = (CalculateWindowMax(inputImage, { ix, iy }, wsize) - inputImage[iy * WIDTH + ix]) / CalculateIntegralWindowAverage(integralImage, { ix, iy }, wsize);

    Normalization(DPImage, outputImage);

    delete[] integralImage;
    delete[] DPImage;

    return outputImage;
}

// +------------------------------------------------< MAIN >------------------------------------------------+

int main(void)
{
    static const char* INPUT_RAW_FILE_NAME  = "Lena.raw";
    static const char* OUTPUT_RAW_FILE_NAME = "Lena_DPEdge.raw";

    FILE* fileStream;

    inputImage  = new byte_t[WIDTH * HEIGHT];
    outputImage = new byte_t[WIDTH * HEIGHT];

    fileStream = fopen(INPUT_RAW_FILE_NAME, "rb");
    fread(inputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    DPEdge(inputImage, outputImage, { 5, 5 });
    MaxEdgeRatioThreshold(outputImage, outputImage, 0.2);

    fileStream = fopen(OUTPUT_RAW_FILE_NAME, "w+b");
    fwrite(outputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    delete[] inputImage;
    delete[] outputImage;

    return 0;
}

// +------------------------------------------------< END >-------------------------------------------------+