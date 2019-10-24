/* Webcamoid, webcam capture application.
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

#ifndef AKQML_H
#define AKQML_H

#include <QQuickItem>
#include <akaudiocaps.h>
#include <akvideocaps.h>
#include <akunit.h>

class AkFrac;
class AkCaps;

class AkQml: public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(AkQml)

    public:
        AkQml(QQuickItem *parent=nullptr);
        ~AkQml() = default;

        Q_INVOKABLE qint64 id() const;

        Q_INVOKABLE QObject *newFrac() const;
        Q_INVOKABLE QObject *newFrac(qint64 num, qint64 den) const;
        Q_INVOKABLE QObject *newFrac(const QString &frac) const;
        Q_INVOKABLE QObject *newFrac(const AkFrac &frac) const;

        Q_INVOKABLE QObject *newCaps(const QString &mimeType={}) const;
        Q_INVOKABLE QObject *newCaps(const AkCaps &caps) const;

        Q_INVOKABLE QObject *newAudioCaps() const;
        Q_INVOKABLE QObject *newAudioCaps(const AkCaps &caps) const;
        Q_INVOKABLE QObject *newAudioCaps(const AkAudioCaps &caps) const;
        Q_INVOKABLE QObject *newAudioCaps(AkAudioCaps::SampleFormat format,
                                          AkAudioCaps::ChannelLayout layout,
                                          int rate,
                                          int samples=0,
                                          bool planar=false,
                                          int align=1);
        Q_INVOKABLE QObject *newAudioCaps(const QString &format,
                                          const QString &layout,
                                          int rate,
                                          int samples=0,
                                          bool planar=false,
                                          int align=1);

        Q_INVOKABLE QObject *newVideoCaps() const;
        Q_INVOKABLE QObject *newVideoCaps(const AkCaps &caps) const;
        Q_INVOKABLE QObject *newVideoCaps(const AkVideoCaps &caps) const;
        Q_INVOKABLE QObject *newVideoCaps(AkVideoCaps::PixelFormat format,
                                          int width,
                                          int height,
                                          const AkFrac &fps,
                                          int align=1) const;
        Q_INVOKABLE QObject *newVideoCaps(const QString &format,
                                          int width,
                                          int height,
                                          const AkFrac &fps,
                                          int align=1) const;
        Q_INVOKABLE QObject *newVideoCaps(AkVideoCaps::PixelFormat format,
                                          const QSize &size,
                                          const AkFrac &fps,
                                          int align=1) const;
        Q_INVOKABLE QObject *newVideoCaps(const QString &format,
                                          const QSize &size,
                                          const AkFrac &fps,
                                          int align=1) const;

        Q_INVOKABLE QObject *newUnit(qreal value=0.0,
                                     AkUnit::Unit unit=AkUnit::px);
        Q_INVOKABLE QObject *newUnit(qreal value,
                                     const QString &unit);
        Q_INVOKABLE QObject *newUnit(qreal value,
                                     AkUnit::Unit unit,
                                     QObject *parent);
        Q_INVOKABLE QObject *newUnit(qreal value,
                                     const QString &unit,
                                     QObject *parent);

        Q_INVOKABLE QObject *newElement(const QString &pluginId,
                                        const QString &pluginSub={}) const;

        Q_INVOKABLE QVariantList newList(const QList<AkAudioCaps::SampleFormat> &sampleFormats) const;
        Q_INVOKABLE QVariantList newList(const QList<AkAudioCaps::ChannelLayout> &channelLayouts) const;

        Q_INVOKABLE QVariant varFrac(QObject *frac) const;
        Q_INVOKABLE QVariant varFrac(AkFrac *frac) const;
        Q_INVOKABLE QVariant varFrac(qint64 num, qint64 den) const;

        Q_INVOKABLE QVariant varCaps(QObject *caps) const;
        Q_INVOKABLE QVariant varCaps(AkCaps *caps) const;

        Q_INVOKABLE QVariant varAudioCaps(QObject *caps) const;
        Q_INVOKABLE QVariant varAudioCaps(AkAudioCaps *caps) const;

        Q_INVOKABLE QVariant varVideoCaps(QObject *caps) const;
        Q_INVOKABLE QVariant varVideoCaps(AkAudioCaps *caps) const;

        Q_INVOKABLE QVariant varUnit(QObject *caps) const;
        Q_INVOKABLE QVariant varUnit(AkUnit *caps) const;
};

#endif // AKQML_H
