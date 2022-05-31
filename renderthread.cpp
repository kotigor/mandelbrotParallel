#include "renderthread.h"

#include <QImage>
#include <cmath>

//! [0]
RenderThread::RenderThread(int threadNumber, int numberOfThreads, QObject *parent)
    : QThread(parent)
{
    this->threadNumber = threadNumber;
    this->numberOfThreads = numberOfThreads;
    for (int i = 0; i < ColormapSize; ++i)
        colormap[i] = rgbFromWaveLength(380.0 + (i * 400.0 / ColormapSize));
}
//! [0]

//! [1]
RenderThread::~RenderThread()
{
    mutex.lock();
    abort = true;
    condition.wakeOne();
    mutex.unlock();

    wait();
}
//! [1]

//! [2]
void RenderThread::render(double centerX, double centerY, double scaleFactor,
                          QSize resultSize, double devicePixelRatio, bool isMandelbrot, double realC, double imC)
{
    QMutexLocker locker(&mutex);

    this->centerX = centerX;
    this->centerY = centerY;
    this->scaleFactor = scaleFactor;
    this->devicePixelRatio = devicePixelRatio;
    this->resultSize = resultSize;
    this->isMandelbrot = isMandelbrot;
    this->realC = realC;
    this->imC = imC;

    if (!isRunning()) {
        start(LowPriority);
    } else {
        restart = true;
        condition.wakeOne();
    }
}
//! [2]

//! [3]
void RenderThread::run()
{
    forever {
        mutex.lock();
        const double devicePixelRatio = this->devicePixelRatio;
        const QSize resultSize = this->resultSize * devicePixelRatio;
        const double requestedScaleFactor = this->scaleFactor;
        const double scaleFactor = requestedScaleFactor / devicePixelRatio;
        const double centerX = this->centerX;
        const double centerY = this->centerY;
        double realC = this->realC;
        double imC = this->imC;
        bool isMandelbrot = this->isMandelbrot;
        mutex.unlock();

        int halfWidth = resultSize.width() / 2;
        int halfHeight = resultSize.height() / 2;
        QImage image(resultSize.width(), resultSize.height()/numberOfThreads, QImage::Format_RGB32);
        image.setDevicePixelRatio(devicePixelRatio);

        const int NumPasses = 5;
        int pass = 0;
        while (pass < NumPasses) {
            const int MaxIterations = (1 << (2 * pass + 6)) + 32;
            const int Limit = 4;
            bool allBlack = true;

            for (int y = -halfHeight + (resultSize.height() / numberOfThreads) * threadNumber; y < -halfHeight + (resultSize.height() / numberOfThreads) * (threadNumber + 1); ++y) {
                if (restart)
                    break;
                if (abort)
                    return;

                auto scanLine = reinterpret_cast<uint *>(image.scanLine(y + halfHeight - (resultSize.height() / numberOfThreads) * threadNumber));
                const double ay = centerY + (y * scaleFactor);

                for (int x = -halfWidth; x < halfWidth; ++x) {
                    const double ax = centerX + (x * scaleFactor);
                    double a1 = ax;
                    double b1 = ay;
                    int numIterations = 0;
                    double rC, iC;
                    if(!isMandelbrot){
                        rC = realC;
                        iC = imC;
                    }else{
                        rC = ax;
                        iC = ay;
                    }

                    do {
                        ++numIterations;
                        const double a2 = (a1 * a1) - (b1 * b1) + rC;
                        const double b2 = (2 * a1 * b1) + iC;
                        a1 = a2; b1 = b2;
                        if ((a2 * a2) + (b2 * b2) > Limit)
                            break;
                    } while (numIterations < MaxIterations);

                    if (numIterations < MaxIterations) {
                        *scanLine++ = colormap[numIterations % ColormapSize];
                        allBlack = false;
                    } else {
                        *scanLine++ = qRgb(0, 0, 0);
                    }
                }
            }

            if (allBlack && pass == 0) {
                pass = 4;
            } else {
                if (!restart)
                    emit renderedImage(threadNumber, requestedScaleFactor, image);
                ++pass;
            }
        }
        mutex.lock();
        if (!restart)
            condition.wait(&mutex);
        restart = false;
        mutex.unlock();
    }
}
//! [9]

//! [10]
uint RenderThread::rgbFromWaveLength(double wave)
{
    double r = 0;
    double g = 0;
    double b = 0;

    if (wave >= 380.0 && wave <= 440.0) {
        r = -1.0 * (wave - 440.0) / (440.0 - 380.0);
        b = 1.0;
    } else if (wave >= 440.0 && wave <= 490.0) {
        g = (wave - 440.0) / (490.0 - 440.0);
        b = 1.0;
    } else if (wave >= 490.0 && wave <= 510.0) {
        g = 1.0;
        b = -1.0 * (wave - 510.0) / (510.0 - 490.0);
    } else if (wave >= 510.0 && wave <= 580.0) {
        r = (wave - 510.0) / (580.0 - 510.0);
        g = 1.0;
    } else if (wave >= 580.0 && wave <= 645.0) {
        r = 1.0;
        g = -1.0 * (wave - 645.0) / (645.0 - 580.0);
    } else if (wave >= 645.0 && wave <= 780.0) {
        r = 1.0;
    }

    double s = 1.0;
    if (wave > 700.0)
        s = 0.3 + 0.7 * (780.0 - wave) / (780.0 - 700.0);
    else if (wave <  420.0)
        s = 0.3 + 0.7 * (wave - 380.0) / (420.0 - 380.0);

    r = std::pow(r * s, 0.8);
    g = std::pow(g * s, 0.8);
    b = std::pow(b * s, 0.8);
    return qRgb(int(r * 255), int(g * 255), int(b * 255));
}
//! [10]
