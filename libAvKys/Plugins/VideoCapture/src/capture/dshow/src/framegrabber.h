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

#ifndef FRAMEGRABBER_H
#define FRAMEGRABBER_H

#include <QObject>

#include "samplegrabber.h"

class FrameGrabber: public QObject, public ISampleGrabberCB
{
    Q_OBJECT

    public:
        FrameGrabber();
        virtual ~FrameGrabber();
        STDMETHODIMP_(ULONG) AddRef() override;
        STDMETHODIMP_(ULONG) Release() override;
        STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
        STDMETHODIMP SampleCB(double time, IMediaSample *sample) override;
        STDMETHODIMP BufferCB(double time, BYTE *buffer, long bufferSize) override;

    signals:
        void frameReady(qreal time, const QByteArray &packet);
        void sampleReady(qreal time, IMediaSample *sample);
};

#endif // FRAMEGRABBER_H
