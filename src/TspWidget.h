#pragma once

#include <QWidget>
#include <QVector>
#include <QPoint>
#include <QPointF>

#include "TspInstance.h"

class QMouseEvent;
class QEvent;

class TspWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TspWidget(QWidget* parent = nullptr);

    void setInstance(const TspInstance* instance);
    void setTour(const QVector<int>& order);   // current (blue)
    void setLastTour(const QVector<int>& order); // last (red)
    void clearLastTour();

    void setBorderScale(double s); // like Java's borderSize
    void setRotationDeg(int deg);  // 0,90,180,270
    void setShowLines(bool show);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    const TspInstance* m_instance = nullptr;

    QVector<int> m_current;
    QVector<int> m_last;

    double m_borderScale = 1.0;
    int m_rotation = 0;
    bool m_showLines = false;

    // Panning (drag with left mouse button)
    QPointF m_pan{0.0, 0.0};
    bool m_dragging = false;
    QPoint m_lastMousePos;

    static constexpr double TICK = 25.0;
};
