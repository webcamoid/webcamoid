/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://kde-apps.org/content/show.php/Webcamoid?content=144796
 */

#ifndef MULTISRCELEMENT_H
#define MULTISRCELEMENT_H

#include <QtGui>
#include <qb.h>

#include "abstractstream.h"

class MultiSrcElement: public QbElement
{
    Q_OBJECT
    Q_PROPERTY(QString location READ location WRITE setLocation RESET resetLocation)
    Q_PROPERTY(bool loop READ loop WRITE setLoop RESET resetLoop)
    Q_PROPERTY(QVariantMap streamCaps READ streamCaps)

    Q_PROPERTY(QStringList filterStreams READ filterStreams
                                         WRITE setFilterStreams
                                         RESET resetFilterStreams)

    Q_PROPERTY(bool audioAlign READ audioAlign
                               WRITE setAudioAlign
                               RESET resetAudioAlign)

    public:
        explicit MultiSrcElement();
        ~MultiSrcElement();

        Q_INVOKABLE QString location() const;
        Q_INVOKABLE bool loop() const;
        Q_INVOKABLE QVariantMap streamCaps();
        Q_INVOKABLE QStringList filterStreams() const;
        Q_INVOKABLE bool audioAlign() const;

        Q_INVOKABLE int defaultStream(const QString &mimeType);

    protected:
        bool init();
        void uninit();
        void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    private:
        QString m_location;
        bool m_loop;
        QStringList m_filterStreams;
        bool m_audioAlign;

        FormatContextPtr m_inputContext;
        QTimer m_timer;
        QMap<int, AbstractStreamPtr> m_streams;

        static void deleteFormatContext(AVFormatContext *context);

        inline int roundDown(int value, int multiply)
        {
            return value - value % multiply;
        }

    signals:
        void error(QString message);

    public slots:
        void setLocation(const QString &location);
        void setLoop(bool loop);
        void setFilterStreams(const QStringList &filterStreams);
        void setAudioAlign(bool audioAlign);
        void resetLocation();
        void resetLoop();
        void resetFilterStreams();
        void resetAudioAlign();

        void setState(QbElement::ElementState state);

    private slots:
        void readPackets();
};

#endif // MULTISRCELEMENT_H
