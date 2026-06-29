/**
 * @file    MusicPlayerWidget.h
 * @brief   音乐播放器悬浮组件，支持播放控制、音量调节与播放模式切换
 * @author  
 * @date    2026-06-24
 */

#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QStringList>

/**
 * @class   MusicPlayerWidget
 * @brief   可拖拽的音乐播放器悬浮窗，提供播放/暂停、上下曲、音量和模式控制
 */
class MusicPlayerWidget : public QWidget {
    Q_OBJECT

public:
    explicit MusicPlayerWidget(QWidget *parent = nullptr);
    void setMusicVolume(int volume);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void togglePlayPause();
    void playPrevious();
    void playNext();
    void updatePosition(qint64 position);
    void updateDuration(qint64 duration);
    void updatePlayerState(QMediaPlayer::PlaybackState state);
    void onSliderMoved(int position);
    void onSliderPressed();
    void onSliderReleased();
    void showVolumePopup();
    void hideVolumePopup();
    void updateVolume(int volume);
    void switchPlaybackMode();
    void updateTrackInfo(int index);
    void setCurrentTrack(int index);

private:
    void setupUI();
    void setupConnections();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void loadPlaylist();
    void updateModeDisplay();
    QString formatTime(qint64 ms) const;
    QString locateAssetsRoot() const;
    QString playbackModeLabel() const;

    enum class PlaybackMode {
        Loop,
        Single,
        Random
    };

    // 拖拽状态
    bool isDragging_ = false;
    QPoint dragStartPos_;

    QMediaPlayer *player_;
    QAudioOutput *audioOutput_;
    QStringList trackList_;
    int currentTrackIndex_;
    PlaybackMode playbackMode_ = PlaybackMode::Loop;

    QPushButton *prevBtn_;
    QPushButton *playPauseBtn_;
    QPushButton *nextBtn_;
    QPushButton *modeBtn_;
    QLabel *titleLabel_;
    QLabel *timeLabel_;
    QSlider *progressSlider_;
    QPushButton *volumeButton_;
    QWidget *volumePopup_;
    QSlider *volumeSlider_;
    bool isSeeking_ = false;
};
