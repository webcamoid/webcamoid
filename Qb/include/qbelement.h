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
 * Email   : hipersayan DOT x AT gmail DOT com
 * Web-Site: http://github.com/hipersayanX/webcamoid
 */

#ifndef QBELEMENT_H
#define QBELEMENT_H

#include <QStringList>
#include <QQmlEngine>

#include "qbpacket.h"

#define qbSend(packet) { \
    if (packet) \
        emit this->oStream(packet); \
    \
    return packet; \
}

class QbElement;
class QbElementPrivate;

typedef QSharedPointer<QbElement> QbElementPtr;

/// Plugin template.
class QbElement: public QObject
{
    Q_OBJECT
    Q_ENUMS(ElementState)
    Q_ENUMS(SearchPaths)
    Q_PROPERTY(QString pluginId
               READ pluginId)
    Q_PROPERTY(QbElement::ElementState state
               READ state
               WRITE setState
               RESET resetState
               NOTIFY stateChanged)

    public:
        enum ElementState
        {
            ElementStateNull,
            ElementStatePaused,
            ElementStatePlaying
        };

        enum SearchPaths
        {
            SearchPathsAll,
            SearchPathsDefaults,
            SearchPathsExtras
        };

        explicit QbElement(QObject *parent=NULL);
        virtual ~QbElement();

        Q_INVOKABLE QString pluginId() const;
        Q_INVOKABLE virtual QbElement::ElementState state() const;
        Q_INVOKABLE virtual QObject *controlInterface(QQmlEngine *engine,
                                                      const QString &controlId) const;

        Q_INVOKABLE virtual bool link(const QObject *dstElement,
                                      Qt::ConnectionType connectionType=Qt::AutoConnection) const;

        Q_INVOKABLE virtual bool link(const QbElementPtr &dstElement,
                                      Qt::ConnectionType connectionType=Qt::AutoConnection) const;

        Q_INVOKABLE virtual bool unlink(const QObject *dstElement) const;
        Q_INVOKABLE virtual bool unlink(const QbElementPtr &dstElement) const;

        Q_INVOKABLE static bool link(const QbElementPtr &srcElement,
                                     const QObject *dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);
        Q_INVOKABLE static bool link(const QbElementPtr &srcElement,
                                     const QbElementPtr &dstElement,
                                     Qt::ConnectionType connectionType=Qt::AutoConnection);
        Q_INVOKABLE static bool unlink(const QbElementPtr &srcElement,
                                       const QObject *dstElement);
        Q_INVOKABLE static bool unlink(const QbElementPtr &srcElement,
                                       const QbElementPtr &dstElement);
        Q_INVOKABLE static QbElementPtr create(const QString &pluginId,
                                               const QString &elementName="");
        Q_INVOKABLE static bool recursiveSearch();
        Q_INVOKABLE static void setRecursiveSearch(bool enable);
        Q_INVOKABLE static QStringList searchPaths(SearchPaths pathType=SearchPathsAll);
        Q_INVOKABLE static void addSearchPath(const QString &path);
        Q_INVOKABLE static void setSearchPaths(const QStringList &searchPaths);
        Q_INVOKABLE static void resetSearchPaths();
        Q_INVOKABLE static QStringList listPlugins(const QString &type="");
        Q_INVOKABLE static QStringList listPluginPaths(const QString &searchPath);
        Q_INVOKABLE static QStringList listPluginPaths();
        Q_INVOKABLE static QString pluginPath(const QString &pluginId);
        Q_INVOKABLE static QVariantMap pluginInfo(const QString &pluginId);
        Q_INVOKABLE static void clearCache();

    private:
        QbElementPrivate *d;

    protected:
        virtual void stateChange(QbElement::ElementState from, QbElement::ElementState to);

    signals:
        void stateChanged(QbElement::ElementState state);
        void oStream(const QbPacket &packet);

    public slots:
        virtual QbPacket iStream(const QbPacket &packet);
        virtual void setState(QbElement::ElementState state);
        virtual void resetState();
};

QDataStream &operator >>(QDataStream &istream, QbElement::ElementState &state);
QDataStream &operator <<(QDataStream &ostream, QbElement::ElementState state);
Q_DECLARE_METATYPE(QbElement::ElementState)

#endif // QBELEMENT_H
