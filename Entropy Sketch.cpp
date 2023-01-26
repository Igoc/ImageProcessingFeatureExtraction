// +-------------------------------------------< PREPROCESSING >--------------------------------------------+

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

// +----------------------------------------------< INCLUDE >-----------------------------------------------+

#include <Windows.h>

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// +------------------------------------------< TYPE DEFINITION >-------------------------------------------+

typedef uint8_t byte_t;

// +------------------------------------------< GLOBAL VARIABLE >-------------------------------------------+

static const size_t WIDTH  = 512;
static const size_t HEIGHT = 512;

byte_t* inputImage;
byte_t* outputImage;

// +----------------------------------------------< UTILITY >-----------------------------------------------+

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

byte_t* MinEdgeRatioThreshold(byte_t* inputImage, byte_t* outputImage, const double edgeRatio = 0.2)
{
    assert(inputImage  != NULL);
    assert(outputImage != NULL);
    assert(edgeRatio > 0.0 && edgeRatio <= 1.0);

    uint32_t histogram[256] = { 0 };
    uint32_t histogramCount = 0;
    byte_t   threshold      = 255;

    for (unsigned int iy = 0; iy < HEIGHT; ++iy)
        for (unsigned int ix = 0; ix < WIDTH; ++ix)
            histogram[inputImage[iy * WIDTH + ix]]++;

    for (int brightness = 0; brightness < 256; ++brightness)
        if ((histogramCount += histogram[brightness]) > WIDTH * HEIGHT * edgeRatio)
        {
            threshold = brightness - 1;
            break;
        }

    for (int iy = 0; iy < HEIGHT; ++iy)
        for (int ix = 0; ix < WIDTH; ++ix)
            outputImage[iy * WIDTH + ix] = (inputImage[iy * WIDTH + ix] <= threshold) ? (0) : (255);

    return outputImage;
}

// +-------------------------------------------< ENTROPY SKETCH >-------------------------------------------+

byte_t* EntropySketchEdge(byte_t* inputImage, byte_t* outputImage, SIZE wsize)
{
    assert(inputImage   != NULL);
    assert(outputImage  != NULL);
    assert(wsize.cx % 2 == 1);
    assert(wsize.cy % 2 == 1);

    double* entropyImage = new double[WIDTH * HEIGHT];

    memset(outputImage, 255, sizeof(byte_t) * WIDTH * HEIGHT);
    memset(entropyImage, 0, sizeof(double) * WIDTH * HEIGHT);

    for (int iy = wsize.cy / 2; iy < HEIGHT - wsize.cy / 2; ++iy)
        for (int ix = wsize.cx / 2; ix < WIDTH - wsize.cx / 2; ++ix)
        {
            double pixelSum = 0.0;

            for (int wy = -wsize.cy / 2; wy <= wsize.cy / 2; ++wy)
                for (int wx = -wsize.cx / 2; wx <= wsize.cx / 2; ++wx)
                    pixelSum += inputImage[(iy + wy) * WIDTH + (ix + wx)];

            for (int wy = -wsize.cy / 2; wy <= wsize.cy / 2; ++wy)
                for (int wx = -wsize.cx / 2; wx <= wsize.cx / 2; ++wx)
                    entropyImage[iy * WIDTH + ix] += log2(inputImage[(iy + wy) * WIDTH + (ix + wx)] / pixelSum) * inputImage[(iy + wy) * WIDTH + (ix + wx)] / pixelSum;
            entropyImage[iy * WIDTH + ix] = -entropyImage[iy * WIDTH + ix];
        }

    Normalization(entropyImage, outputImage);

    delete[] entropyImage;

    return outputImage;
}

// +------------------------------------------------< MAIN >------------------------------------------------+

int main(void)
{
    static const char* INPUT_RAW_FILE_NAME  = "Lena.raw";
    static const char* OUTPUT_RAW_FILE_NAME = "Lena_EntropySketchEdge.raw";

    FILE* fileStream;

    inputImage  = new byte_t[WIDTH * HEIGHT];
    outputImage = new byte_t[WIDTH * HEIGHT];

    fileStream = fopen(INPUT_RAW_FILE_NAME, "rb");
    fread(inputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    EntropySketchEdge(inputImage, outputImage, { 5, 5 });
    MinEdgeRatioThreshold(outputImage, outputImage, 0.2);

    fileStream = fopen(OUTPUT_RAW_FILE_NAME, "w+b");
    fwrite(outputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    delete[] inputImage;
    delete[] outputImage;

    return 0;
}

// +------------------------------------------------< END >-------------------------------------------------+