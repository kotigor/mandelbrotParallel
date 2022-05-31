#include "mandelbrotwidget.h"

#include <QPainter>
#include <QKeyEvent>
#include <QDebug>
#include <QCheckBox>
#include <math.h>
#include <QLabel>
//! [0]
const double DefaultCenterX = -0.637011;
const double DefaultCenterY = -0.0395159;
const double DefaultScale = 0.00403897;

const double ZoomInFactor = 0.8;
const double ZoomOutFactor = 1 / ZoomInFactor;
const int ScrollStep = 20;
//! [0]

//! [1]
MandelbrotWidget::MandelbrotWidget(QWidget *parent) :
    QWidget(parent),
    centerX(DefaultCenterX),
    centerY(DefaultCenterY),
    pixmapScale(DefaultScale),
    curScale(DefaultScale)
{
    numberOfThreads = QThread::idealThreadCount();
    for(int i = 0; i < numberOfThreads; ++i)
    {
        threadIsFinished.push_back(false);
        threads.push_back(new RenderThread(i, numberOfThreads));
        connect(&*threads[i], &RenderThread::renderedImage,
                this, &MandelbrotWidget::updatePixmap);
    }


    setWindowTitle(tr("Mandelbrot"));
#if QT_CONFIG(cursor)
    setCursor(Qt::CrossCursor);
#endif
    resize(550, 400);
    piecesOfImage.resize(numberOfThreads);
    sliderX = new QSlider(Qt::Horizontal, this);
    sliderX->setGeometry(0,30,200,25);
    sliderX->setMinimum(-200);
    sliderX->setMaximum(100);
    sliderY = new QSlider(Qt::Horizontal, this);
    sliderY->setGeometry(210, 30, 200, 25);
    sliderY->setMinimum(-100);
    sliderY->setMaximum(100);
    labelX = new QLabel("real", this);
    labelY = new QLabel("imaginary", this);
    labelX->setGeometry(0, 50, 100, 15);
    labelY->setGeometry(210, 50, 100, 15);
    isMandelbrotCheckBox = new QCheckBox(tr("mandelbrot"), this);
    isMandelbrotCheckBox->setGeometry(450, 30, 100, 10);
    connect(isMandelbrotCheckBox, &QCheckBox::stateChanged, this, &MandelbrotWidget::isMandelbrotChange);
    connect(sliderX, &QSlider::valueChanged, this, &MandelbrotWidget::realChange);
    connect(sliderY, &QSlider::valueChanged, this, &MandelbrotWidget::imChange);
}
//! [1]

//! [2]
void MandelbrotWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (pixmap.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, tr("Rendering initial image, please wait..."));
//! [2] //! [3]
        return;
//! [3] //! [4]
    }
//! [4]

//! [5]
    if (qFuzzyCompare(curScale, pixmapScale)) {
//! [5] //! [6]
        painter.drawPixmap(pixmapOffset, pixmap);
//! [6] //! [7]
    } else {
//! [7] //! [8]
        auto previewPixmap = qFuzzyCompare(pixmap.devicePixelRatioF(), qreal(1))
            ? pixmap
            : pixmap.scaled(pixmap.size() / pixmap.devicePixelRatioF(), Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);
        double scaleFactor = pixmapScale / curScale;
        int newWidth = int(previewPixmap.width() * scaleFactor);
        int newHeight = int(previewPixmap.height() * scaleFactor);
        int newX = pixmapOffset.x() + (previewPixmap.width() - newWidth) / 2;
        int newY = pixmapOffset.y() + (previewPixmap.height() - newHeight) / 2;

        painter.save();
        painter.translate(newX, newY);
        painter.scale(scaleFactor, scaleFactor);

        QRectF exposed = painter.transform().inverted().mapRect(rect()).adjusted(-1, -1, 1, 1);
        painter.drawPixmap(exposed, previewPixmap, exposed);
        painter.restore();
    }
//! [8] //! [9]

    QString text = tr("Use mouse wheel or the '+' and '-' keys to zoom. "
                      "Press and hold left mouse button to scroll.");
    QFontMetrics metrics = painter.fontMetrics();
    int textWidth = metrics.horizontalAdvance(text);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 127));
    painter.drawRect((width() - textWidth) / 2 - 5, 0, textWidth + 10, metrics.lineSpacing() + 5);
    painter.setPen(Qt::white);
    painter.drawText((width() - textWidth) / 2, metrics.leading() + metrics.ascent(), text);
//    painter.drawText(0, 30, tr("real"));
//    painter.drawText(210, 30, tr("imaginary"));
}
//! [9]

//! [10]

void MandelbrotWidget::recreateImage()
{
    for(int i = 0; i < numberOfThreads; ++i)
    {
        threadIsFinished[i] = false;
    }
}

void MandelbrotWidget::resizeEvent(QResizeEvent * /* event */)
{
    recreateImage();
    start_time = std::chrono::steady_clock::now();
    for(int i = 0; i < numberOfThreads; ++i)
    {
        threads[i]->render(centerX, centerY, curScale, size(), devicePixelRatioF(), isMandelbrot, realC, imC);
    }
}
//! [10]

//! [11]
void MandelbrotWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        zoom(ZoomInFactor);
        break;
    case Qt::Key_Minus:
        zoom(ZoomOutFactor);
        break;
    case Qt::Key_Left:
        scroll(-ScrollStep, 0);
        break;
    case Qt::Key_Right:
        scroll(+ScrollStep, 0);
        break;
    case Qt::Key_Down:
        scroll(0, -ScrollStep);
        break;
    case Qt::Key_Up:
        scroll(0, +ScrollStep);
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}
//! [11]

#if QT_CONFIG(wheelevent)
//! [12]
void MandelbrotWidget::wheelEvent(QWheelEvent *event)
{
    const int numDegrees = event->angleDelta().y() / 8;
    const double numSteps = numDegrees / double(15);
    zoom(pow(ZoomInFactor, numSteps));
}
//! [12]
#endif

//! [13]
void MandelbrotWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        lastDragPos = event->pos();
}
//! [13]

//! [14]
void MandelbrotWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = event->pos();
        update();
    }
}
//! [14]

//! [15]
void MandelbrotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pixmapOffset += event->pos() - lastDragPos;
        lastDragPos = QPoint();

        const auto pixmapSize = pixmap.size() / pixmap.devicePixelRatioF();
        int deltaX = (width() - pixmapSize.width()) / 2 - pixmapOffset.x();
        int deltaY = (height() - pixmapSize.height()) / 2 - pixmapOffset.y();
        scroll(deltaX, deltaY);
    }
}
//! [15]

//! [16]
void MandelbrotWidget::updatePixmap(int threadNumber, double scaleFactor, const QImage& image)
{
    if (!lastDragPos.isNull())
        return;
    threadIsFinished[threadNumber] = true;
    piecesOfImage[threadNumber] = image;
    if(!isAllThreadFinished())
        return;
    QImage image_sum(size(), image.format());
    QPainter p(&image_sum);
    for(int i = 0; i < numberOfThreads; ++i)
    {
        p.drawImage(0, image.height()*i, piecesOfImage[i]);
    }
    p.end();
    end_time = std::chrono::steady_clock::now();
    auto elapsed_ns = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    qDebug() << elapsed_ns.count() << " ns\n";

    pixmap = QPixmap::fromImage(image_sum);
    pixmapOffset = QPoint();
    lastDragPos = QPoint();
    pixmapScale = scaleFactor;
    update();
}
//! [16]

//! [17]
void MandelbrotWidget::zoom(double zoomFactor)
{
    curScale *= zoomFactor;
    update();
    recreateImage();
    start_time = std::chrono::steady_clock::now();
    for(int i = 0; i < numberOfThreads; ++i)
    {
        threads[i]->render(centerX, centerY, curScale, size(), devicePixelRatioF(), isMandelbrot, realC, imC);
    }
}
//! [17]

//! [18]
void MandelbrotWidget::scroll(int deltaX, int deltaY)
{
    centerX += deltaX * curScale;
    centerY += deltaY * curScale;
    update();
    recreateImage();
    start_time = std::chrono::steady_clock::now();
    for(int i = 0; i < numberOfThreads; ++i)
    {
        threads[i]->render(centerX, centerY, curScale, size(), devicePixelRatioF(), isMandelbrot, realC, imC);
    }
}

bool MandelbrotWidget::isAllThreadFinished()
{
    for(int i = 0; i < numberOfThreads; ++i)
    {
        if(!threadIsFinished[i])
        {
            return false;
        }
    }
    return true;
}
//! [18]


void MandelbrotWidget::isMandelbrotChange(int state)
{
    isMandelbrot = (bool)state;
    sliderX->setVisible(!isMandelbrot);
    sliderY->setVisible(!isMandelbrot);
    labelX->setVisible(!isMandelbrot);
    labelY->setVisible(!isMandelbrot);
    update();
    recreateImage();
    start_time = std::chrono::steady_clock::now();
    for(int i = 0; i < numberOfThreads; ++i)
    {
        threads[i]->render(centerX, centerY, curScale, size(), devicePixelRatioF(), isMandelbrot, realC, imC);
    }
}

void MandelbrotWidget::realChange(int newValue)
{
    realC = (double)newValue / 100;
    update();
    recreateImage();
    start_time = std::chrono::steady_clock::now();
    for(int i = 0; i < numberOfThreads; ++i)
    {
        threads[i]->render(centerX, centerY, curScale, size(), devicePixelRatioF(), isMandelbrot, realC, imC);
    }
}

void MandelbrotWidget::imChange(int newValue)
{
    imC = (double)newValue / 100;
    update();
    recreateImage();
    start_time = std::chrono::steady_clock::now();
    for(int i = 0; i < numberOfThreads; ++i)
    {
        threads[i]->render(centerX, centerY, curScale, size(), devicePixelRatioF(), isMandelbrot, realC, imC);
    }
}
