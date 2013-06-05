/* Webcamod, webcam capture plasmoid.
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

#ifndef OUTPUTOPTIONS_H
#define OUTPUTOPTIONS_H

#include <qb.h>
#include "customdeleters.h"

typedef QSharedPointer<AVCodecContext> CodecContextPtr;

class OutputOptions: public QObject
{
    Q_OBJECT
    Q_PROPERTY(CodecContextPtr codecContext READ codecContext WRITE setCodecContext RESET resetCodecContext)
    Q_PROPERTY(QbCaps caps READ caps WRITE setCaps RESET resetCaps)

    public:
        explicit OutputOptions(QObject *parent=NULL);
        OutputOptions(CodecContextPtr codecContext, QbCaps caps);
        OutputOptions(const OutputOptions &other);
        OutputOptions &operator =(const OutputOptions &other);

        Q_INVOKABLE CodecContextPtr codecContext() const;
        Q_INVOKABLE QbCaps caps() const;

    private:
        CodecContextPtr m_codecContext;
        QbCaps m_caps;

    public slots:
        void setCodecContext(CodecContextPtr codecContext);
        void setCaps(QbCaps caps);
        void resetCodecContext();
        void resetCaps();
};

Q_DECLARE_METATYPE(OutputOptions)

#endif // OUTPUTOPTIONS_H
