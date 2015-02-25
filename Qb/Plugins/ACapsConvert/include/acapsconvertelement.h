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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef ACAPSCONVERTELEMENT_H
#define ACAPSCONVERTELEMENT_H

extern "C"
{
    #include <libavdevice/avdevice.h>
    #include <libswresample/swresample.h>
    #include <libavutil/opt.h>
}

#include <qb.h>

class ACapsConvertElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString caps READ caps WRITE setCaps RESET resetCaps)

    public:
        explicit ACapsConvertElement();
        ~ACapsConvertElement();

        Q_INVOKABLE QString caps() const;

    private:
        QbCaps m_caps;

        QbCaps m_curInputCaps;
        SwrContext *m_resampleContext;

        void deleteSwrContext();

    public slots:
        void setCaps(const QString &caps);
        void resetCaps();

        QbPacket iStream(const QbPacket &packet);
};

#endif // ACAPSCONVERTELEMENT_H
