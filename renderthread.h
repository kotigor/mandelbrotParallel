#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include "qimage.h"
#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>


QT_BEGIN_NAMESPACE
class QImage;
QT_END_NAMESPACE

//! [0]
class RenderThread : public QThread
{
    Q_OBJECT

public:
    RenderThread(int threadNumber, int numberOfThreads, QObject *parent = nullptr);
    ~RenderThread();

    void render(double centerX, double centerY, double scaleFactor, QSize resultSize,
                double devicePixelRatio, bool isMandelbrot, double realC, double imC);

signals:
    void renderedImage(int threadNumber, double scaleFactor, const QImage &image);

protected:
    void run() override;

private:
    static uint rgbFromWaveLength(double wave);

    QMutex mutex;
    QWaitCondition condition;
    double centerX;
    double centerY;
    double scaleFactor;
    double devicePixelRatio;
    QSize resultSize;
    bool restart = false;
    bool abort = false;

    int threadNumber;
    int numberOfThreads;

    bool isMandelbrot;
    double realC = 0;
    double imC = 0;

    enum { ColormapSize = 512 };
    uint colormap[ColormapSize];
};
//! [0]

#endif // RENDERTHREAD_H
