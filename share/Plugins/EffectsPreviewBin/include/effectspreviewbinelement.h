/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#ifndef EFFECTSPREVIEWBINELEMENT_H
#define EFFECTSPREVIEWBINELEMENT_H

#include <QtGui>
#include <gst/gst.h>

#include "element.h"

class EffectsPreviewBinElement: public Element
{
    Q_OBJECT

    Q_PROPERTY(QStringList effects READ effects
                                   WRITE setEffects
                                   RESET resetEffects)

    Q_PROPERTY(QSize frameSize READ frameSize
                               WRITE setFrameSize
                               RESET resetFrameSize)

    public:
        explicit EffectsPreviewBinElement();
        ~EffectsPreviewBinElement();

        Q_INVOKABLE QStringList effects();
        Q_INVOKABLE QSize frameSize();

        Q_INVOKABLE ElementState state();

    private:
        QStringList m_effects;
        QSize m_frameSize;
        ElementState m_state;

        QMutex m_mutex;
        QMap<QString, int> m_callBack;
        GstElement *m_pipeline;
        QImage m_iFrame;
        QImage m_oFrame;
        QSize m_curFrameSize;

        QString hashFromName(QString name="");
        QString nameFromHash(QString hash="");
        static void needData(GstElement *appsrc, guint size, gpointer self);
        static void newBuffer(GstElement *appsink, gpointer self);

    public slots:
        void setEffects(QStringList effects);
        void setFrameSize(QSize frameSize);
        void resetEffects();
        void resetFrameSize();

        void iStream(const void *data, int datalen, QString dataType);
        void setState(ElementState state);
        void resetState();
};

#endif // EFFECTSPREVIEWBINELEMENT_H
