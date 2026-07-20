/* Webcamoid, camera capture application.
 * Copyright (C) 2016  Gonzalo Exequiel Pedone
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

#ifndef CHANGEHSLELEMENT_H
#define CHANGEHSLELEMENT_H

#include <iak/akvideoeffect.h>

class ChangeHSLElementPrivate;

class ChangeHSLElement: public AkVideoEffect
{
    Q_OBJECT
    Q_PROPERTY(QVariantList kernel
               READ kernel
               WRITE setKernel
               RESET resetKernel
               NOTIFY kernelChanged)

    public:
        ChangeHSLElement();
        ~ChangeHSLElement();

        Q_INVOKABLE QVariantList kernel() const;
        Q_INVOKABLE bool init(QOpenGLBuffer *vbo,
                              QOpenGLBuffer *ibo) override;
        Q_INVOKABLE void process(QOpenGLFramebufferObject *inputFbo,
                                 QOpenGLFramebufferObject *&outputFbo,
                                 qint64 streamId,
                                 qreal pts) override;
                                 Q_INVOKABLE void uninit() override;

    private:
        ChangeHSLElementPrivate *d;

    protected:
        QString controlInterfaceProvide(const QString &controlId) const override;
        void controlInterfaceConfigure(QQmlContext *context,
                                       const QString &controlId) const override;

    signals:
        void kernelChanged(const QVariantList &kernel);

    public slots:
        void setKernel(const QVariantList &kernel);
        void resetKernel();
};

#endif // CHANGEHSLELEMENT_H
