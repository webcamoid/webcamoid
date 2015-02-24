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
 * Email     : hipersayan DOT x AT gmail DOT com
 * Web-Site 1: http://github.com/hipersayanX/Webcamoid
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "outputformat.h"

OutputFormat::OutputFormat(QObject *parent): QObject(parent)
{
    this->m_isOpen = false;
}

bool OutputFormat::isOpen() const
{
    return this->m_isOpen;
}

FormatContextPtr OutputFormat::outputContext() const
{
    return this->m_outputContext;
}

StreamMapPtr OutputFormat::streams() const
{
    return this->m_streams;
}

bool OutputFormat::open(const QString &fileName,
                        const QMap<QString, OutputParams> &outputParams,
                        const QVariantMap &outputOptions,
                        const QVariantMap &inputOptions)
{
    if (fileName.isEmpty())
        return false;

    if (this->isOpen())
        return !this->isOpen();

    AVFormatContext *outputContext = NULL;

    if (outputOptions.contains("f"))
        avformat_alloc_output_context2(&outputContext,
                                       NULL,
                                       outputOptions["f"].toString().toStdString().c_str(),
                                       fileName.toStdString().c_str());
    else
        avformat_alloc_output_context2(&outputContext,
                                       NULL,
                                       NULL,
                                       fileName.toStdString().c_str());

    this->m_isOpen = false;

    if (!outputContext)
        return this->m_isOpen;

    this->m_outputContext = FormatContextPtr(outputContext, CustomDeleters::deleteFormatContext);
    this->m_streams.clear();

    foreach (QString input, outputParams.keys()) {
        QVariantMap streamOptions = inputOptions[input].toMap();
        QVariantMap codecOptions;

        if (streamOptions.contains("opt"))
            codecOptions = streamOptions["opt"].toMap();

        if (!this->addStream(input, outputParams[input], codecOptions)) {
            this->m_streams.clear();

            return this->m_isOpen;
        }
    }

    av_dump_format(this->m_outputContext.data(),
                   0,
                   fileName.toStdString().c_str(),
                   1);

    if (!(this->m_outputContext->oformat->flags & AVFMT_NOFILE)) {
        int error = avio_open(&this->m_outputContext->pb,
                              fileName.toStdString().c_str(),
                              AVIO_FLAG_WRITE);

        if (error < 0) {
            char errorStr[1024];
            av_strerror(AVERROR(error), errorStr, 1024);
            qDebug() << "Error in MultiSink(" << this->objectName() << "): " << errorStr;

            return this->m_isOpen;
        }
    }

    if (avformat_write_header(this->m_outputContext.data(), NULL) < 0)
        return this->m_isOpen;

    return this->m_isOpen = true;
}

bool OutputFormat::addStream(const QString &input,
                             const OutputParams &outputParams,
                             const QVariantMap &codecOptions)
{
    StreamPtr stream = StreamPtr(avformat_new_stream(this->m_outputContext.data(),
                                                     outputParams.codecContext()->codec),
                                 CustomDeleters::deleteStream);

    if (!stream || avcodec_copy_context(stream->codec, outputParams.codecContext().data()) != 0)
        return false;

    stream->id = outputParams.outputIndex();

    // Some formats want stream headers to be separate.
    if (this->m_outputContext->oformat->flags & AVFMT_GLOBALHEADER)
        stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;

    const AVCodec *codec = outputParams.codecContext()->codec;

    if (!codec)
        return false;

    AVDictionary *options = NULL;

    foreach (QString key, codecOptions.keys()) {
        QString value = codecOptions[key].toString();

        av_dict_set(&options,
                    key.toStdString().c_str(),
                    value.toStdString().c_str(),
                    0);
    }

    if (avcodec_open2(stream->codec, codec, &options) < 0)
        return false;

    av_dict_free(&options);

    this->m_streams[input] = stream;

    return true;
}

void OutputFormat::close()
{
    if (!this->m_isOpen)
        return;

    // Write the trailer, if any. The trailer must be written before you
    // close the CodecContexts open when you wrote the header; otherwise
    // av_write_trailer() may try to use memory that was freed on
    // av_codec_close().
    av_write_trailer(this->m_outputContext.data());

    // Close each codec.
    foreach (StreamPtr stream, this->m_streams)
        avcodec_close(stream->codec);

    // Free the streams.
    this->m_streams.clear();

    if (!(this->m_outputContext->oformat->flags & AVFMT_NOFILE))
        // Close the output file.
        avio_close(this->m_outputContext->pb);

    // free the stream
    this->m_outputContext.clear();

    this->m_isOpen = false;
}
