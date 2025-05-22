#include "WinGsmtcsmManager.h"

#include <QtDebug>
#include <QtCore/QFile>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>


using namespace winrt::Windows::Media::Control ;

class WinGsmtcsmManager::Worker : public QObject {
    Q_OBJECT

public:
    Worker() = default;

    static QString stringFromHString(const winrt::hstring &hs) {
        return QString::fromWCharArray(hs.c_str(), hs.size());
    }

    Q_SLOT bool init() {
        if (manager != nullptr) return true;
        manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        return manager != nullptr;
    }

    Q_SLOT void update() {
        if (!init()) return;
        auto session = manager.GetCurrentSession();
        if (session == nullptr) {
            qDebug() << "No session!";
            return;
        }
        GlobalSystemMediaTransportControlsSessionMediaProperties mediaProperty = session.TryGetMediaPropertiesAsync().get();

        auto title = stringFromHString(mediaProperty.Title());
        auto subtitle = stringFromHString(mediaProperty.Subtitle());
        winrt::Windows::Foundation::IReference<winrt::Windows::Media::MediaPlaybackType> m_playbackType = mediaProperty.PlaybackType();
        auto thumbnail = mediaProperty.Thumbnail();
        QByteArray thumbnailData;
        if (thumbnail) {
            auto stream = thumbnail.OpenReadAsync().get();
            if (stream && stream.CanRead()) {
                winrt::Windows::Storage::Streams::IBuffer data = winrt::Windows::Storage::Streams::Buffer(stream.Size());
                data = stream.ReadAsync(data, stream.Size(), winrt::Windows::Storage::Streams::InputStreamOptions::None).get();
                thumbnailData = QByteArray(reinterpret_cast<char *>(data.data()), data.Capacity());

                QFile f("C:\\thumbnail.png");
                f.open(QIODevice::WriteOnly);
                f.write(thumbnailData);
                f.close();
            }
        }
        qDebug()
                << "Current playing:" << title
                << "Artist:" << stringFromHString(mediaProperty.Artist())
                << "Album artist:" << stringFromHString(mediaProperty.AlbumArtist())
                << "Album title:" << stringFromHString(mediaProperty.AlbumTitle())
                << "Track number:" << mediaProperty.TrackNumber()
                << "Album track count:" << mediaProperty.AlbumTrackCount()
                // << "Genres:" << mediaProperty.Genres()
                << "Playback type:" << (m_playbackType == nullptr ? -1 : static_cast<int>(m_playbackType.Value()))
                << "Subtitle:" << subtitle
                << "Thumbnail:" << thumbnailData.length() << "bytes";
    }

    GlobalSystemMediaTransportControlsSessionManager manager{nullptr};
};


WinGsmtcsmManager::WinGsmtcsmManager() : worker(new Worker()) {
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    // connect(this, &Controller::operate, worker, &Worker::doWork);
    // connect(worker, &Worker::resultReady, this, &Controller::handleResults);
    workerThread.start();
    QMetaObject::invokeMethod(worker, "init", Qt::QueuedConnection);
    QMetaObject::invokeMethod(worker, "update", Qt::QueuedConnection);
}

WinGsmtcsmManager::~WinGsmtcsmManager() {
    workerThread.quit();
    workerThread.wait();
}

#include "WinGsmtcsmManager.moc"
