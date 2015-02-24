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

#ifndef PARSEDOPTION_H
#define PARSEDOPTION_H

#include <QObject>
#include <QVariant>
#include <QDebug>

class ParsedOption: public QObject
{
    Q_OBJECT
    Q_ENUMS(OptionType)
    Q_PROPERTY(QString key READ key WRITE setKey RESET resetKey)
    Q_PROPERTY(QVariant value READ value WRITE setValue RESET resetValue)
    Q_PROPERTY(OptionType type READ type WRITE setType RESET resetType)

    public:
        enum OptionType
        {
            OptionTypeNone,
            OptionTypeData,
            OptionTypeSingle,
            OptionTypePair
        };

        explicit ParsedOption(QObject *parent=NULL);
        ParsedOption(QString key, QVariant value=QVariant(), OptionType type=OptionTypeNone);
        ParsedOption(const ParsedOption &other);

        ParsedOption &operator =(const ParsedOption &other);

        Q_INVOKABLE QString key() const;
        Q_INVOKABLE QVariant value() const;
        Q_INVOKABLE OptionType type() const;

    private:
        QString m_key;
        QVariant m_value;
        OptionType m_type;

        friend QDebug operator <<(QDebug debug, const ParsedOption &option);

    public slots:
        void setKey(QString key);
        void setValue(QVariant value);
        void setType(OptionType type);
        void resetKey();
        void resetValue();
        void resetType();
};

QDebug operator <<(QDebug debug, const ParsedOption &option);

Q_DECLARE_METATYPE(ParsedOption)

#endif // PARSEDOPTION_H
