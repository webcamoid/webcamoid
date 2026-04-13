/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#ifndef AKVIDEOEFFECT_H
#define AKVIDEOEFFECT_H

#include <QObject>

#include "../akcommons.h"

class AkVideoEffect;
using AkVideoEffectPtr = QSharedPointer<AkVideoEffect>;

class QOpenGLFunctions;
class QOpenGLBuffer;
class QOpenGLFramebufferObject;
class QQmlEngine;
class QQmlContext;

class AKCOMMONS_EXPORT AkVideoEffect: public QObject
{
    Q_OBJECT

    public:
        explicit AkVideoEffect(QObject *parent=nullptr);
        virtual ~AkVideoEffect();

        Q_INVOKABLE virtual QObject *controlInterface(QQmlEngine *engine,
                                                      const QString &controlId) const;
        Q_INVOKABLE virtual bool init(QOpenGLBuffer *vbo,
                                      QOpenGLBuffer *ibo) = 0;
        Q_INVOKABLE virtual void process(const QOpenGLFramebufferObject *inputFbo,
                                         QOpenGLFramebufferObject *&outputFbo,
                                         qint64 streamId,
                                         qreal pts) = 0;
        Q_INVOKABLE virtual void uninit() {}

    protected:
        QOpenGLFunctions *m_gl {nullptr};

        virtual QString controlInterfaceProvide(const QString &controlId) const;
        virtual void controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const;

    public Q_SLOTS:
        void setGLFunctions(QOpenGLFunctions *gl);
};

#endif // AKVIDEOEFFECT_H
