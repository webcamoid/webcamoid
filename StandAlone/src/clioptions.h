/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2017  Gonzalo Exequiel Pedone
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

#ifndef CLIOPTIONS_H
#define CLIOPTIONS_H

#include <QCommandLineParser>

class CliOptions: public QCommandLineParser
{
    Q_PROPERTY(QCommandLineOption configPathOpt
               READ configPathOpt)
    Q_PROPERTY(QCommandLineOption qmlPathOpt
               READ qmlPathOpt)
    Q_PROPERTY(QCommandLineOption recursiveOpt
               READ recursiveOpt)
    Q_PROPERTY(QCommandLineOption pluginPathsOpt
               READ pluginPathsOpt)
    Q_PROPERTY(QCommandLineOption blackListOpt
               READ blackListOpt)
    Q_PROPERTY(QCommandLineOption vcamPathOpt
               READ vcamPathOpt)

    public:
        explicit CliOptions();
        ~CliOptions();

        Q_INVOKABLE QCommandLineOption configPathOpt() const;
        Q_INVOKABLE QCommandLineOption qmlPathOpt() const;
        Q_INVOKABLE QCommandLineOption recursiveOpt() const;
        Q_INVOKABLE QCommandLineOption pluginPathsOpt() const;
        Q_INVOKABLE QCommandLineOption blackListOpt() const;
        Q_INVOKABLE QCommandLineOption vcamPathOpt() const;

    private:
        QCommandLineOption *m_configPathOpt;
        QCommandLineOption *m_qmlPathOpt;
        QCommandLineOption *m_recursiveOpt;
        QCommandLineOption *m_pluginPathsOpt;
        QCommandLineOption *m_blackListOpt;
        QCommandLineOption *m_vcamPathOpt;

        QString convertToAbsolute(const QString &path) const;
};

#endif // CLIOPTIONS_H
