#include "videosink.h"
#include "api/video/i420_buffer.h"
#include "third_party/libyuv/include/libyuv/convert_from.h"
#include "settings.h"
#include <chrono>

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QDir>
#include <QVBoxLayout>


VideoSink::VideoSink() {
    frameCount = 0;
    image_.reset(new uint8_t[640 * 360 * 4]);

    if(Settings::useUI) {
        QWidget *win = new QWidget();
        QString title = QString(Settings::label.c_str()) + QString(" - ") + (Settings::mode == Settings::Mode::Publisher ? QString("Publisher") : QString("Player"));
        win->setWindowTitle(title);

        label = new QLabel(win);
        label->setGeometry(QRect(5,5,640,360));

        win->setFixedSize(QSize(650,370));

        win->show();
    }
}

void VideoSink::setTrack(webrtc::VideoTrackInterface* track_to_render) {
    track_to_render->AddOrUpdateSink(this, rtc::VideoSinkWants());
}

void VideoSink::saveFrame(QImage &img) {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    QString fileName = QDir::currentPath()+QString("/")
            + QString(Settings::label.c_str())
            + QString("_frame") + QString::number(frameCount)
            + QString("@") + QString::number(now) + QString(".png");
    img.save(fileName, "png", 100);
}
void VideoSink::OnFrame(const webrtc::VideoFrame& video_frame) {
    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(video_frame.video_frame_buffer()->ToI420());

    if (video_frame.rotation() != webrtc::kVideoRotation_0) {
      buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
    }

    int h = video_frame.height();
    int w = video_frame.width();
    int s =video_frame.size();

    image_.reset(new uint8_t[w * h * 4]);

    libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(),
                       buffer->DataU(), buffer->StrideU(),
                       buffer->DataV(), buffer->StrideV(),
                       image_.get(), w * 4,
                       buffer->width(), buffer->height());


    if((Settings::period > 0) && ((frameCount % Settings::period) == 0)) {
        QImage img(image_.get(), w, h, QImage::Format_ARGB32 );
        saveFrame(img);
    }

    if(Settings::useUI) {
        QImage img(image_.get(), w, h, QImage::Format_ARGB32 );
        label->setPixmap(QPixmap::fromImage(img).scaledToWidth(640, Qt::SmoothTransformation));
    }

    frameCount++;
}
