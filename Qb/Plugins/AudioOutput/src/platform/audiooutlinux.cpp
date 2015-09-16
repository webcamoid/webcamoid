/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2015  Gonzalo Exequiel Pedone
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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#include <QCoreApplication>
#include <QMap>

#include "platform/audiooutlinux.h"

typedef QMap<QbAudioCaps::SampleFormat, pa_sample_format_t> SampleFormatMap;

inline SampleFormatMap initSampleFormatMap()
{
    SampleFormatMap sampleFormat;
    sampleFormat[QbAudioCaps::SampleFormat_u8] = PA_SAMPLE_U8;
    sampleFormat[QbAudioCaps::SampleFormat_s16] = PA_SAMPLE_S16LE;
    sampleFormat[QbAudioCaps::SampleFormat_s32] = PA_SAMPLE_S32LE;
    sampleFormat[QbAudioCaps::SampleFormat_flt] = PA_SAMPLE_FLOAT32LE;

    return sampleFormat;
}

Q_GLOBAL_STATIC_WITH_ARGS(SampleFormatMap, sampleFormats, (initSampleFormatMap()))

AudioOut::AudioOut(QObject *parent):
    QObject(parent)
{
    this->m_paSimple = NULL;
}

AudioOut::~AudioOut()
{
    this->uninit();
}

QString AudioOut::error() const
{
    return this->m_error;
}

bool AudioOut::init(QbAudioCaps::SampleFormat sampleFormat,
                    int channels,
                    int sampleRate)
{
    int error;

    pa_sample_spec ss;
    ss.format = sampleFormats->value(sampleFormat);
    ss.channels = channels;
    ss.rate = sampleRate;

    this->m_paSimple = pa_simple_new(NULL,
                                     QCoreApplication::applicationName().toStdString().c_str(),
                                     PA_STREAM_PLAYBACK,
                                     NULL,
                                     QCoreApplication::organizationName().toStdString().c_str(),
                                     &ss,
                                     NULL,
                                     NULL,
                                     &error);

    if (!this->m_paSimple) {
        this->m_error = QString(pa_strerror(error));
        emit this->errorChanged(this->m_error);

        return false;
    }

    return true;
}

bool AudioOut::write(const QByteArray &frame)
{
    if (!this->m_paSimple)
        return false;

    int error;

    if (pa_simple_write(this->m_paSimple,
                        frame.data(),
                        frame.size(),
                        &error) < 0) {
        this->m_error = QString(pa_strerror(error));
        emit this->errorChanged(this->m_error);

        return false;
    }

    return true;
}

bool AudioOut::uninit()
{
    bool ok = true;

    if (this->m_paSimple) {
        int error;

        if (pa_simple_drain(this->m_paSimple, &error) < 0) {
            this->m_error = QString(pa_strerror(error));
            emit this->errorChanged(this->m_error);
            ok = false;
        }

        pa_simple_free(this->m_paSimple);
        this->m_paSimple = NULL;
    } else
        ok = false;

    return ok;
}
