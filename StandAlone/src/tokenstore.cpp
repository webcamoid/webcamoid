/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QtGlobal>

#include "tokenstore.h"

#define TOKENSTORE_SERVICE "Webcamoid"

#if defined(Q_OS_WIN)

#include <cstring>
#include <string>
#include <windows.h>
#include <wincred.h>

namespace {
    std::wstring targetName(const QString &name)
    {
        return QString("%1/%2")
                .arg(TOKENSTORE_SERVICE, name)
                .toStdWString();
    }
}

TokenStore::Result TokenStore::read(const QString &name, QByteArray *data)
{
    auto target = targetName(name);
    PCREDENTIALW cred = nullptr;

    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &cred))
        return GetLastError() == ERROR_NOT_FOUND? NotFound: Unavailable;

    if (data)
        *data = QByteArray(reinterpret_cast<const char *>(cred->CredentialBlob),
                           int(cred->CredentialBlobSize));

    CredFree(cred);

    return Ok;
}

TokenStore::Result TokenStore::write(const QString &name,
                                     const QByteArray &data)
{
    if (data.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE)
        return Unavailable;

    auto target = targetName(name);
    CREDENTIALW cred;
    memset(&cred, 0, sizeof(CREDENTIALW));
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = const_cast<LPWSTR>(target.c_str());
    cred.CredentialBlobSize = DWORD(data.size());
    cred.CredentialBlob =
            reinterpret_cast<LPBYTE>(const_cast<char *>(data.constData()));
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

    return CredWriteW(&cred, 0)? Ok: Unavailable;
}

TokenStore::Result TokenStore::remove(const QString &name)
{
    auto target = targetName(name);

    if (CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0))
        return Ok;

    return GetLastError() == ERROR_NOT_FOUND? NotFound: Unavailable;
}

#elif defined(Q_OS_MACOS)

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

namespace {
    // Releases a Core Foundation object when it goes out of scope.
    template<typename T>
    class CfRef
    {
        public:
            explicit CfRef(T ref=nullptr): m_ref(ref) {}
            CfRef(const CfRef &) = delete;
            CfRef &operator =(const CfRef &) = delete;

            CfRef(CfRef &&other) noexcept:
                m_ref(other.m_ref)
            {
                other.m_ref = nullptr;
            }

            CfRef &operator =(CfRef &&other) noexcept
            {
                if (this != &other) {
                    if (this->m_ref)
                        CFRelease(this->m_ref);

                    this->m_ref = other.m_ref;
                    other.m_ref = nullptr;
                }

                return *this;
            }

            ~CfRef()
            {
                if (this->m_ref)
                    CFRelease(this->m_ref);
            }

            T get() const { return this->m_ref; }

        private:
            T m_ref;
    };

    CfRef<CFStringRef> cfString(const QString &str)
    {
        auto utf8 = str.toUtf8();

        return CfRef<CFStringRef>(CFStringCreateWithBytes(nullptr,
                                                          reinterpret_cast<const UInt8 *>(utf8.constData()),
                                                          utf8.size(),
                                                          kCFStringEncodingUTF8,
                                                          false));
    }

    CfRef<CFMutableDictionaryRef> baseQuery(const QString &name)
    {
        CfRef<CFMutableDictionaryRef> query(CFDictionaryCreateMutable(nullptr,
                                                                      0,
                                                                      &kCFTypeDictionaryKeyCallBacks,
                                                                      &kCFTypeDictionaryValueCallBacks));
        auto service = cfString(TOKENSTORE_SERVICE);
        auto account = cfString(name);
        CFDictionarySetValue(query.get(), kSecClass, kSecClassGenericPassword);
        CFDictionarySetValue(query.get(), kSecAttrService, service.get());
        CFDictionarySetValue(query.get(), kSecAttrAccount, account.get());

        return query;
    }

    CfRef<CFDataRef> cfData(const QByteArray &data)
    {
        return CfRef<CFDataRef>(CFDataCreate(nullptr,
                                             reinterpret_cast<const UInt8 *>(data.constData()),
                                             data.size()));
    }

    // The access of the keyring items is bound to the identity of the
    // application. A binary that is not signed, or is signed ad hoc, has a
    // different identity in every build, so macOS would ask the user for
    // permission again after each update. Only use the keyring when the binary
    // is signed with a certificate.
    bool hasStableSignature()
    {
        static const bool stable = [] () {
            SecCodeRef self = nullptr;

            if (SecCodeCopySelf(kSecCSDefaultFlags, &self) != errSecSuccess
                || !self)
                return false;

            CfRef<SecCodeRef> selfRef(self);
            SecStaticCodeRef staticCode = nullptr;

            if (SecCodeCopyStaticCode(self,
                                      kSecCSDefaultFlags,
                                      &staticCode) != errSecSuccess
                || !staticCode)
                return false;

            CfRef<SecStaticCodeRef> staticCodeRef(staticCode);
            CFDictionaryRef info = nullptr;

            // It fails if the binary is not signed.
            if (SecCodeCopySigningInformation(staticCode,
                                              kSecCSSigningInformation,
                                              &info) != errSecSuccess
                || !info)
                return false;

            CfRef<CFDictionaryRef> infoRef(info);

            // An ad hoc signature has no certificates.
            auto certificates =
                    static_cast<CFArrayRef>(CFDictionaryGetValue(info,
                                                                 kSecCodeInfoCertificates));

            return certificates && CFArrayGetCount(certificates) > 0;
        }();

        return stable;
    }
}

TokenStore::Result TokenStore::read(const QString &name, QByteArray *data)
{
    if (!hasStableSignature())
        return Unavailable;

    auto query = baseQuery(name);
    CFDictionarySetValue(query.get(), kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query.get(), kSecMatchLimit, kSecMatchLimitOne);
    CFTypeRef found = nullptr;
    auto status = SecItemCopyMatching(query.get(), &found);

    if (status == errSecItemNotFound)
        return NotFound;

    if (status != errSecSuccess || !found)
        return Unavailable;

    CfRef<CFTypeRef> result(found);

    if (CFGetTypeID(found) != CFDataGetTypeID())
        return Unavailable;

    auto bytes = static_cast<CFDataRef>(found);

    if (data)
        *data = QByteArray(reinterpret_cast<const char *>(CFDataGetBytePtr(bytes)),
                           int(CFDataGetLength(bytes)));

    return Ok;
}

TokenStore::Result TokenStore::write(const QString &name,
                                     const QByteArray &data)
{
    if (!hasStableSignature())
        return Unavailable;

    auto value = cfData(data);
    auto query = baseQuery(name);
    CfRef<CFMutableDictionaryRef> attributes(CFDictionaryCreateMutable(nullptr,
                                                                       0,
                                                                       &kCFTypeDictionaryKeyCallBacks,
                                                                       &kCFTypeDictionaryValueCallBacks));
    CFDictionarySetValue(attributes.get(), kSecValueData, value.get());
    auto status = SecItemUpdate(query.get(), attributes.get());

    if (status == errSecItemNotFound) {
        CFDictionarySetValue(query.get(), kSecValueData, value.get());
        status = SecItemAdd(query.get(), nullptr);
    }

    return status == errSecSuccess? Ok: Unavailable;
}

TokenStore::Result TokenStore::remove(const QString &name)
{
    if (!hasStableSignature())
        return Unavailable;

    auto query = baseQuery(name);
    auto status = SecItemDelete(query.get());

    if (status == errSecSuccess)
        return Ok;

    return status == errSecItemNotFound? NotFound: Unavailable;
}

#elif defined(HAVE_QTDBUS)

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QEventLoop>
#include <QList>
#include <QMap>
#include <QObject>
#include <QTimer>
#include <QVariant>

#define SS_SERVICE    "org.freedesktop.secrets"
#define SS_PATH       "/org/freedesktop/secrets"
#define SS_SERVICE_IF "org.freedesktop.Secret.Service"
#define SS_COLLECT_IF "org.freedesktop.Secret.Collection"
#define SS_ITEM_IF    "org.freedesktop.Secret.Item"
#define SS_PROMPT_IF  "org.freedesktop.Secret.Prompt"
#define SS_SESSION_IF "org.freedesktop.Secret.Session"
#define SS_PROPS_IF   "org.freedesktop.DBus.Properties"
#define SS_ITEM_LABEL "org.freedesktop.Secret.Item.Label"
#define SS_ITEM_ATTRS "org.freedesktop.Secret.Item.Attributes"

#define SS_CALL_TIMEOUT_MS   15000
#define SS_PROMPT_TIMEOUT_MS 120000

using SecretServiceAttributes = QMap<QString, QString>;

// The "Secret" structure of the Secret Service API: (oayays).
struct SecretServiceSecret
{
    QDBusObjectPath session;
    QByteArray parameters;
    QByteArray value;
    QString contentType;
};

Q_DECLARE_METATYPE(SecretServiceSecret)

QDBusArgument &operator <<(QDBusArgument &argument,
                           const SecretServiceSecret &secret)
{
    argument.beginStructure();
    argument << secret.session
             << secret.parameters
             << secret.value
             << secret.contentType;
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator >>(const QDBusArgument &argument,
                                 SecretServiceSecret &secret)
{
    argument.beginStructure();
    argument >> secret.session
             >> secret.parameters
             >> secret.value
             >> secret.contentType;
    argument.endStructure();

    return argument;
}

namespace {
    bool isNullPath(const QDBusObjectPath &path)
    {
        return path.path().isEmpty() || path.path() == "/";
    }

    // Receives the result of a prompt shown by the keyring.
    class PromptWaiter: public QObject
    {
        Q_OBJECT

        public:
            bool dismissed {true};

        signals:
            void finished();

        public slots:
            void completed(bool dismissed, const QDBusVariant &result)
            {
                Q_UNUSED(result)

                this->dismissed = dismissed;
                emit this->finished();
            }
    };

    class SecretService
    {
        public:
            SecretService():
                m_bus(QDBusConnection::sessionBus())
            {
                qDBusRegisterMetaType<SecretServiceAttributes>();
                qDBusRegisterMetaType<SecretServiceSecret>();
            }

            ~SecretService()
            {
                if (!this->m_session.isEmpty()) {
                    QDBusInterface session(SS_SERVICE,
                                           this->m_session,
                                           SS_SESSION_IF,
                                           this->m_bus);
                    session.setTimeout(SS_CALL_TIMEOUT_MS);
                    session.call("Close");
                }
            }

            // Opens the session and unlocks the default collection.
            bool open()
            {
                if (!this->m_bus.isConnected())
                    return false;

                QDBusInterface service(SS_SERVICE,
                                       SS_PATH,
                                       SS_SERVICE_IF,
                                       this->m_bus);

                if (!service.isValid())
                    return false;

                service.setTimeout(SS_CALL_TIMEOUT_MS);

                // The secrets are sent without encryption over the session
                // bus, which is private to the user.
                auto reply =
                        service.call("OpenSession",
                                     "plain",
                                     QVariant::fromValue(QDBusVariant(QString())));

                if (reply.type() != QDBusMessage::ReplyMessage
                    || reply.arguments().size() < 2)
                    return false;

                this->m_session =
                        reply.arguments().at(1).value<QDBusObjectPath>().path();

                reply = service.call("ReadAlias", "default");

                if (reply.type() != QDBusMessage::ReplyMessage
                    || reply.arguments().isEmpty())
                    return false;

                auto collection =
                        reply.arguments().first().value<QDBusObjectPath>();

                if (isNullPath(collection))
                    return false;

                this->m_collection = collection.path();

                if (!this->locked())
                    return true;

                reply = service.call("Unlock",
                                     QVariant::fromValue(QList<QDBusObjectPath> {collection}));

                if (reply.type() != QDBusMessage::ReplyMessage
                    || reply.arguments().size() < 2)
                    return false;

                auto prompt = reply.arguments().at(1).value<QDBusObjectPath>();

                if (!isNullPath(prompt) && !this->runPrompt(prompt.path()))
                    return false;

                return !this->locked();
            }

            QList<QDBusObjectPath> search(const QString &name)
            {
                QDBusInterface collection(SS_SERVICE,
                                          this->m_collection,
                                          SS_COLLECT_IF,
                                          this->m_bus);
                collection.setTimeout(SS_CALL_TIMEOUT_MS);
                auto reply =
                        collection.call("SearchItems",
                                        QVariant::fromValue(attributes(name)));

                if (reply.type() != QDBusMessage::ReplyMessage
                    || reply.arguments().isEmpty())
                    return {};

                return qdbus_cast<QList<QDBusObjectPath>>(reply.arguments().first());
            }

            bool getSecret(const QString &itemPath, QByteArray *data)
            {
                QDBusInterface item(SS_SERVICE,
                                    itemPath,
                                    SS_ITEM_IF,
                                    this->m_bus);
                item.setTimeout(SS_CALL_TIMEOUT_MS);
                auto reply =
                        item.call("GetSecret",
                                  QVariant::fromValue(QDBusObjectPath(this->m_session)));

                if (reply.type() != QDBusMessage::ReplyMessage
                    || reply.arguments().isEmpty())
                    return false;

                auto secret =
                        qdbus_cast<SecretServiceSecret>(reply.arguments().first());

                if (data)
                    *data = secret.value;

                return true;
            }

            bool createItem(const QString &name, const QByteArray &data)
            {
                QDBusInterface collection(SS_SERVICE,
                                          this->m_collection,
                                          SS_COLLECT_IF,
                                          this->m_bus);
                collection.setTimeout(SS_CALL_TIMEOUT_MS);

                QVariantMap properties {
                    {SS_ITEM_LABEL,
                     QString("%1: %2").arg(TOKENSTORE_SERVICE, name)},
                    {SS_ITEM_ATTRS,
                     QVariant::fromValue(attributes(name))},
                };
                SecretServiceSecret secret {
                    QDBusObjectPath(this->m_session),
                    {},
                    data,
                    "text/plain"
                };

                // replace = true: an item with the same attributes is
                // replaced.
                auto reply =
                        collection.call("CreateItem",
                                        properties,
                                        QVariant::fromValue(secret),
                                        true);

                if (reply.type() != QDBusMessage::ReplyMessage
                    || reply.arguments().size() < 2)
                    return false;

                auto prompt = reply.arguments().at(1).value<QDBusObjectPath>();

                return isNullPath(prompt) || this->runPrompt(prompt.path());
            }

            bool deleteItem(const QString &itemPath)
            {
                QDBusInterface item(SS_SERVICE,
                                    itemPath,
                                    SS_ITEM_IF,
                                    this->m_bus);
                item.setTimeout(SS_CALL_TIMEOUT_MS);
                auto reply = item.call("Delete");

                if (reply.type() != QDBusMessage::ReplyMessage
                    || reply.arguments().isEmpty())
                    return false;

                auto prompt = reply.arguments().first().value<QDBusObjectPath>();

                return isNullPath(prompt) || this->runPrompt(prompt.path());
            }

        private:
            QDBusConnection m_bus;
            QString m_session;
            QString m_collection;

            static SecretServiceAttributes attributes(const QString &name)
            {
                return {
                    {"service", TOKENSTORE_SERVICE},
                    {"account", name},
                };
            }

            bool locked() const
            {
                QDBusInterface properties(SS_SERVICE,
                                          this->m_collection,
                                          SS_PROPS_IF,
                                          this->m_bus);
                properties.setTimeout(SS_CALL_TIMEOUT_MS);
                auto reply = properties.call("Get", SS_COLLECT_IF, "Locked");

                if (reply.type() != QDBusMessage::ReplyMessage
                    || reply.arguments().isEmpty())
                    return true;

                return reply.arguments()
                            .first()
                            .value<QDBusVariant>()
                            .variant()
                            .toBool();
            }

            // Shows the prompt of the keyring and waits for the user.
            bool runPrompt(const QString &path)
            {
                PromptWaiter waiter;
                QEventLoop loop;
                QTimer timer;
                timer.setSingleShot(true);
                QObject::connect(&waiter,
                                 &PromptWaiter::finished,
                                 &loop,
                                 &QEventLoop::quit);
                QObject::connect(&timer,
                                 &QTimer::timeout,
                                 &loop,
                                 &QEventLoop::quit);

                // Connect before calling Prompt to not miss the signal.
                if (!this->m_bus.connect(SS_SERVICE,
                                         path,
                                         SS_PROMPT_IF,
                                         "Completed",
                                         &waiter,
                                         SLOT(completed(bool,QDBusVariant))))
                    return false;

                QDBusInterface prompt(SS_SERVICE,
                                      path,
                                      SS_PROMPT_IF,
                                      this->m_bus);
                prompt.setTimeout(SS_CALL_TIMEOUT_MS);
                auto reply = prompt.call("Prompt", QString());
                bool ok = false;

                if (reply.type() == QDBusMessage::ReplyMessage) {
                    timer.start(SS_PROMPT_TIMEOUT_MS);
                    loop.exec();
                    ok = timer.isActive() && !waiter.dismissed;
                }

                this->m_bus.disconnect(SS_SERVICE,
                                       path,
                                       SS_PROMPT_IF,
                                       "Completed",
                                       &waiter,
                                       SLOT(completed(bool,QDBusVariant)));

                return ok;
            }
    };
}

TokenStore::Result TokenStore::read(const QString &name, QByteArray *data)
{
    SecretService service;

    if (!service.open())
        return Unavailable;

    auto items = service.search(name);

    if (items.isEmpty())
        return NotFound;

    return service.getSecret(items.first().path(), data)? Ok: Unavailable;
}

TokenStore::Result TokenStore::write(const QString &name,
                                     const QByteArray &data)
{
    SecretService service;

    if (!service.open())
        return Unavailable;

    return service.createItem(name, data)? Ok: Unavailable;
}

TokenStore::Result TokenStore::remove(const QString &name)
{
    SecretService service;

    if (!service.open())
        return Unavailable;

    auto items = service.search(name);

    if (items.isEmpty())
        return NotFound;

    for (auto &item: items)
        if (!service.deleteItem(item.path()))
            return Unavailable;

    return Ok;
}

#include "tokenstore.moc"

#else

TokenStore::Result TokenStore::read(const QString &name, QByteArray *data)
{
    Q_UNUSED(name)
    Q_UNUSED(data)

    return Unavailable;
}

TokenStore::Result TokenStore::write(const QString &name,
                                     const QByteArray &data)
{
    Q_UNUSED(name)
    Q_UNUSED(data)

    return Unavailable;
}

TokenStore::Result TokenStore::remove(const QString &name)
{
    Q_UNUSED(name)

    return Unavailable;
}

#endif
