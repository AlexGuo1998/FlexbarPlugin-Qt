#include "WinGsmtcsmManager.h"

#include <QtDebug>
#include <QTimer>
#include <windows.h>
#include <QtCore/QFile>
#include <QtCore/QMutexLocker>
#include <QtGui/QImage>
#include <QtGui/QImageReader>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

#include <robuffer.h>


using namespace winrt::Windows::Media::Control ;

static QString qStringFromHString(const winrt::hstring &hs) {
    return QString::fromWCharArray(hs.c_str(), hs.size());
}


static QDebug &operator<<(QDebug &debug, const winrt::hstring &hs) {
    debug << qStringFromHString(hs);
    return debug;
}

static QDebug &operator<<(QDebug &debug, winrt::hstring &&hs) {
    debug << qStringFromHString(hs);
    return debug;
}

class RandomAccessStreamAdapter : public QIODevice {
    class VoidPtrBuffer : public winrt::implements<VoidPtrBuffer,
                winrt::Windows::Storage::Streams::IBuffer,
                Windows::Storage::Streams::IBufferByteAccess
            > {
    public:
        VoidPtrBuffer(char *data, uint32_t capacity) : m_buffer(data), m_length(0), m_capacity(capacity) {}

        uint32_t Capacity() const {
            return m_capacity;
        }

        uint32_t Length() const {
            return m_length;
        }

        void Length(uint32_t value) {
            if (value > m_capacity) {
                throw winrt::hresult_invalid_argument(L"Length cannot exceed capacity");
            }
            m_length = value;
        }

        // IBufferByteAccess
        STDMETHOD(Buffer)(byte **value) {
            *value = static_cast<uint8_t *>(m_buffer);
            return S_OK;
        }

    private:
        void *m_buffer;
        uint32_t m_length;
        uint32_t m_capacity;
    };

public:
    explicit RandomAccessStreamAdapter(winrt::Windows::Storage::Streams::IRandomAccessStream stream): stream(stream) {}

    qint64 readData(char *data, qint64 maxlen) override {
        using namespace winrt::Windows::Storage::Streams;
        const IBuffer buffer = winrt::make<VoidPtrBuffer>(data, maxlen);
        const IBuffer newBuffer = stream.ReadAsync(buffer, maxlen, InputStreamOptions::None).get();
        uint32_t length = newBuffer.Length();
        if (newBuffer != buffer) {
#if _DEBUG
            qWarning() << "Returned IBuffer differs from VoidPtrBuffer, using memcpy to copy the data";
#endif
            memcpy(data, newBuffer.data(), length);
        }
        return length;
    }

    qint64 writeData(const char *data, qint64 len) override { return 0; }

    winrt::Windows::Storage::Streams::IRandomAccessStream stream;
};


class W_GlobalSystemMediaTransportControlsSession {
public:
    GlobalSystemMediaTransportControlsSession p;
    W_GlobalSystemMediaTransportControlsSession::W_GlobalSystemMediaTransportControlsSession() : p{nullptr} {}
    W_GlobalSystemMediaTransportControlsSession::W_GlobalSystemMediaTransportControlsSession(const GlobalSystemMediaTransportControlsSession &p) : p{p} {}
};

Q_DECLARE_METATYPE(W_GlobalSystemMediaTransportControlsSession)


class WinGsmtcsmManager::Worker : public QObject {
    Q_OBJECT

public:
    Q_SIGNAL void infoUpdated(QString info, QImage thumbnail);

    Worker() = default;

    ~Worker() override {
#ifdef _DEBUG
        qDebug() << "Worker::~Worker";
#endif
    }

    Q_SLOT bool init() {
        if (manager != nullptr) return true;
        try {
            manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        } catch (winrt::hresult_error const &e) {
            qCritical() << "GlobalSystemMediaTransportControlsSessionManager::RequestAsync failed:" << e.code() << e.message();
        }
        if (manager == nullptr) {
            qCritical() << "GlobalSystemMediaTransportControlsSessionManager::RequestAsync returned NULL";
        }
        return manager != nullptr;
    }

    Q_SLOT void listen() {
        if (!init()) return;
        manager.CurrentSessionChanged(
            [weakThis = weakThis](const auto &, const auto &) {
                if (QSharedPointer<Worker> strongThis = weakThis.toStrongRef()) {
                    qDebug() << "Current session changed";
                    QMetaObject::invokeMethod(strongThis.get(), "updateCurrentSession");
                }
            }
        );
        updateCurrentSession();
    }

    static QImage readThumbnail(const GlobalSystemMediaTransportControlsSessionMediaProperties &mediaProperty) {
        QImage image;
        if (const auto thumbnail = mediaProperty.Thumbnail()) {
            if (const auto stream = thumbnail.OpenReadAsync().get();
                stream && stream.CanRead()
            ) {
                RandomAccessStreamAdapter adapter(stream);
                auto format = qStringFromHString(stream.ContentType());
                if (format.startsWith(QLatin1String("image/"))) {
                    format = format.remove(0, 6);
                }

                image.load(&adapter, format.toUtf8().constData());
                // bool ok = image.save("thumbnail.png");
            }
        }
        return image;
    }

    Q_SLOT void updateCurrentSession() {
        if (!init()) return;
        currentSession = manager.GetCurrentSession();

        if (currentSession == nullptr) {
            qDebug() << "No session";
            propertyChangeHandle.revoke();
            return;
        }

        propertyChangeHandle = currentSession.MediaPropertiesChanged(
            winrt::auto_revoke,
            [weakThis = weakThis](const GlobalSystemMediaTransportControlsSession &session, const auto &) {
                if (QSharedPointer<Worker> strongThis = weakThis.toStrongRef()) {
                    qDebug() << "Media property changed";
                    W_GlobalSystemMediaTransportControlsSession s1(session);
                    QMetaObject::invokeMethod(
                        strongThis.get(), "updateMediaPropertyCheckSession",
                        Q_ARG(W_GlobalSystemMediaTransportControlsSession, s1)
                    );
                }
            }
        );
        updateMediaProperty();
    }

    Q_SLOT void updateMediaPropertyCheckSession(W_GlobalSystemMediaTransportControlsSession session) {
        if (session.p == currentSession) updateMediaProperty();
    }

    void updateMediaProperty() {
        GlobalSystemMediaTransportControlsSessionMediaProperties mediaProperty(nullptr);
        try {
            mediaProperty = currentSession.TryGetMediaPropertiesAsync().get();
        } catch (winrt::hresult_error const &e) {
            qDebug() << "TryGetMediaPropertiesAsync error:" << e.code();
            return;
        }

        auto title = qStringFromHString(mediaProperty.Title());
        auto subtitle = qStringFromHString(mediaProperty.Subtitle());
        winrt::Windows::Foundation::IReference<winrt::Windows::Media::MediaPlaybackType> m_playbackType = mediaProperty.PlaybackType();

        QImage thumbnail = readThumbnail(mediaProperty);
        qDebug()
                << "Current playing:" << title
                << "Artist:" << mediaProperty.Artist()
                << "Album artist:" << mediaProperty.AlbumArtist()
                << "Album title:" << mediaProperty.AlbumTitle()
                << "Track number:" << mediaProperty.TrackNumber()
                << "Album track count:" << mediaProperty.AlbumTrackCount()
                // << "Genres:" << mediaProperty.Genres()
                << "Playback type:" << (m_playbackType == nullptr ? -1 : static_cast<int>(m_playbackType.Value()))
                << "Subtitle:" << subtitle
                << "Thumbnail:" << thumbnail;

        Q_EMIT infoUpdated(title, thumbnail);
    }

    QWeakPointer<Worker> weakThis{nullptr};
    GlobalSystemMediaTransportControlsSessionManager manager{nullptr};

    GlobalSystemMediaTransportControlsSession currentSession{nullptr};

    GlobalSystemMediaTransportControlsSession::MediaPropertiesChanged_revoker propertyChangeHandle;
};

WinGsmtcsmManager::WinGsmtcsmManager()
    : workerThread(new QThread(this)),
      worker(new Worker(), [](Worker *p) { p->deleteLater(); }) {
    qRegisterMetaType<W_GlobalSystemMediaTransportControlsSession>();

    worker->weakThis = worker.toWeakRef();
    worker->moveToThread(workerThread);
    workerThread->start();

    // for (auto fmt: QImageReader::supportedImageFormats())
    //     qDebug() << fmt;
    qDebug() << QImageReader::supportedImageFormats();

    connect(worker.get(), &Worker::infoUpdated, this, &WinGsmtcsmManager::infoUpdated);

    QMetaObject::invokeMethod(worker.get(), "init", Qt::QueuedConnection);
    QMetaObject::invokeMethod(worker.get(), "listen", Qt::QueuedConnection);
    // QMetaObject::invokeMethod(worker.get(), "updateCurrentSession", Qt::QueuedConnection);
}

WinGsmtcsmManager::~WinGsmtcsmManager() {
    worker.clear();
    workerThread->quit();
    workerThread->wait();
}

#include "WinGsmtcsmManager.moc"
