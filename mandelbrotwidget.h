#ifndef MANDELBROTWIDGET_H
#define MANDELBROTWIDGET_H

#include <QPixmap>
#include <QWidget>
#include <QSlider>
#include <QCheckBox>
#include "qlabel.h"
#include "renderthread.h"


//! [0]
class MandelbrotWidget : public QWidget
{
    Q_OBJECT

public:
    MandelbrotWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
#if QT_CONFIG(wheelevent)
    void wheelEvent(QWheelEvent *event) override;
#endif
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void updatePixmap(int threadNumber, double scaleFactor, const QImage& image);
    void zoom(double zoomFactor);
    void isMandelbrotChange(int state);
    void realChange(int value);
    void imChange(int value);

private:
    void scroll(int deltaX, int deltaY);
    void recreateImage();
    bool isAllThreadFinished();

    QVector<RenderThread*> threads;
    QVector<bool> threadIsFinished;
    QVector<QImage> piecesOfImage;
    QPixmap pixmap;
    QPoint pixmapOffset;
    QPoint lastDragPos;
    double centerX;
    double centerY;
    double pixmapScale;
    double curScale;
    int numberOfThreads;
    std::chrono::steady_clock::time_point start_time, end_time;
    QSlider* sliderX;
    QSlider* sliderY;
    QLabel* labelX;
    QLabel* labelY;
    QCheckBox* isMandelbrotCheckBox;
    double realC = 0;
    double imC = 0;
    bool isMandelbrot = false;
};
//! [0]

#endif // MANDELBROTWIDGET_H
