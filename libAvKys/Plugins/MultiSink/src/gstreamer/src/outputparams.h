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

#ifndef OUTPUTPARAMS_H
#define OUTPUTPARAMS_H

#include <QObject>

class OutputParamsPrivate;

class OutputParams: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int inputIndex
               READ inputIndex
               WRITE setInputIndex
               RESET resetInputIndex
               NOTIFY inputIndexChanged)
    Q_PROPERTY(quint64 nFrame
               READ nFrame
               WRITE setNFrame
               RESET resetNFrame
               NOTIFY nFrameChanged)

    public:
        OutputParams(int inputIndex=0, QObject *parent=nullptr);
        OutputParams(const OutputParams &other);
        ~OutputParams();

        OutputParams &operator =(const OutputParams &other);

        Q_INVOKABLE int inputIndex() const;
        Q_INVOKABLE int &inputIndex();
        Q_INVOKABLE quint64 nFrame() const;
        Q_INVOKABLE quint64 &nFrame();
        Q_INVOKABLE qint64 nextPts(qint64 pts, qint64 id);

    private:
        OutputParamsPrivate *d;

    signals:
        void inputIndexChanged(int inputIndex);
        void nFrameChanged(quint64 nFrame);

    public slots:
        void setInputIndex(int inputIndex);
        void setNFrame(quint64 nFrame);
        void resetInputIndex();
        void resetNFrame();
};

Q_DECLARE_METATYPE(OutputParams)

#endif // OUTPUTPARAMS_H
