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

#include <QTime>
#include <QOpenGLContext>
#include <akutils.h>
#import <Syphon.h>

#include "syphonioelement.h"
#include "renderwidget.h"
#import "serverobserver.h"

class SyphonIOElementPrivate
{
    public:
        id m_serverObserver;
        SyphonIOElement *m_element;
        SyphonClient *m_syphonClient;
        SyphonServer *m_syphonServer;
        CGLContextObj m_oglContext;
        RenderWidget m_ogl;

        explicit SyphonIOElementPrivate(SyphonIOElement *element)
        {
            this->m_syphonClient = nil;
            this->m_syphonServer = nil;

            auto curContext = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
            this->m_ogl.makeCurrent();
            this->m_ogl.show();
            this->m_oglContext = CGLGetCurrentContext();
            this->m_ogl.hide();

            if (curContext)
                curContext->makeCurrent(NULL);

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
            this->m_element = element;
        }

        ~SyphonIOElementPrivate()
        {
            [[NSNotificationCenter defaultCenter]
             removeObserver: this->m_serverObserver];

            [this->m_serverObserver release];

            if (this->m_syphonServer) {
                [this->m_syphonServer stop];
                [this->m_syphonServer release];
            }
        }

        NSDictionary *descriptionFromMedia(const QString &media)
        {
            NSArray *servers = [[SyphonServerDirectory sharedDirectory] servers];

            for (NSDictionary *server in servers) {
                NSString *serverId =
                        [server objectForKey: SyphonServerDescriptionUUIDKey];

                if (QString::fromNSString(serverId) == media)
                    return server;
            }

            return nil;
        }

        void frameReady(SyphonClient *client) {
            auto frame = [client newFrameImageForContext: this->m_oglContext];

            if (frame) {
                GLuint texture = frame.textureName;
                NSSize dimensions = frame.textureSize;

                this->m_ogl.resize(int(dimensions.width),
                                   int(dimensions.height));
                this->m_ogl.setTexture(texture);
                this->m_element->frameReady(this->m_ogl.grabFrame());

                [frame release];
            }
        }
};

SyphonIOElement::SyphonIOElement(): AkMultimediaSourceElement()
{
    this->m_isOutput = false;
    this->m_fps = AkFrac(30, 1);
    this->d = new SyphonIOElementPrivate(this);

    QObject::connect(this,
                     &SyphonIOElement::isOutputChanged,
                     this,
                     &SyphonIOElement::isServerChanged);
}

SyphonIOElement::~SyphonIOElement()
{
    this->setState(AkElement::ElementStateNull);
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
    if (this->m_isOutput) {
        if (!this->d->m_syphonServer)
            return QString();

        NSString *serverId =
                [this->d->m_syphonServer.serverDescription
                 objectForKey: SyphonServerDescriptionUUIDKey];

        return QString::fromNSString(serverId);
    }

    return this->m_media;
}

QList<int> SyphonIOElement::streams() const
{
    return QList<int>();
}

bool SyphonIOElement::isOutput() const
{
    return this->m_isOutput;
}

int SyphonIOElement::defaultStream(const QString &mimeType)
{
    return mimeType == "video/x-raw"? 0: -1;
}

QString SyphonIOElement::description(const QString &media)
{
    if (this->m_isOutput)
        return this->m_description;

    this->m_mutex.lock();
    auto description = this->m_servers.value(media);
    this->m_mutex.unlock();

    return description;
}

AkCaps SyphonIOElement::caps(int stream)
{
    if (stream != 0)
        return AkCaps();

    return this->m_caps;
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

void SyphonIOElement::frameReady(const QImage &frame)
{
    AkVideoCaps caps;
    caps.isValid() = true;
    caps.format() = AkVideoCaps::Format_rgb24;
    caps.bpp() = AkVideoCaps::bitsPerPixel(caps.format());
    caps.width() = frame.width();
    caps.height() = frame.height();
    caps.fps() = this->m_fps;

    AkPacket packet = AkUtils::imageToPacket(frame.convertToFormat(QImage::Format_RGB888),
                                             caps.toCaps());

    if (!packet)
        return;

    qint64 pts = qint64(QTime::currentTime().msecsSinceStartOfDay()
                        * caps.fps().value() / 1e3);

    packet.setPts(pts);
    packet.setTimeBase(caps.fps().invert());
    packet.setIndex(0);
    packet.setId(this->m_id);
    this->m_caps = packet.caps();

    emit this->oStream(packet);
}

void SyphonIOElement::setMedia(const QString &media)
{
    if (this->m_media == media)
        return;

    this->m_media = media;
    emit this->mediaChanged(media);
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
    if (this->m_isOutput == isOutput)
        return;

    this->m_isOutput = isOutput;
    emit this->isOutputChanged(isOutput);
}

void SyphonIOElement::resetMedia()
{
    this->setMedia("");
}

void SyphonIOElement::resetDescription()
{
    this->setDescription("");
}

void SyphonIOElement::resetAsInput()
{
    this->setAsOutput(false);
}

bool SyphonIOElement::setState(AkElement::ElementState state)
{
    AkElement::ElementState curState = this->state();

    switch (curState) {
    case AkElement::ElementStateNull: {
        switch (state) {
        case AkElement::ElementStatePaused:
            this->m_id = Ak::id();

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            // Start capture/serve
            if (this->m_isOutput) {

            } else {
                this->m_id = Ak::id();
                auto description = this->d->descriptionFromMedia(this->m_media);

                if (!description)
                    return false;

                this->d->m_syphonClient =
                        [[SyphonClient alloc]
                         initWithServerDescription: description
                         options: nil
                         newFrameHandler: ^(SyphonClient *client) {
                            this->d->frameReady(client);
                         }];
            }

            return AkElement::setState(state);
        case AkElement::ElementStateNull:
            break;
        }

        break;
    }
    case AkElement::ElementStatePaused: {
        switch (state) {
        case AkElement::ElementStateNull:
            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            // Start capture/serve
            if (this->m_isOutput) {

            } else {
                auto description = this->d->descriptionFromMedia(this->m_media);

                if (!description)
                    return false;

                this->d->m_syphonClient =
                        [[SyphonClient alloc]
                         initWithServerDescription: description
                         options: nil
                         newFrameHandler: ^(SyphonClient *client) {
                            this->d->frameReady(client);
                         }];
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePaused:
            break;
        }

        break;
    }
    case AkElement::ElementStatePlaying: {
        switch (state) {
        case AkElement::ElementStateNull:
        case AkElement::ElementStatePaused:
            if (this->d->m_syphonClient) {
                [this->d->m_syphonClient stop];
                [this->d->m_syphonClient release];
                this->d->m_syphonClient = nil;
            }

            return AkElement::setState(state);
        case AkElement::ElementStatePlaying:
            break;
        }

        break;
    }
    }

    return false;
}

AkPacket SyphonIOElement::iStream(const AkPacket &packet)
{
    if (!this->d->m_syphonServer
        || this->d->m_syphonServer.hasClients == NO
        || this->state() != AkElement::ElementStatePlaying)
        return AkPacket();

    auto frame = AkUtils::packetToImage(packet)
                    .convertToFormat(QImage::Format_ARGB32);

    this->d->m_ogl.makeCurrent();
    glEnable(TEXTURE_TARGET);
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(TEXTURE_TARGET, texture);
    glTexParameteri(TEXTURE_TARGET, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(TEXTURE_TARGET, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(TEXTURE_TARGET, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(TEXTURE_TARGET, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(TEXTURE_TARGET,
                 0,
                 GL_RGBA,
                 frame.width(),
                 frame.height(),
                 0,
                 GL_BGRA,
                 GL_UNSIGNED_BYTE,
                 frame.constBits());

    [this->d->m_syphonServer
     publishFrameTexture: texture
     textureTarget: TEXTURE_TARGET
     imageRegion: NSMakeRect(0, 0, frame.width(), frame.height())
     textureDimensions: NSMakeSize(frame.width(), frame.height())
     flipped: YES];

    glBindTexture(TEXTURE_TARGET, 0);
    glDeleteTextures(1, &texture);
    glDisable(TEXTURE_TARGET);

    return packet;
}

void SyphonIOElement::isServerChanged(bool isOutput)
{
    this->setState(AkElement::ElementStateNull);

    if (isOutput) {
        this->d->m_syphonServer
                = [[SyphonServer alloc]
                   initWithName: this->m_description.toNSString()
                   context: this->d->m_oglContext
                   options: nil];
    } else if (this->d->m_syphonServer) {
        [this->d->m_syphonServer stop];
        [this->d->m_syphonServer release];
        this->d->m_syphonServer = nil;
    }
}
