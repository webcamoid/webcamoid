/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2016  Gonzalo Exequiel Pedone
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

#ifndef AKQML_H
#define AKQML_H

#include <QQuickItem>
#include <ak.h>

class AkQml: public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(AkQml)

    public:
        AkQml(QQuickItem *parent=NULL);
        ~AkQml();

        Q_INVOKABLE qint64 id() const;

        Q_INVOKABLE QObject *newFrac() const;
        Q_INVOKABLE QObject *newFrac(qint64 num, qint64 den) const;
        Q_INVOKABLE QObject *newFrac(const QString &frac) const;
        Q_INVOKABLE QObject *newFrac(const AkFrac &frac) const;

        Q_INVOKABLE QObject *newCaps() const;
        Q_INVOKABLE QObject *newCaps(const QVariantMap &caps) const;
        Q_INVOKABLE QObject *newCaps(const QString &caps) const;
        Q_INVOKABLE QObject *newCaps(const AkCaps &caps) const;

        Q_INVOKABLE QObject *newAudioCaps() const;
        Q_INVOKABLE QObject *newAudioCaps(const QVariantMap &caps) const;
        Q_INVOKABLE QObject *newAudioCaps(const QString &caps) const;
        Q_INVOKABLE QObject *newAudioCaps(const AkCaps &caps) const;
        Q_INVOKABLE QObject *newAudioCaps(const AkAudioCaps &caps) const;
        Q_INVOKABLE QObject *newAkAudioCaps(AkAudioCaps::SampleFormat format,
                                            int channels,
                                            int rate);

        Q_INVOKABLE QObject *newVideoCaps() const;
        Q_INVOKABLE QObject *newVideoCaps(const QVariantMap &caps) const;
        Q_INVOKABLE QObject *newVideoCaps(const QString &caps) const;
        Q_INVOKABLE QObject *newVideoCaps(const AkCaps &caps) const;
        Q_INVOKABLE QObject *newVideoCaps(const AkVideoCaps &caps) const;

        Q_INVOKABLE QObject *newElement(const QString &pluginId,
                                        const QString &elementName="") const;

        Q_INVOKABLE QVariant varFrac(AkFrac *frac) const;
        Q_INVOKABLE QVariant varCaps(AkCaps *caps) const;
};

#endif // AKQML_H
