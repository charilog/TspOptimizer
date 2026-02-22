#include "TspWidget.h"
#include <QPainter>
#include <QTransform>
#include <QMouseEvent>
#include <QEvent>
#include <cmath>

TspWidget::TspWidget(QWidget* parent)
: QWidget(parent)
{
    setMinimumSize(600, 500);
    setAutoFillBackground(true);
    setMouseTracking(true);
}

void TspWidget::setInstance(const TspInstance* instance)
{
    m_instance = instance;
    m_current.clear();
    m_last.clear();
    m_pan = QPointF(0.0, 0.0);
    m_dragging = false;
    unsetCursor();
    update();
}

void TspWidget::setTour(const QVector<int>& order)
{
    m_last = m_current;
    m_current = order;
    update();
}

void TspWidget::setLastTour(const QVector<int>& order)
{
    m_last = order;
    update();
}

void TspWidget::clearLastTour()
{
    m_last.clear();
    update();
}

void TspWidget::setBorderScale(double s)
{
    m_borderScale = std::max(0.1, s);
    update();
}

void TspWidget::setRotationDeg(int deg)
{
    deg %= 360;
    if (deg < 0) deg += 360;
    m_rotation = deg;
    update();
}

void TspWidget::setShowLines(bool show)
{
    m_showLines = show;
    update();
}

static inline QPointF mapPoint(const TspPoint& p, double x_scale, double y_scale, int32_t minX, int32_t minY)
{
    const double x = (static_cast<double>(p.x) * x_scale) - static_cast<double>(minX) * x_scale;
    const double y = (static_cast<double>(p.y) * y_scale) - static_cast<double>(minY) * y_scale;
    return QPointF(x, y);
}

static void drawTour(QPainter& painter, const TspInstance& inst, const QVector<int>& ord,
                     double x_scale, double y_scale, int32_t minX, int32_t minY, bool showLines)
{
    if (ord.isEmpty()) return;

    const auto& pts = inst.points();
    const int radius = 1;

    QPointF prev;
    bool hasPrev = false;

    for (int k = 0; k < ord.size(); ++k)
    {
        const int idx = ord[k];
        if (idx < 0 || idx >= static_cast<int>(pts.size()))
            continue;

        const QPointF p = mapPoint(pts[idx], x_scale, y_scale, minX, minY);

        painter.setBrush(painter.pen().color());
        painter.drawEllipse(p, radius, radius);
        painter.setBrush(Qt::NoBrush);

        if (showLines && hasPrev && k != 0)
            painter.drawLine(prev, p);

        prev = p;
        hasPrev = true;
    }
}


void TspWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TspWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton))
    {
        const QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();
        m_pan += QPointF(static_cast<double>(delta.x()), static_cast<double>(delta.y()));
        update();
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void TspWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = false;
        unsetCursor();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void TspWidget::leaveEvent(QEvent* event)
{
    // If the mouse leaves the widget while dragging, stop the drag cleanly.
    if (m_dragging)
    {
        m_dragging = false;
        unsetCursor();
    }
    QWidget::leaveEvent(event);
}


void TspWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // grid
    painter.setPen(QPen(Qt::lightGray, 1));
    for (double i = 0.0; i <= 1.0; i += (1.0 / TICK))
    {
        const int y = static_cast<int>(i * height());
        const int x = static_cast<int>(i * width());
        painter.drawLine(0, y, width(), y);
        painter.drawLine(x, 0, x, height());
    }

    if (!m_instance) return;

    const int32_t maxX = m_instance->maxX();
    const int32_t maxY = m_instance->maxY();
    const int32_t minX = m_instance->minX();
    const int32_t minY = m_instance->minY();

    // keep behavior close to Java: scale against maxX/maxY
    const double x_scale = (static_cast<double>(width())  * m_borderScale) / (static_cast<double>(maxX));
    const double y_scale = (static_cast<double>(height()) * m_borderScale) / (static_cast<double>(maxY));

    // rotation pivot close to Java
    QPointF pivot;
    if (m_rotation == 0 || m_rotation == 180)
        pivot = QPointF(x_scale * (static_cast<double>(maxX - minX)) / 2.0,
                        y_scale * (static_cast<double>(maxY - minY)) / 2.0);
    else
        pivot = QPointF(y_scale * (static_cast<double>(maxY - minY)) / 2.0,
                        x_scale * (static_cast<double>(maxX - minX)) / 2.0);

    QTransform tr;
    tr.translate(m_pan.x(), m_pan.y());
    tr.translate(pivot.x(), pivot.y());
    tr.rotate(static_cast<double>(m_rotation));
    tr.translate(-pivot.x(), -pivot.y());
    painter.setWorldTransform(tr, true);

    // last in red
    painter.setPen(QPen(Qt::red, 1));
    drawTour(painter, *m_instance, m_last, x_scale, y_scale, minX, minY, m_showLines);

    // current in blue
    painter.setPen(QPen(Qt::blue, 1));
    drawTour(painter, *m_instance, m_current, x_scale, y_scale, minX, minY, m_showLines);
}
