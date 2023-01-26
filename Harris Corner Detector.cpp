// +-------------------------------------------< PREPROCESSING >--------------------------------------------+

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

// +----------------------------------------------< INCLUDE >-----------------------------------------------+

#include <Windows.h>

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

typedef uint8_t  byte_t;
typedef uint32_t lbyte_t;
typedef int32_t  mag_t;

// +------------------------------------------< GLOBAL VARIABLE >-------------------------------------------+

static const size_t WIDTH  = 550;
static const size_t HEIGHT = 550;

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

lbyte_t* CreateIntegralImage(mag_t* inputImage, lbyte_t* integralImage)
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

// +-------------------------------------------< HARRIS CORNER >--------------------------------------------+

byte_t* HarrisCorner(byte_t* inputImage, byte_t* outputImage, const int wsize, const double lamda = 0.05)
{
    assert(inputImage  != NULL);
    assert(outputImage != NULL);
    assert(wsize % 2   == 1);

    mag_t*   sobelMagnitudePowX = new mag_t[WIDTH * HEIGHT];
    mag_t*   sobelMagnitudePowY = new mag_t[WIDTH * HEIGHT];
    mag_t*   sobelMagnitudeXY   = new mag_t[WIDTH * HEIGHT];

    lbyte_t* integralImagePowX  = new lbyte_t[WIDTH * HEIGHT];
    lbyte_t* integralImagePowY  = new lbyte_t[WIDTH * HEIGHT];
    lbyte_t* integralImageXY    = new lbyte_t[WIDTH * HEIGHT];

    memset(outputImage, 0, sizeof(byte_t) * WIDTH * HEIGHT);

    Sobel(inputImage, sobelMagnitudePowX, SOBEL_X);
    Sobel(inputImage, sobelMagnitudePowY, SOBEL_Y);

    for (int iy = 0; iy < HEIGHT; ++iy)
        for (int ix = 0; ix < WIDTH; ++ix)
        {
            sobelMagnitudeXY[iy * WIDTH + ix]   = abs(sobelMagnitudePowX[iy * WIDTH + ix]) * abs(sobelMagnitudePowY[iy * WIDTH + ix]);
            sobelMagnitudePowX[iy * WIDTH + ix] = sobelMagnitudePowX[iy * WIDTH + ix] * sobelMagnitudePowX[iy * WIDTH + ix];
            sobelMagnitudePowY[iy * WIDTH + ix] = sobelMagnitudePowY[iy * WIDTH + ix] * sobelMagnitudePowY[iy * WIDTH + ix];
        }

    CreateIntegralImage(sobelMagnitudePowX, integralImagePowX);
    CreateIntegralImage(sobelMagnitudePowY, integralImagePowY);
    CreateIntegralImage(sobelMagnitudeXY, integralImageXY);

    for (int iy = wsize / 2; iy < HEIGHT - wsize / 2; ++iy)
        for (int ix = wsize / 2; ix < WIDTH - wsize / 2; ++ix)
        {
            double sobelMagnitudeMeanPowX = CalculateIntegralWindowAverage(integralImagePowX, { ix, iy }, { wsize, wsize });
            double sobelMagnitudeMeanPowY = CalculateIntegralWindowAverage(integralImagePowY, { ix, iy }, { wsize, wsize });
            double sobelMagnitudeMeanXY   = CalculateIntegralWindowAverage(integralImageXY, { ix, iy }, { wsize, wsize });

            if ((sobelMagnitudeMeanPowX * sobelMagnitudeMeanPowY - pow(sobelMagnitudeMeanXY, 2) - lamda * pow(sobelMagnitudeMeanPowX + sobelMagnitudeMeanPowY, 2)) > 0.01)
                outputImage[iy * WIDTH + ix] = 255;
        }

    delete[] sobelMagnitudePowX;
    delete[] sobelMagnitudePowY;
    delete[] sobelMagnitudeXY;

    delete[] integralImagePowX;
    delete[] integralImagePowY;
    delete[] integralImageXY;

    return outputImage;
}

// +------------------------------------------------< MAIN >------------------------------------------------+

int main(void)
{
    static const char* INPUT_RAW_FILE_NAME  = "Ctest.raw";
    static const char* OUTPUT_RAW_FILE_NAME = "Ctest_HarrisCorner.raw";

    FILE* fileStream;

    inputImage  = new byte_t[WIDTH * HEIGHT];
    outputImage = new byte_t[WIDTH * HEIGHT];

    fileStream = fopen(INPUT_RAW_FILE_NAME, "rb");
    fread(inputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    HarrisCorner(inputImage, outputImage, 5, 0.05);

    fileStream = fopen(OUTPUT_RAW_FILE_NAME, "w+b");
    fwrite(outputImage, sizeof(byte_t), WIDTH * HEIGHT, fileStream);
    fclose(fileStream);

    delete[] inputImage;
    delete[] outputImage;

    return 0;
}

// +------------------------------------------------< END >-------------------------------------------------+