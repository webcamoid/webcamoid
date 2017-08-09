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

#include <QTimer>
#include <QGLContext>
#include <QGLFramebufferObject>
#import <Syphon.h>

#include "syphonioelement.h"
#include "serverobserver.h"

class SyphonIOElementPrivate
{
    public:
        id m_serverObserver;

        explicit SyphonIOElementPrivate(SyphonIOElement *element)
        {
            this->m_serverObserver = [[ServerObserver alloc]
                                      initWithIOElement: element];

            [[NSNotificationCenter defaultCenter]
             addObserver: this->m_serverObserver
             selector: @selector(serverAdded:)
             name: SyphonServerAnnounceNotification
             object: nil];

            [[NSNotificationCenter defaultCenter]
             addObserver: this->m_serverObserver
             selector: @selector(serverChanged:)
             name: SyphonServerUpdateNotification
             object: nil];

            [[NSNotificationCenter defaultCenter]
             addObserver: this->m_serverObserver
             selector: @selector(serverRemoved:)
             name: SyphonServerRetireNotification
             object: nil];

            element->updateServers();
        }

        ~SyphonIOElementPrivate()
        {
            [[NSNotificationCenter defaultCenter]
             removeObserver: this->m_serverObserver];

            [this->m_serverObserver release];
        }
};

SyphonIOElement::SyphonIOElement(): AkMultimediaSourceElement()
{
    this->d = new SyphonIOElementPrivate(this);
}

SyphonIOElement::~SyphonIOElement()
{
    delete this->d;
}

QObject *SyphonIOElement::controlInterface(QQmlEngine *engine,
                                           const QString &controlId) const
{
    Q_UNUSED(controlId)

    if (!engine)
        return NULL;

    // Load the UI from the plugin.
    QQmlComponent component(engine,
                            QUrl(QStringLiteral("qrc:/SyphonIO/share/qml/main.qml")));

    if (component.isError()) {
        qDebug() << "Error in plugin "
                 << this->metaObject()->className()
                 << ":"
                 << component.errorString();

        return NULL;
    }

    // Create a context for the plugin.
    QQmlContext *context = new QQmlContext(engine->rootContext());
    context->setContextProperty("Syphon", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());

    // Create an item with the plugin context.
    QObject *item = component.create(context);

    if (!item) {
        delete context;

        return NULL;
    }

    context->setParent(item);

    return item;
}

QStringList SyphonIOElement::medias()
{
    this->m_mutex.lock();
    auto medias = this->m_servers.keys();
    this->m_mutex.unlock();

    return medias;
}

QString SyphonIOElement::media() const
{
    return QString();
}

QList<int> SyphonIOElement::streams() const
{
    return QList<int>();
}

bool SyphonIOElement::isOutput() const
{
    return false;
}

int SyphonIOElement::defaultStream(const QString &mimeType)
{
    return -1;
}

QString SyphonIOElement::description(const QString &media)
{
    this->m_mutex.lock();
    auto description = this->m_servers.value(media);
    this->m_mutex.unlock();

    return description;
}

AkCaps SyphonIOElement::caps(int stream)
{
    return AkCaps();
}

void SyphonIOElement::updateServers()
{
    NSArray *servers = [[SyphonServerDirectory sharedDirectory] servers];
    QMap<QString, QString> serversMap;

    for (NSDictionary *server in servers) {
        NSString *serverId =
                [server objectForKey: SyphonServerDescriptionUUIDKey];
        NSString *description =
                [server objectForKey: SyphonServerDescriptionAppNameKey];

        serversMap[QString::fromNSString(serverId)] =
                QString::fromNSString(description);
    }

    this->m_mutex.lock();
    this->m_servers = serversMap;
    this->m_mutex.unlock();

    emit this->mediasChanged(serversMap.keys());
}

void SyphonIOElement::setMedia(const QString &media)
{
}

void SyphonIOElement::setDescription(const QString &description)
{
    if (this->m_description == description)
        return;

    this->m_description = description;
    emit this->descriptionChanged(description);
}

void SyphonIOElement::setAsOutput(bool isOutput)
{

}

void SyphonIOElement::resetMedia()
{

}

void SyphonIOElement::resetDescription()
{
    this->setDescription("");
}

void SyphonIOElement::resetAsInput()
{

}

bool SyphonIOElement::setState(AkElement::ElementState state)
{

}

AkPacket SyphonIOElement::iStream(const AkPacket &packet)
{
    return AkPacket()
}
