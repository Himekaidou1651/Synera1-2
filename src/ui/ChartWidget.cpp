/**
 * @file    ChartWidget.cpp
 * @brief   自定义折线图组件实现
 * @author  
 * @date    2026-06-24
 */

#include "ChartWidget.h"
#include "FontConfig.h"
#include "../core/Commondata.h"
#include <QPainterPath>
#include <QLinearGradient>
#include <cmath>
#include <algorithm>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent) {
    setMouseTracking(true);
    setMinimumHeight(200);
}

void ChartWidget::setData(const std::vector<DataPoint>& data) {
    data_ = data;
    hoveredIndex_ = -1;
    update();
}

void ChartWidget::setTitle(const QString& title) {
    title_ = title;
    update();
}

void ChartWidget::setLineColor(const QColor& color) {
    lineColor_ = color;
    fillColor_ = QColor(color.red(), color.green(), color.blue(), 40);
    update();
}

void ChartWidget::setFillColor(const QColor& color) {
    fillColor_ = color;
    update();
}

void ChartWidget::setValuePrefix(const QString& prefix) {
    valuePrefix_ = prefix;
    update();
}

void ChartWidget::setValueSuffix(const QString& suffix) {
    valueSuffix_ = suffix;
    update();
}

void ChartWidget::clearData() {
    data_.clear();
    hoveredIndex_ = -1;
    update();
}

void ChartWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 背景
    painter.fillRect(rect(), QColor(0, 0, 0, 0));

    if (data_.empty()) {
        painter.setPen(QColor(255, 255, 255, 100));
        painter.setFont(appFont(FONT_SMALL));
        painter.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    // 边距
    const int marginLeft = CHART_MARGIN_LEFT;
    const int marginRight = CHART_MARGIN_RIGHT;
    const int marginTop = CHART_MARGIN_TOP;
    const int marginBottom = CHART_MARGIN_BOTTOM;

    QRectF chartArea(
        marginLeft, marginTop,
        width() - marginLeft - marginRight,
        height() - marginTop - marginBottom
    );

    // 绘制标题
    if (!title_.isEmpty()) {
        painter.setPen(QColor(255, 255, 255, 200));
        painter.setFont(appFont(13, true));
        painter.drawText(QRectF(0, 4, width(), 22), Qt::AlignCenter, title_);
    }

    drawGrid(painter, chartArea);
    drawAxes(painter, chartArea);
    drawLineChart(painter, chartArea);
    drawTooltip(painter, chartArea);
}

void ChartWidget::drawGrid(QPainter& painter, const QRectF& chartArea) {
    if (data_.empty()) return;

    painter.setPen(QPen(QColor(255, 255, 255, 20), 1, Qt::DashLine));

    // 横向网格线（5条）
    double minVal = 0;
    double maxVal = data_[0].value;
    for (const auto& dp : data_) {
        maxVal = std::max(maxVal, dp.value);
    }
    // 给顶部留10%空间
    maxVal = maxVal * 1.1;
    if (maxVal <= 0) maxVal = 10;

    for (int i = 0; i <= 4; ++i) {
        double y = chartArea.bottom() - (chartArea.height() * i / 4.0);
        painter.drawLine(QPointF(chartArea.left(), y), QPointF(chartArea.right(), y));

        // Y轴标签
        double val = minVal + (maxVal - minVal) * i / 4.0;
        painter.setPen(QColor(255, 255, 255, 120));
        painter.setFont(appFont(10));
        painter.drawText(QRectF(0, y - 8, chartArea.left() - 6, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(val, 'f', 0));
        painter.setPen(QPen(QColor(255, 255, 255, 20), 1, Qt::DashLine));
    }
}

void ChartWidget::drawAxes(QPainter& painter, const QRectF& chartArea) {
    painter.setPen(QPen(QColor(255, 255, 255, 60), 1));
    // X轴
    painter.drawLine(QPointF(chartArea.left(), chartArea.bottom()),
                     QPointF(chartArea.right(), chartArea.bottom()));
    // Y轴
    painter.drawLine(QPointF(chartArea.left(), chartArea.top()),
                     QPointF(chartArea.left(), chartArea.bottom()));

    // X轴标签
    if (data_.size() <= 1) return;
    painter.setPen(QColor(255, 255, 255, 120));
    painter.setFont(appFont(10));

    // 最多显示10个X轴标签
    int maxLabels = std::min(static_cast<int>(data_.size()), 10);
    int step = std::max(1, static_cast<int>(data_.size() - 1) / (maxLabels - 1));
    for (size_t i = 0; i < data_.size(); i += step) {
        double x = chartArea.left() + chartArea.width() * i / (data_.size() - 1);
        painter.drawText(QRectF(x - 15, chartArea.bottom() + 2, 30, 18),
                         Qt::AlignCenter,
                         QString("R%1").arg(data_[i].round));
    }
    // 最后一个标签
    size_t last = data_.size() - 1;
    double xLast = chartArea.left() + chartArea.width();
    painter.drawText(QRectF(xLast - 15, chartArea.bottom() + 2, 30, 18),
                     Qt::AlignCenter,
                     QString("R%1").arg(data_[last].round));
}

void ChartWidget::drawLineChart(QPainter& painter, const QRectF& chartArea) {
    if (data_.size() < 2) return;

    // 计算Y轴范围
    double minVal = 0;
    double maxVal = data_[0].value;
    for (const auto& dp : data_) {
        maxVal = std::max(maxVal, dp.value);
        minVal = std::min(minVal, dp.value);
    }
    maxVal = maxVal * 1.1;
    if (maxVal <= 0) maxVal = 10;

    // 数据点映射到像素坐标
    std::vector<QPointF> points;
    for (size_t i = 0; i < data_.size(); ++i) {
        double x = chartArea.left() + chartArea.width() * i / (data_.size() - 1);
        double y = chartArea.bottom() - chartArea.height() * (data_[i].value - minVal) / (maxVal - minVal);
        points.emplace_back(x, y);
    }

    // 填充区域（渐变）
    QLinearGradient gradient(0, chartArea.top(), 0, chartArea.bottom());
    gradient.setColorAt(0.0, fillColor_);
    gradient.setColorAt(1.0, QColor(fillColor_.red(), fillColor_.green(), fillColor_.blue(), 10));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);

    QPainterPath fillPath;
    fillPath.moveTo(points[0].x(), chartArea.bottom());
    for (const auto& pt : points) {
        fillPath.lineTo(pt);
    }
    fillPath.lineTo(points.back().x(), chartArea.bottom());
    fillPath.closeSubpath();
    painter.drawPath(fillPath);

    // 折线
    QPen linePen(lineColor_, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(linePen);
    painter.setBrush(Qt::NoBrush);
    for (size_t i = 1; i < points.size(); ++i) {
        painter.drawLine(points[i - 1], points[i]);
    }

    // 数据点圆点
    for (size_t i = 0; i < points.size(); ++i) {
        QColor dotColor = (static_cast<int>(i) == hoveredIndex_) ? QColor(255, 255, 255) : lineColor_;
        painter.setBrush(dotColor);
        painter.setPen(QPen(QColor(255, 255, 255, 180), 1));
        double radius = (static_cast<int>(i) == hoveredIndex_) ? 5.0 : 3.5;
        painter.drawEllipse(points[i], radius, radius);
    }
}

void ChartWidget::drawTooltip(QPainter& painter, const QRectF& chartArea) {
    if (hoveredIndex_ < 0 || hoveredIndex_ >= static_cast<int>(data_.size())) return;

    // 重新计算Y轴范围以获取点的位置
    double minVal = 0;
    double maxVal = data_[0].value;
    for (const auto& dp : data_) {
        maxVal = std::max(maxVal, dp.value);
    }
    maxVal = maxVal * 1.1;
    if (maxVal <= 0) maxVal = 10;

    double x = chartArea.left() + chartArea.width() * hoveredIndex_ / (data_.size() - 1);
    double y = chartArea.bottom() - chartArea.height() * (data_[hoveredIndex_].value - minVal) / (maxVal - minVal);

    // 绘制十字参考线
    painter.setPen(QPen(QColor(255, 255, 255, 60), 1, Qt::DashLine));
    painter.drawLine(QPointF(x, chartArea.top()), QPointF(x, chartArea.bottom()));
    painter.drawLine(QPointF(chartArea.left(), y), QPointF(chartArea.right(), y));

    // 提示框
    QString tipText = QString("回合 %1: %2%3%4")
        .arg(data_[hoveredIndex_].round)
        .arg(valuePrefix_)
        .arg(data_[hoveredIndex_].value, 0, 'f', 0)
        .arg(valueSuffix_);

    QFont tipFont = appFont(11, true);
    QFontMetrics fm(tipFont);
    int tipWidth = fm.horizontalAdvance(tipText) + 16;
    int tipHeight = 26;

    double tipX = x - tipWidth / 2.0;
    double tipY = y - tipHeight - 12;
    if (tipX < 2) tipX = 2;
    if (tipX + tipWidth > width() - 2) tipX = width() - tipWidth - 2;
    if (tipY < 2) tipY = y + 8;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(12, 14, 18, 230));
    painter.drawRoundedRect(QRectF(tipX, tipY, tipWidth, tipHeight), 6, 6);

    painter.setPen(QPen(lineColor_, 2));
    painter.drawRoundedRect(QRectF(tipX, tipY, tipWidth, tipHeight), 6, 6);

    painter.setPen(QColor(255, 255, 255));
    painter.setFont(tipFont);
    painter.drawText(QRectF(tipX, tipY, tipWidth, tipHeight), Qt::AlignCenter, tipText);
}

int ChartWidget::findNearestPoint(const QPointF& mousePos, const QRectF& chartArea) const {
    if (data_.empty()) return -1;

    double minVal = 0;
    double maxVal = data_[0].value;
    for (const auto& dp : data_) {
        maxVal = std::max(maxVal, dp.value);
    }
    maxVal = maxVal * 1.1;
    if (maxVal <= 0) maxVal = 10;

    int nearest = -1;
    double minDist = 30.0; // 最大感应距离

    for (size_t i = 0; i < data_.size(); ++i) {
        double px = chartArea.left() + chartArea.width() * i / (data_.size() - 1);
        double py = chartArea.bottom() - chartArea.height() * (data_[i].value - minVal) / (maxVal - minVal);
        double dist = std::sqrt(std::pow(mousePos.x() - px, 2) + std::pow(mousePos.y() - py, 2));
        if (dist < minDist) {
            minDist = dist;
            nearest = static_cast<int>(i);
        }
    }
    return nearest;
}

void ChartWidget::mouseMoveEvent(QMouseEvent *event) {
    if (data_.empty()) return;

    const int marginLeft = 50;
    const int marginRight = 20;
    const int marginTop = 30;
    const int marginBottom = 40;

    QRectF chartArea(
        marginLeft, marginTop,
        width() - marginLeft - marginRight,
        height() - marginTop - marginBottom
    );

    int idx = findNearestPoint(event->position(), chartArea);
    if (idx != hoveredIndex_) {
        hoveredIndex_ = idx;
        if (idx >= 0) {
            double minVal = 0;
            double maxVal = data_[0].value;
            for (const auto& dp : data_) maxVal = std::max(maxVal, dp.value);
            maxVal = maxVal * 1.1;
            if (maxVal <= 0) maxVal = 10;
            double px = chartArea.left() + chartArea.width() * idx / (data_.size() - 1);
            double py = chartArea.bottom() - chartArea.height() * (data_[idx].value - minVal) / (maxVal - minVal);
            hoveredPoint_ = QPointF(px, py);
        }
        update();
    }
}

void ChartWidget::leaveEvent(QEvent*) {
    if (hoveredIndex_ >= 0) {
        hoveredIndex_ = -1;
        update();
    }
}
