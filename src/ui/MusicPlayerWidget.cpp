/**
 * @file    MusicPlayerWidget.cpp
 * @brief   音乐播放器悬浮组件实现
 * @author  
 * @date    2026-06-24
 */

#include "MusicPlayerWidget.h"
#include "../core/Commondata.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QDir>
#include <QEvent>
#include <QStyle>
#include <QApplication>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QCursor>

MusicPlayerWidget::MusicPlayerWidget(QWidget *parent)
    : QWidget(parent),
      player_(new QMediaPlayer(this)),
      audioOutput_(new QAudioOutput(this)),
      trackList_(),
      currentTrackIndex_(0),
      playbackMode_(PlaybackMode::Loop) {
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(MUSICPLAYER_HEIGHT);
    setStyleSheet(
        "MusicPlayerWidget {"
        "    background-color: rgba(12, 14, 18, 220);"
        "    border: 1px solid rgba(255, 255, 255, 0.12);"
        "    border-radius: 14px;"
        "}"
        "QLabel { color: #ffffff; }"
        "QPushButton { color: #ffffff; border: none; background: transparent; font-size: 16px; }"
        "QPushButton:hover { color: #f1c40f; }"
        "QSlider::groove:horizontal { background: rgba(255,255,255,0.14); height: 8px; border-radius: 4px; }"
        "QSlider::handle:horizontal { background: #ffffff; width: 12px; border-radius: 6px; margin: -2px 0; }"
        "QSlider::groove:vertical { background: rgba(255,255,255,0.14); width: 8px; border-radius: 4px; }"
        "QSlider::handle:vertical { background: #ffffff; height: 12px; border-radius: 6px; margin: 0 -2px; }"
    );

    player_->setAudioOutput(audioOutput_);
    audioOutput_->setVolume(AUDIO_INITIAL_VOLUME);

    setupUI();
    setupConnections();
    loadPlaylist();
    updateModeDisplay();
    if (!trackList_.isEmpty()) {
        setCurrentTrack(0);
        player_->play();
    }

    player_->setAudioOutput(audioOutput_);
    
    connect(player_, &QMediaPlayer::mediaStatusChanged, 
            this, &MusicPlayerWidget::onMediaStatusChanged);
}

void MusicPlayerWidget::setupUI() {
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(MUSIC_MARGIN_LR, MUSIC_MARGIN_TB, MUSIC_MARGIN_LR, MUSIC_MARGIN_TB);
    rootLayout->setSpacing(LAYOUT_SPACING_WIDE2);

    // 信息区
    QWidget *infoContainer = new QWidget(this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoContainer);
    infoLayout->setSpacing(4);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    titleLabel_ = new QLabel("当前播放: 无音乐", infoContainer);
    titleLabel_->setStyleSheet("QLabel { font-size: 13px; font-weight: bold; }");
    timeLabel_ = new QLabel("00:00 / 00:00", infoContainer);
    timeLabel_->setStyleSheet("QLabel { font-size: 11px; color: rgba(255,255,255,0.72); }");
    infoLayout->addWidget(titleLabel_);
    infoLayout->addWidget(timeLabel_);
    rootLayout->addWidget(infoContainer, 2);

    // 控制区
    QWidget *controlContainer = new QWidget(this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlContainer);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->setSpacing(8);
    prevBtn_ = new QPushButton("⏮", controlContainer);
    playPauseBtn_ = new QPushButton("⏯", controlContainer);
    nextBtn_ = new QPushButton("⏭", controlContainer);
    prevBtn_->setFixedSize(BUTTON_SIZE_MEDIUM, BUTTON_SIZE_MEDIUM);
    playPauseBtn_->setFixedSize(BUTTON_SIZE_MEDIUM, BUTTON_SIZE_MEDIUM);
    nextBtn_->setFixedSize(BUTTON_SIZE_MEDIUM, BUTTON_SIZE_MEDIUM);
    controlLayout->addWidget(prevBtn_);
    controlLayout->addWidget(playPauseBtn_);
    controlLayout->addWidget(nextBtn_);
    rootLayout->addWidget(controlContainer, 1, Qt::AlignVCenter);

    // 进度区
    QWidget *progressContainer = new QWidget(this);
    QVBoxLayout *progressLayout = new QVBoxLayout(progressContainer);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(6);
    progressSlider_ = new QSlider(Qt::Horizontal, progressContainer);
    progressSlider_->setRange(0, SLIDER_PROGRESS_MAX);
    progressSlider_->setStyleSheet("QSlider { min-height: 12px; }");
    QLabel *progressLabel = new QLabel("进度", progressContainer);
    progressLabel->setStyleSheet("QLabel { font-size: 11px; color: rgba(255,255,255,0.72); }");
    progressLayout->addWidget(progressLabel);
    progressLayout->addWidget(progressSlider_);
    rootLayout->addWidget(progressContainer, 3);

    // 音量区
    QWidget *volumeContainer = new QWidget(this);
    QVBoxLayout *volumeLayout = new QVBoxLayout(volumeContainer);
    volumeLayout->setContentsMargins(0, 0, 0, 0);
    volumeLayout->setSpacing(4);
    volumeButton_ = new QPushButton("🔊", volumeContainer);
    volumeButton_->setFixedSize(32, 32);
    volumeButton_->setStyleSheet("QPushButton { font-size: 14px; } QPushButton:hover { color: #f39c12; }");
    volumeLayout->addWidget(volumeButton_, 0, Qt::AlignHCenter);
    QLabel *volumeLabel = new QLabel("音量", volumeContainer);
    volumeLabel->setStyleSheet("QLabel { font-size: 11px; color: rgba(255,255,255,0.72); }");
    volumeLayout->addWidget(volumeLabel, 0, Qt::AlignHCenter);
    rootLayout->addWidget(volumeContainer, 0, Qt::AlignVCenter);

    // 音量弹窗
    volumePopup_ = new QWidget(this, Qt::Popup | Qt::FramelessWindowHint);
    volumePopup_->setAttribute(Qt::WA_StyledBackground, true);
    volumePopup_->setStyleSheet(
        "QWidget { background-color: rgba(14, 17, 22, 240); border: 1px solid rgba(255,255,255,0.12); border-radius: 12px; }"
        "QSlider::groove:vertical { background: rgba(255,255,255,0.14); width: 8px; border-radius: 4px; }"
        "QSlider::handle:vertical { background: #ffffff; height: 12px; border-radius: 6px; margin: 0 -2px; }"
    );
    QVBoxLayout *popupLayout = new QVBoxLayout(volumePopup_);
    popupLayout->setContentsMargins(8, 8, 8, 8);
    volumeSlider_ = new QSlider(Qt::Vertical, volumePopup_);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(65);
    volumeSlider_->setFixedHeight(MUSICPLAYER_HEIGHT - 4);
    popupLayout->addWidget(volumeSlider_, 0, Qt::AlignCenter);
    volumePopup_->installEventFilter(this);

    // 模式区
    QWidget *modeContainer = new QWidget(this);
    QVBoxLayout *modeLayout = new QVBoxLayout(modeContainer);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->setSpacing(4);
    modeBtn_ = new QPushButton("列表循环", modeContainer);
    modeBtn_->setStyleSheet(
        "QPushButton { background-color: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.12); border-radius: 8px; padding: 8px; font-size: 11px; }"
        "QPushButton:hover { background-color: rgba(255,255,255,0.14); }"
    );
    modeBtn_->setFixedSize(100, 38);
    modeLayout->addWidget(modeBtn_, 0, Qt::AlignCenter);
    QLabel *modeLabel = new QLabel("播放模式", modeContainer);
    modeLabel->setStyleSheet("QLabel { font-size: 11px; color: rgba(255,255,255,0.72); }");
    modeLayout->addWidget(modeLabel, 0, Qt::AlignCenter);
    rootLayout->addWidget(modeContainer, 0, Qt::AlignVCenter);

        // 音量按钮事件过滤器保留用于悬停显示弹窗
    volumeButton_->installEventFilter(this);

    // 初始隐藏进度标签（由外部控制显示）
    setVisible(false);
}

void MusicPlayerWidget::setupConnections() {
    connect(prevBtn_, &QPushButton::clicked, this, &MusicPlayerWidget::playPrevious);
    connect(playPauseBtn_, &QPushButton::clicked, this, &MusicPlayerWidget::togglePlayPause);
    connect(nextBtn_, &QPushButton::clicked, this, &MusicPlayerWidget::playNext);
    connect(modeBtn_, &QPushButton::clicked, this, &MusicPlayerWidget::switchPlaybackMode);
    connect(player_, &QMediaPlayer::positionChanged, this, &MusicPlayerWidget::updatePosition);
    connect(player_, &QMediaPlayer::durationChanged, this, &MusicPlayerWidget::updateDuration);
    connect(player_, &QMediaPlayer::playbackStateChanged, this, &MusicPlayerWidget::updatePlayerState);
    connect(progressSlider_, &QSlider::sliderMoved, this, &MusicPlayerWidget::onSliderMoved);
    connect(progressSlider_, &QSlider::sliderPressed, this, &MusicPlayerWidget::onSliderPressed);
    connect(progressSlider_, &QSlider::sliderReleased, this, &MusicPlayerWidget::onSliderReleased);
    connect(volumeButton_, &QPushButton::clicked, this, &MusicPlayerWidget::showVolumePopup);
    connect(volumeSlider_, &QSlider::valueChanged, this, &MusicPlayerWidget::updateVolume);
}

void MusicPlayerWidget::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::EndOfMedia) {  // 当前歌曲播放完毕
        playNext();  // 自动播放下一首
    }
}

void MusicPlayerWidget::loadPlaylist() {
    QString root = locateAssetsRoot();
    if (root.isEmpty()) {
        titleLabel_->setText("未检测到 assets 目录");
        return;
    }

    QDir musicDir(root + "/music");
    if (!musicDir.exists()) {
        titleLabel_->setText("未检测到 music 目录");
        return;
    }

    QStringList filters = {"*.mp3", "*.wav", "*.ogg", "*.flac"};
    QFileInfoList entries = musicDir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo &info : entries) {
        trackList_.append(info.absoluteFilePath());
    }

    if (trackList_.isEmpty()) {
        titleLabel_->setText("当前未检测到音轨");
        return;
    }

    currentTrackIndex_ = 0;
    setCurrentTrack(currentTrackIndex_);
    player_->play();
}

QString MusicPlayerWidget::locateAssetsRoot() const {
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        if (dir.exists("assets")) {
            return dir.filePath("assets");
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QString();
}

void MusicPlayerWidget::togglePlayPause() {
    if (player_->playbackState() == QMediaPlayer::PlayingState) {
        player_->pause();
    } else {
        player_->play();
    }
}

void MusicPlayerWidget::playPrevious() {
    if (trackList_.isEmpty()) {
        return;
    }
    if (playbackMode_ == PlaybackMode::Random) {
        currentTrackIndex_ = QRandomGenerator::global()->bounded(trackList_.size());
    } else {
        currentTrackIndex_ = (currentTrackIndex_ - 1 + trackList_.size()) % trackList_.size();
    }
    setCurrentTrack(currentTrackIndex_);
    player_->play();
}

void MusicPlayerWidget::playNext() {
    if (trackList_.isEmpty()) {
        return;
    }
    if (playbackMode_ == PlaybackMode::Random) {
        currentTrackIndex_ = QRandomGenerator::global()->bounded(trackList_.size());
    } else {
        currentTrackIndex_ = (currentTrackIndex_ + 1) % trackList_.size();
    }
    setCurrentTrack(currentTrackIndex_);
    player_->play();
}

void MusicPlayerWidget::updatePosition(qint64 position) {
    if (!isSeeking_) {
        qint64 duration = player_->duration();
        if (duration > 0) {
            progressSlider_->setValue(static_cast<int>(position * 1000 / duration));
        }
        timeLabel_->setText(QString("%1 / %2").arg(formatTime(position), formatTime(duration)));
    }
}

void MusicPlayerWidget::updateDuration(qint64 duration) {
    if (duration > 0 && !isSeeking_) {
        timeLabel_->setText(QString("%1 / %2").arg(formatTime(player_->position()), formatTime(duration)));
    }
}

void MusicPlayerWidget::updatePlayerState(QMediaPlayer::PlaybackState state) {
    playPauseBtn_->setText(state == QMediaPlayer::PlayingState ? "⏸" : "▶");
}

void MusicPlayerWidget::onSliderMoved(int position) {
    if (player_->duration() > 0) {
        qint64 ms = static_cast<qint64>(position) * player_->duration() / 1000;
        timeLabel_->setText(QString("%1 / %2").arg(formatTime(ms), formatTime(player_->duration())));
    }
}

void MusicPlayerWidget::onSliderPressed() {
    isSeeking_ = true;
}

void MusicPlayerWidget::onSliderReleased() {
    if (player_->duration() > 0) {
        qint64 ms = static_cast<qint64>(progressSlider_->value()) * player_->duration() / 1000;
        player_->setPosition(ms);
    }
    isSeeking_ = false;
}

void MusicPlayerWidget::showVolumePopup() {
    if (!volumePopup_) return;
    if (volumePopup_->isVisible()) {
        volumePopup_->hide();
        return;
    }

    volumePopup_->adjustSize();
    QPoint globalPos = volumeButton_->mapToGlobal(volumeButton_->rect().bottomLeft());
    volumePopup_->move(globalPos.x() - volumePopup_->width() + volumeButton_->width(), globalPos.y() + 6);
    volumePopup_->show();
    volumePopup_->raise();
    volumePopup_->setFocus();
}

void MusicPlayerWidget::hideVolumePopup() {
    if (volumePopup_) {
        volumePopup_->hide();
    }
}

void MusicPlayerWidget::updateVolume(int volume) {
    audioOutput_->setVolume(static_cast<float>(volume) / 100.0f);
}

void MusicPlayerWidget::setMusicVolume(int volume) {
    audioOutput_->setVolume(static_cast<float>(volume) / 100.0f);
    if (volumeSlider_) {
        volumeSlider_->blockSignals(true);
        volumeSlider_->setValue(volume);
        volumeSlider_->blockSignals(false);
    }
}

void MusicPlayerWidget::switchPlaybackMode() {
    switch (playbackMode_) {
        case PlaybackMode::Loop:
            playbackMode_ = PlaybackMode::Single;
            break;
        case PlaybackMode::Single:
            playbackMode_ = PlaybackMode::Random;
            break;
        case PlaybackMode::Random:
            playbackMode_ = PlaybackMode::Loop;
            break;
    }
    updateModeDisplay();
}

void MusicPlayerWidget::updateTrackInfo(int index) {
    if (index < 0 || index >= trackList_.size()) {
        titleLabel_->setText("当前播放: 无音乐");
        return;
    }
    QString fileName = QFileInfo(trackList_.at(index)).baseName();
    titleLabel_->setText(QString("当前播放: %1").arg(fileName));
}

void MusicPlayerWidget::setCurrentTrack(int index) {
    if (index < 0 || index >= trackList_.size()) {
        return;
    }
    currentTrackIndex_ = index;
    player_->setSource(QUrl::fromLocalFile(trackList_.at(index)));
    updateTrackInfo(index);
}

QString MusicPlayerWidget::formatTime(qint64 ms) const {
    if (ms < 0) {
        return "00:00";
    }
    qint64 seconds = ms / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

QString MusicPlayerWidget::playbackModeLabel() const {
    switch (playbackMode_) {
        case PlaybackMode::Random:
            return "随机播放";
        case PlaybackMode::Single:
            return "单曲循环";
        default:
            return "列表循环";
    }
}

void MusicPlayerWidget::updateModeDisplay() {
    if (!modeBtn_) return;
    modeBtn_->setText(playbackModeLabel());
}

bool MusicPlayerWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == volumeButton_) {
        if (event->type() == QEvent::Enter) {
            showVolumePopup();
            return true;
        }
    }

    if (watched == volumePopup_) {
        if (event->type() == QEvent::Leave) {
            hideVolumePopup();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

// ============ 可拖动浮动窗口实现 ============

void MusicPlayerWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging_ = true;
        dragStartPos_ = event->globalPosition().toPoint() - parentWidget()->mapToGlobal(pos());
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void MusicPlayerWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging_ && parentWidget()) {
        QPoint newPos = event->globalPosition().toPoint() - dragStartPos_;
        // 限制在父窗口范围内，留边距
        QWidget *parent = parentWidget();
        int margin = 10;
        newPos.setX(qBound(margin, newPos.x(), parent->width() - width() - margin));
        newPos.setY(qBound(margin, newPos.y(), parent->height() - height() - margin));
        move(newPos);
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void MusicPlayerWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isDragging_) {
        isDragging_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
    QWidget::mouseReleaseEvent(event);
}
