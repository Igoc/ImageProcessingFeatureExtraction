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

// +------------------------------------------< SOBEL DIRECTION >-------------------------------------------+

#define SOBEL_X 0
#define SOBEL_Y 1

// +------------------------------------------< TYPE DEFINITION >-------------------------------------------+

typedef uint8_t byte_t;
typedef int32_t mag_t;

// +------------------------------------------< GLOBAL VARIABLE >-------------------------------------------+

static const size_t WIDTH  = 512;
static const size_t HEIGHT = 512;

byte_t* inputImage;
byte_t* outputImage;

// +----------------------------------------------< UTILITY >-----------------------------------------------+

byte_t* Normalization(mag_t* inputImage, byte_t* outputImage)
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

// +-----------------------------------------------< SOBEL >------------------------------------------------+

mag_t CalculateSobelMagnitude(byte_t* inputImage, POINT center, const int direction)
{
    assert(inputImage != NULL);
    assert(center.x >= 1 && center.x < WIDTH - 1);
    assert(center.y >= 1 && center.y < HEIGHT - 1);
    assert(direction == SOBEL_X || direction == SOBEL_Y);

    static const int SOBEL_MASK_X[9] = { -1,  0,  1, -2,  0,  2, -1,  0,  1 };
    static const int SOBEL_MASK_Y[9] = { -1, -2, -1,  0,  0,  0,  1,  2,  1 };

    mag_t magnitude = 0;

    for (int wy = -1; wy <= 1; ++wy)
        for (int wx = -1; wx <= 1; ++wx)
            magnitude += inputImage[(center.y + wy) * WIDTH + (center.x + wx)] * (direction ? SOBEL_MASK_Y[(wy + 1) * 3 + (wx + 1)] : SOBEL_MASK_X[(wy + 1) * 3 + (wx + 1)]);

    return magnitude;
}

mag_t* Sobel(byte_t* inputImage, mag_t* sobelImage, const int direction)
{
    assert(inputImage != NULL);
    assert(sobelImage != NULL);
    assert(direction == SOBEL_X || direction == SOBEL_Y);

    memset(sobelImage, 0, sizeof(mag_t) * WIDTH * HEIGHT);

    for (int iy = 1; iy < HEIGHT - 1; ++iy)
        for (int ix = 1; ix < WIDTH - 1; ++ix)
            sobelImage[iy * WIDTH + ix] = CalculateSobelMagnitude(inputImage, { ix, iy }, direction);

    return sobelImage;
}

byte_t* SobelEdge(byte_t* inputImage, byte_t* outputImage)
{
    assert(inputImage  != NULL);
    assert(outputImage != NULL);

    mag_t* sobelImage = new mag_t[WIDTH * HEIGHT];
    mag_t* magnitudeX = new mag_t[WIDTH * HEIGHT];
    mag_t* magnitudeY = new mag_t[WIDTH * HEIGHT];

    memset(outputImage, 255, sizeof(byte_t) * WIDTH * HEIGHT);

    Sobel(inputImage, magnitudeX, SOBEL_X);
    Sobel(inputImage, magnitudeY, SOBEL_Y);

    for (int iy = 0; iy < HEIGHT; ++iy)
        for (int ix = 0; ix < WIDTH; ++ix)
            sobelImage[iy * WIDTH + ix] = abs(magnitudeX[iy * WIDTH + ix]) + abs(magnitudeY[iy * WIDTH + ix]);

    Normalization(sobelImage, outputImage);

    delete[] sobelImage;
    delete[] magnitudeX;
    delete[] magnitudeY;

    return outputImage;
}

// +------------------------------------------------< MAIN >------------------------------------------------+

int main(void)
{
    static const char* INPUT_RAW_FILE_NAME  = "Lena.raw";
    static const char* OUTPUT_RAW_FILE_NAME = "Lena_SobelEdge.raw";

    FILE* fileStream;

    inputImage  = new byte_t[WIDTH * HEIGHT];
    outputImage = new byte_t[WIDTH * HEIGHT];

    fileStream = fopen(INPUT_RAW_FILE_NAME, "rb");
    fread(inputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    SobelEdge(inputImage, outputImage);
    MaxEdgeRatioThreshold(outputImage, outputImage, 0.2);

    fileStream = fopen(OUTPUT_RAW_FILE_NAME, "w+b");
    fwrite(outputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    delete[] inputImage;
    delete[] outputImage;

    return 0;
}

// +------------------------------------------------< END >-------------------------------------------------+