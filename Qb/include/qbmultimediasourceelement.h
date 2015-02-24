/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
 *
 * Webcamod is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamod is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamod. If not, see <http://www.gnu.org/licenses/>.
 *
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#ifndef QBMULTIMEDIASOURCEELEMENT_H
#define QBMULTIMEDIASOURCEELEMENT_H

#include <qbelement.h>

class QbMultimediaSourceElement;
class QbMultimediaSourceElementPrivate;

typedef QSharedPointer<QbMultimediaSourceElement> QbMultimediaSourceElementPtr;

class QbMultimediaSourceElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QStringList medias
               READ medias
               NOTIFY mediasChanged)
    Q_PROPERTY(QString media
               READ media
               WRITE setMedia
               RESET resetMedia
               NOTIFY mediaChanged)
    Q_PROPERTY(QList<int> streams
               READ streams
               WRITE setStreams
               RESET resetStreams
               NOTIFY streamsChanged)
    Q_PROPERTY(bool loop
               READ loop
               WRITE setLoop
               RESET resetLoop
               NOTIFY loopChanged)

    public:
        QbMultimediaSourceElement(QObject *parent=NULL);
        ~QbMultimediaSourceElement();

        Q_INVOKABLE virtual QStringList medias() const;
        Q_INVOKABLE virtual QString media() const;
        Q_INVOKABLE virtual QList<int> streams() const;
        Q_INVOKABLE virtual bool loop() const;

        Q_INVOKABLE virtual int defaultStream(const QString &mimeType) const;
        Q_INVOKABLE virtual QString description(const QString &media) const;
        Q_INVOKABLE virtual QbCaps caps(int stream) const;
        Q_INVOKABLE virtual bool isCompressed(int stream) const;

    private:
        QbMultimediaSourceElementPrivate *d;

    signals:
        void mediasChanged(const QStringList &medias);
        void mediaChanged(const QString &media);
        void streamsChanged(const QList<int> &streams);
        void loopChanged(bool loop);

    public slots:
        virtual void setMedia(const QString &media);
        virtual void setStreams(const QList<int> &streams);
        virtual void setLoop(bool loop);
        virtual void resetMedia();
        virtual void resetStreams();
        virtual void resetLoop();
};

#endif // QBMULTIMEDIASOURCEELEMENT_H
