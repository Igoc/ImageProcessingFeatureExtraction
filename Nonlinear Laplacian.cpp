// +-------------------------------------------< PREPROCESSING >--------------------------------------------+

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

// +----------------------------------------------< INCLUDE >-----------------------------------------------+

#include <Windows.h>

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
byte_t* unbiasEdgeImage;
byte_t* unbiasThresholdEdgeImage;

// +----------------------------------------------< UTILITY >-----------------------------------------------+

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

// +-----------------------------------------< LAPLACIAN UTILITY >------------------------------------------+

byte_t* FindZeroCrossing(int32_t* inputImage, byte_t* outputImage)
{
    assert(inputImage  != NULL);
    assert(outputImage != NULL);

    memset(outputImage, 255, sizeof(byte_t) * WIDTH * HEIGHT);

    for (int iy = 1; iy < HEIGHT - 1; ++iy)
        for (int ix = 1; ix < WIDTH - 1; ++ix)
        {
            if (inputImage[iy * WIDTH + ix] == 0 && inputImage[iy * WIDTH + (ix - 1)] * inputImage[iy * WIDTH + (ix + 1)] < 0)
                outputImage[iy * WIDTH + ix] = 0;

            if (inputImage[iy * WIDTH + ix] * inputImage[iy * WIDTH + (ix + 1)] < 0)
                outputImage[iy * WIDTH + ix] = 0;

            if (inputImage[iy * WIDTH + ix] == 0 && inputImage[(iy - 1) * WIDTH + ix] * inputImage[(iy + 1) * WIDTH + ix] < 0)
                outputImage[iy * WIDTH + ix] = 0;

            if (inputImage[iy * WIDTH + ix] * inputImage[(iy + 1) * WIDTH + ix] < 0)
                outputImage[iy * WIDTH + ix] = 0;
        }

    return outputImage;
}

byte_t* LocalVarianceThreshold(byte_t* inputImage, byte_t* inputUnbiasEdgeImage, byte_t* outputImage, SIZE wsize)
{
    assert(inputImage           != NULL);
    assert(inputUnbiasEdgeImage != NULL);
    assert(outputImage          != NULL);
    assert(wsize.cx % 2         == 1);
    assert(wsize.cy % 2         == 1);

    double* varianceImage = new double[WIDTH * HEIGHT];
    double  threshold     = 0.0;

    memset(outputImage, 255, sizeof(byte_t) * WIDTH * HEIGHT);
    memset(varianceImage, 0, sizeof(double) * WIDTH * HEIGHT);

    for (int iy = wsize.cy / 2; iy < HEIGHT - wsize.cy / 2; ++iy)
        for (int ix = wsize.cx / 2; ix < WIDTH - wsize.cx / 2; ++ix)
        {
            double mean = 0.0;

            for (int wy = -wsize.cy / 2; wy <= wsize.cy / 2; ++wy)
                for (int wx = -wsize.cx / 2; wx <= wsize.cx / 2; ++wx)
                    mean += inputImage[(iy + wy) * WIDTH + (ix + wx)];
            mean /= wsize.cx * wsize.cy;

            for (int wy = -wsize.cy / 2; wy <= wsize.cy / 2; ++wy)
                for (int wx = -wsize.cx / 2; wx <= wsize.cx / 2; ++wx)
                    varianceImage[iy * WIDTH + ix] += (inputImage[(iy + wy) * WIDTH + (ix + wx)] - mean) * (inputImage[(iy + wy) * WIDTH + (ix + wx)] - mean);
            varianceImage[iy * WIDTH + ix] /= wsize.cx * wsize.cy - 1;

            threshold += varianceImage[iy * WIDTH + ix];
        }

    threshold /= (WIDTH - wsize.cx + 1) * (HEIGHT - wsize.cy + 1);

    for (int iy = wsize.cy / 2; iy < HEIGHT - wsize.cy / 2; ++iy)
        for (int ix = wsize.cx / 2; ix < WIDTH - wsize.cx / 2; ++ix)
            if (varianceImage[iy * WIDTH + ix] >= threshold && inputUnbiasEdgeImage[iy * WIDTH + ix] == 0)
                outputImage[iy * WIDTH + ix] = 0;

    delete[] varianceImage;

    return outputImage;
}

// +-----------------------------------------------< UNBIAS >-----------------------------------------------+

byte_t* UnbiasEdge(byte_t* inputImage, byte_t* outputImage, SIZE wsize)
{
    assert(inputImage   != NULL);
    assert(outputImage  != NULL);
    assert(wsize.cx % 2 == 1);
    assert(wsize.cy % 2 == 1);
    
    int32_t* unbiasImage = new int32_t[WIDTH * HEIGHT];

    memset(outputImage, 255, sizeof(byte_t) * WIDTH * HEIGHT);
    memset(unbiasImage, 0, sizeof(int32_t) * WIDTH * HEIGHT);

    for (int iy = wsize.cy / 2; iy < HEIGHT - wsize.cy / 2; ++iy)
        for (int ix = wsize.cx / 2; ix < WIDTH - wsize.cx / 2; ++ix)
            unbiasImage[iy * WIDTH + ix] = CalculateWindowMax(inputImage, { ix, iy }, wsize) + CalculateWindowMin(inputImage, { ix, iy }, wsize) - 2 * inputImage[iy * WIDTH + ix];

    FindZeroCrossing(unbiasImage, outputImage);

    delete[] unbiasImage;

    return outputImage;
}

// +------------------------------------------------< MAIN >------------------------------------------------+

int main(void)
{
    static const char* INPUT_RAW_FILE_NAME                        = "Lena.raw";
    static const char* OUTPUT_UNBIAS_EDGE_RAW_FILE_NAME           = "Lena_UnbiasEdge.raw";
    static const char* OUTPUT_UNBIAS_THRESHOLD_EDGE_RAW_FILE_NAME = "Lena_UnbiasThresholdEdge.raw";

    FILE* fileStream;

    inputImage               = new byte_t[WIDTH * HEIGHT];
    unbiasEdgeImage          = new byte_t[WIDTH * HEIGHT];
    unbiasThresholdEdgeImage = new byte_t[WIDTH * HEIGHT];

    fileStream = fopen(INPUT_RAW_FILE_NAME, "rb");
    fread(inputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    UnbiasEdge(inputImage, unbiasEdgeImage, { 5, 5 });
    LocalVarianceThreshold(inputImage, unbiasEdgeImage, unbiasThresholdEdgeImage, { 5, 5 });

    fileStream = fopen(OUTPUT_UNBIAS_EDGE_RAW_FILE_NAME, "w+b");
    fwrite(unbiasEdgeImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    fileStream = fopen(OUTPUT_UNBIAS_THRESHOLD_EDGE_RAW_FILE_NAME, "w+b");
    fwrite(unbiasThresholdEdgeImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    delete[] inputImage;
    delete[] unbiasEdgeImage;
    delete[] unbiasThresholdEdgeImage;

    return 0;
}

// +------------------------------------------------< END >-------------------------------------------------+