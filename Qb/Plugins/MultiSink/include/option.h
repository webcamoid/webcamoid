/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2012  Gonzalo Exequiel Pedone
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

#ifndef OPTION_H
#define OPTION_H

#include <QtCore>

class Option: public QObject
{
    Q_OBJECT
    Q_ENUMS(OptionFlags)
    Q_PROPERTY(QString name READ name WRITE setName RESET resetName)
    Q_PROPERTY(OptionFlags flags READ flags WRITE setFlags RESET resetFlags)

    public:
        enum OptionFlags
        {
            OptionFlagsNoFlags,
            OptionFlagsHasValue,
            OptionFlagsIsLong
        };

        explicit Option(QObject *parent=NULL);
        Option(QString name, OptionFlags flags=OptionFlagsNoFlags);
        Option(const Option &other);

        Option &operator =(const Option &other);

        Q_INVOKABLE QString name();
        Q_INVOKABLE OptionFlags flags();

    private:
        QString m_name;
        OptionFlags m_flags;

    public slots:
        void setName(QString name);
        void setFlags(OptionFlags flags);
        void resetName();
        void resetFlags();
};

#endif // OPTION_H
