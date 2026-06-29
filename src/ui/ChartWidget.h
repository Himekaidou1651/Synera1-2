/**
 * @file    ChartWidget.h
 * @brief   自定义折线图组件，用于经济统计可视化
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QWidget>
#include <QPainter>
#include <QToolTip>
#include <QMouseEvent>
#include <vector>
#include <utility>

/**
 * @class   ChartWidget
 * @brief   自定义折线图组件，支持数据点悬停提示和颜色自定义
 */
class ChartWidget : public QWidget {
    Q_OBJECT

public:
    struct DataPoint {
        int round;
        double value;
    };

    explicit ChartWidget(QWidget *parent = nullptr);

    void setData(const std::vector<DataPoint>& data);
    void setTitle(const QString& title);
    void setLineColor(const QColor& color);
    void setFillColor(const QColor& color);
    void setValuePrefix(const QString& prefix);
    void setValueSuffix(const QString& suffix);
    void clearData();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void drawGrid(QPainter& painter, const QRectF& chartArea);
    void drawAxes(QPainter& painter, const QRectF& chartArea);
    void drawLineChart(QPainter& painter, const QRectF& chartArea);
    void drawTooltip(QPainter& painter, const QRectF& chartArea);
    int findNearestPoint(const QPointF& mousePos, const QRectF& chartArea) const;

    std::vector<DataPoint> data_;
    QString title_;
    QColor lineColor_ = QColor(93, 173, 226);
    QColor fillColor_ = QColor(93, 173, 226, 40);
    QString valuePrefix_;
    QString valueSuffix_;

    int hoveredIndex_ = -1;
    QPointF hoveredPoint_;
};
