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
class AkAudioPacket;

class AKCOMMONS_EXPORT AkVideoEffect: public QObject
{
    Q_OBJECT

    public:
        explicit AkVideoEffect(QObject *parent=nullptr);
        virtual ~AkVideoEffect();

        // The effect can receive and process audio, through iAudioStream().
        // The application uses it to warn the user when the audio source of
        // the effect is not set. It is false by default.
        Q_INVOKABLE virtual bool canProcessAudio() const;
        Q_INVOKABLE virtual QObject *controlInterface(QQmlEngine *engine,
                                                      const QString &controlId) const;
        Q_INVOKABLE virtual bool init(QOpenGLBuffer *vbo,
                                      QOpenGLBuffer *ibo) = 0;
        Q_INVOKABLE virtual void process(QOpenGLFramebufferObject *inputFbo,
                                         QOpenGLFramebufferObject *&outputFbo,
                                         qint64 streamId,
                                         qreal pts) = 0;
        Q_INVOKABLE virtual void uninit() = 0;

    protected:
        QOpenGLFunctions *m_gl {nullptr};

        virtual QString controlInterfaceProvide(const QString &controlId) const;
        virtual void controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const;

    public Q_SLOTS:
        void setGLFunctions(QOpenGLFunctions *gl);
        // Receives the audio from the source the effect is applied to, or the
        // audio returned by the previous effect in the chain. The effect can
        // process it and return the result to be sent to the next effect.
        // By default the audio is passed through unchanged.
        //
        // It is called from the audio thread, not from the OpenGL thread, so
        // the implementation must be fast, thread-safe and must not use any
        // OpenGL function.
        virtual AkAudioPacket iAudioStream(const AkAudioPacket &audioPacket);
        virtual void reset() = 0;
};

#endif // AKVIDEOEFFECT_H
