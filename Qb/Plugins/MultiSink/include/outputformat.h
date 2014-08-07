/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

#ifndef OUTPUTFORMAT_H
#define OUTPUTFORMAT_H

#include <QtCore>

#include "outputparams.h"

typedef QSharedPointer<AVFormatContext> FormatContextPtr;
typedef QSharedPointer<AVStream> StreamPtr;
typedef QMap<QString, StreamPtr> StreamMapPtr;

class OutputFormat: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isOpen READ isOpen)
    Q_PROPERTY(FormatContextPtr outputContext READ outputContext)
    Q_PROPERTY(StreamMapPtr streams READ streams)

    public:
        explicit OutputFormat(QObject *parent=NULL);

        Q_INVOKABLE bool isOpen() const;
        Q_INVOKABLE FormatContextPtr outputContext() const;
        Q_INVOKABLE StreamMapPtr streams() const;

        Q_INVOKABLE bool open(const QString &fileName,
                              const QMap<QString, OutputParams> &outputParams,
                              const QVariantMap &outputOptions, const QVariantMap &inputOptions);

    private:
        bool m_isOpen;
        FormatContextPtr m_outputContext;
        StreamMapPtr m_streams;

        bool addStream(const QString &input,
                       const OutputParams &outputParams,
                       const QVariantMap &codecOptions);

    public slots:
        void close();
};

#endif // OUTPUTFORMAT_H
