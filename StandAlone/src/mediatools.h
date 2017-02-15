/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef MEDIATOOLS_H
#define MEDIATOOLS_H

#include <QSize>
#include <QMutex>
#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <ak.h>
#include <akutils.h>

#include "pluginconfigs.h"
#include "mediasource.h"
#include "audiolayer.h"
#include "videoeffects.h"
#include "recording.h"

class MediaTools: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int windowWidth
               READ windowWidth
               WRITE setWindowWidth
               RESET resetWindowWidth
               NOTIFY windowWidthChanged)
    Q_PROPERTY(int windowHeight
               READ windowHeight
               WRITE setWindowHeight
               RESET resetWindowHeight
               NOTIFY windowHeightChanged)
    Q_PROPERTY(bool enableVirtualCamera
               READ enableVirtualCamera
               WRITE setEnableVirtualCamera
               RESET resetEnableVirtualCamera
               NOTIFY enableVirtualCameraChanged)
    Q_PROPERTY(AkElement::ElementState virtualCameraState
               READ virtualCameraState
               WRITE setVirtualCameraState
               RESET resetVirtualCameraState
               NOTIFY virtualCameraStateChanged)

    public:
        explicit MediaTools(QObject *parent=NULL);
        ~MediaTools();

        Q_INVOKABLE int windowWidth() const;
        Q_INVOKABLE int windowHeight() const;
        Q_INVOKABLE bool enableVirtualCamera() const;
        Q_INVOKABLE AkElement::ElementState virtualCameraState() const;
        Q_INVOKABLE QString applicationName() const;
        Q_INVOKABLE QString applicationVersion() const;
        Q_INVOKABLE QString qtVersion() const;
        Q_INVOKABLE QString copyrightNotice() const;
        Q_INVOKABLE QString projectUrl() const;
        Q_INVOKABLE QString projectLicenseUrl() const;
        Q_INVOKABLE QString fileNameFromUri(const QString &uri) const;
        Q_INVOKABLE bool matches(const QString &pattern, const QStringList &strings) const;
        Q_INVOKABLE QString currentTime() const;
        Q_INVOKABLE QStringList standardLocations(const QString &type) const;
        Q_INVOKABLE QString saveFileDialog(const QString &caption="",
                                           const QString &fileName="",
                                           const QString &directory="",
                                           const QString &suffix="",
                                           const QString &filters="") const;
        Q_INVOKABLE QString readFile(const QString &fileName) const;
        Q_INVOKABLE QString urlToLocalFile(const QUrl &url) const;
        Q_INVOKABLE bool embedVirtualCameraControls(const QString &where,
                                                    const QString &name="");
        Q_INVOKABLE void removeInterface(const QString &where,
                                         QQmlApplicationEngine *engine=NULL);
        static QString convertToAbsolute(const QString &path);

    private:
        QQmlApplicationEngine *m_engine;
        PluginConfigsPtr m_pluginConfigs;
        MediaSourcePtr m_mediaSource;
        AudioLayerPtr m_audioLayer;
        VideoEffectsPtr m_videoEffects;
        RecordingPtr m_recording;
        int m_windowWidth;
        int m_windowHeight;
        bool m_enableVirtualCamera;

        AkElementPtr m_virtualCamera;

        bool embedInterface(QQmlApplicationEngine *engine,
                            QObject *ctrlInterface,
                            const QString &where) const;

    signals:
        void windowWidthChanged(int windowWidth);
        void windowHeightChanged(int windowHeight);
        void enableVirtualCameraChanged(bool enableVirtualCamera);
        void virtualCameraStateChanged(AkElement::ElementState virtualCameraState);
        void error(const QString &message);
        void interfaceLoaded();

    public slots:
        void setWindowWidth(int windowWidth);
        void setWindowHeight(int windowHeight);
        void setEnableVirtualCamera(bool enableVirtualCamera);
        void setVirtualCameraState(AkElement::ElementState virtualCameraState);
        void resetWindowWidth();
        void resetWindowHeight();
        void resetEnableVirtualCamera();
        void resetVirtualCameraState();
        void loadConfigs();
        void saveVirtualCameraConvertLib(const QString &convertLib);
        void saveVirtualCameraOutputLib(const QString &outputLib);
        void saveConfigs();
        void show();

    private slots:
        void updateVCamCaps(const AkCaps &videoCaps);
        void updateVCamState();
};

#endif // MEDIATOOLS_H
