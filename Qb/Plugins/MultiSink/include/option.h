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

#ifndef OPTION_H
#define OPTION_H

#include <QObject>
#include <QDebug>

class Option: public QObject
{
    Q_OBJECT
    Q_ENUMS(OptionFlags)
    Q_PROPERTY(QString name
               READ name
               WRITE setName
               RESET resetName
               NOTIFY nameChanged)
    Q_PROPERTY(QString comment
               READ comment
               WRITE setComment
               RESET resetComment
               NOTIFY commentChanged)
    Q_PROPERTY(QString valregex
               READ valregex
               WRITE setValregex
               RESET resetValregex
               NOTIFY valregexChanged)
    Q_PROPERTY(OptionFlags flags
               READ flags
               WRITE setFlags
               RESET resetFlags
               NOTIFY flagsChanged)

    public:
        enum OptionFlags
        {
            OptionFlagsNoFlags,
            OptionFlagsHasValue,
            OptionFlagsIsLong
        };

        explicit Option(QObject *parent=NULL);

        Option(const QString &name,
               const QString &comment="",
               const QString &valregex=".*",
               const OptionFlags &flags=OptionFlagsNoFlags);

        Option(const Option &other);
        Option &operator =(const Option &other);

        Q_INVOKABLE QString name() const;
        Q_INVOKABLE QString comment() const;
        Q_INVOKABLE QString valregex() const;
        Q_INVOKABLE OptionFlags flags() const;

    private:
        QString m_name;
        QString m_comment;
        QString m_valregex;
        OptionFlags m_flags;

        friend QDebug operator <<(QDebug debug, const Option &option);

    signals:
        void nameChanged(const QString &name);
        void commentChanged(const QString &comment);
        void valregexChanged(const QString &valregex);
        void flagsChanged(const OptionFlags &flags);

    public slots:
        void setName(const QString &name);
        void setComment(const QString &comment);
        void setValregex(const QString &valregex);
        void setFlags(const OptionFlags &flags);
        void resetName();
        void resetComment();
        void resetValregex();
        void resetFlags();
};

QDebug operator <<(QDebug debug, const Option &option);

Q_DECLARE_METATYPE(Option)

#endif // OPTION_H
