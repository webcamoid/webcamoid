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

#include <QJsonDocument>
#include <QJsonArray>
#include <QPoint>
#include <QSize>
#include <QRect>
#include <QLine>
#include <QDate>
#include <QColor>
#include <QBitArray>
#include "pipeline.h"

Pipeline::Pipeline(QObject *parent): QObject(parent)
{
    this->resetElements();
    this->resetLinks();
    this->resetProperties();
    this->resetError();
    this->resetThreads();
}

bool Pipeline::parse(const QString &description)
{
    this->cleanAll();
    QJsonDocument jsonFile = QJsonDocument::fromJson(description.toUtf8());

    if (!jsonFile.isArray()) {
        this->m_error = "Error: This is not a parseable input, "
                        "must be an array of arrays.";

        return false;
    }

    // Parse main array.
    foreach (QJsonValue pipe, jsonFile.array()) {
        if (pipe.isArray()) {
            QJsonArray pipeArray = pipe.toArray();
            QStringList pipeStr;

            // parse a pipe.
            for (int element = 0; element < pipeArray.size(); element++)
                if (pipeArray[element].isObject()) {
                    QJsonObject elementObject = pipeArray[element].toObject();

                    if (elementObject.contains("pluginId")) {
                        if (!elementObject["pluginId"].isString()) {
                            QString error;
                            QDebug debug(&error);

                            debug.nospace() << "Error: 'pluginId' must be a "
                                               "string: "
                                            << elementObject["alias"];

                            this->m_error = error;

                            return false;
                        }

                        QbElementPtr element = QbElement::create(elementObject["pluginId"].toString());

                        if (!element) {
                            this->m_error = QString("Error: Element '%1' doesn't exist.")
                                            .arg(elementObject["pluginId"].toString());

                            return false;
                        }

                        if (elementObject.contains("properties")) {
                            if (!elementObject["properties"].isObject()) {
                                QString error;
                                QDebug debug(&error);

                                debug.nospace() << "Error: 'properties' must "
                                                   "be an object: "
                                                << elementObject["properties"];

                                this->m_error = error;

                                return false;
                            }

                            QVariantMap properties = elementObject["properties"]
                                                     .toObject()
                                                     .toVariantMap();

                            foreach (QString key, properties.keys())
                                element->setProperty(key.toStdString().c_str(),
                                                     this->solveProperty(properties[key]));
                        }

                        QString objectName = this->addElement(element);

                        if (elementObject.contains("connections")) {
                            if (!elementObject["connections"].isArray()) {
                                QString error;
                                QDebug debug(&error);

                                debug.nospace() << "Error: 'connections' must "
                                                   "be an array of arrays: "
                                                << elementObject["connections"];

                                this->m_error = error;

                                return false;
                            }

                            QVariantList connections = elementObject["connections"]
                                                       .toArray()
                                                       .toVariantList();

                            foreach (QVariant connection, connections) {
                                QStringList connectionStr = connection.toStringList();

                                if (connectionStr.size() != 4) {
                                    QString error;
                                    QDebug debug(&error);

                                    debug.nospace() << "Error: A connection must "
                                                       "contains four strings: "
                                                    << connection;

                                    this->m_error = error;

                                    return false;
                                }

                                for (int i = 0; i < connectionStr.size(); i++)
                                    if (connectionStr[i] == "")
                                        connectionStr[i] = objectName;

                                this->m_connections.append(connectionStr);
                            }
                        }

                        pipeStr << objectName;
                    }
                    else if (elementObject.contains("alias")) {
                        if (!elementObject["alias"].isString()) {
                            QString error;
                            QDebug debug(&error);

                            debug.nospace() << "Error: 'alias' must be a "
                                               "string: "
                                            << elementObject["alias"];

                            this->m_error = error;

                            return false;
                        }

                        QString ref = elementObject["alias"].toString();

                        if (ref == "IN") {
                            if (element != 0) {
                                this->m_error = "Error: 'IN' alias must be at "
                                                "the start of a pipe.";

                                return false;
                            }

                            pipeStr << ref + ".";
                        }
                        else if (ref == "OUT") {
                            if (element != pipeArray.size() - 1) {
                                this->m_error = "Error: 'OUT' alias must be "
                                                "at the end of a pipe.";

                                return false;
                            }

                            pipeStr << ref + ".";
                        }
                        else
                            pipeStr << ref;
                    }
                    else {
                        QString error;
                        QDebug debug(&error);

                        debug.nospace() << "Error: Malformed element, "
                                           "must contain 'pluginId' or "
                                           "'alias' key: " << elementObject;

                        this->m_error = error;

                        return false;
                    }
                }
                else if (pipeArray[element].isString()) {
                    QString connectionType = pipeArray[element].toString();

                    if (element == pipeArray.size() - 1) {
                        this->m_error = QString("Error: Connection type to nothing: %1")
                                        .arg(connectionType);

                        return false;
                    }

                    pipeStr << connectionType + "?";
                }
                else {
                    this->m_error = QString("Error: Must be an object or a"
                                            " connection type: %1")
                                    .arg(pipeArray[element].toString());

                    return false;
                }

            this->addLinks(pipeStr);
        }
        else {
            this->m_error = "Error: An pipe must be constructed as an array of objects.";

            return false;
        }
    }

    if (this->linkAll()) {
        if (this->connectAll())
            return true;

        this->m_error = "Error connecting signals and slots.";

        return false;
    }

    this->m_error = "Error linking pipeline.";

    return false;
}

QMap<QString, QbElementPtr> Pipeline::elements() const
{
    return this->m_elements;
}

QList<QStringList> Pipeline::links() const
{
    return this->m_links;
}

QList<QStringList> Pipeline::connections() const
{
    return this->m_connections;
}

QVariantMap Pipeline::properties() const
{
    return this->m_properties;
}

QString Pipeline::error() const
{
    return this->m_error;
}

QString Pipeline::addElement(const QbElementPtr &element)
{
    QString name;

    if (element->objectName().isEmpty())
        name = QString("&%1").arg((quint64) element.data());
    else
        name = element->objectName();

    this->m_elements[name] = element;

    return name;
}

void Pipeline::removeElement(const QString &elementName)
{
    QList<QStringList> connections = this->m_connections;

    foreach (QStringList connection, connections)
        if (connection[0] == elementName
            || connection[2] == elementName) {
            QbElement *sender = this->m_elements[connection[0]].data();
            QbElement *receiver = this->m_elements[connection[2]].data();

            QMetaMethod signal = this->methodByName(sender, connection[1], QMetaMethod::Signal);
            QMetaMethod slot = this->methodByName(receiver, connection[3], QMetaMethod::Slot);

            QObject::disconnect(sender, signal, receiver, slot);
            this->m_connections.removeOne(connection);
        }

    QList<QStringList> links = this->m_links;

    foreach (QStringList link, links)
        if (link[0] == elementName
            || link[1] == elementName) {
            this->m_elements[link[0]]->unlink(this->m_elements[link[1]]);
            this->m_links.removeOne(link);
        }

    this->m_elements.remove(elementName);
}

QList<QbElementPtr> Pipeline::inputs() const
{
    QList<QbElementPtr> inputs;

    foreach (QStringList link, this->m_links)
        if (link[0] == "IN.")
            inputs << this->m_elements[link[1]];

    return inputs;
}

QList<QbElementPtr> Pipeline::outputs() const
{
    QList<QbElementPtr> outputs;

    foreach (QStringList link, this->m_links)
        if (link[1] == "OUT.")
            outputs << this->m_elements[link[0]];

    return outputs;
}

ThreadsMap Pipeline::threads() const
{
    return this->m_threads;
}

QList<Qt::ConnectionType> Pipeline::outputConnectionTypes() const
{
    QList<Qt::ConnectionType> outputoutputConnectionTypes;

    int index = this->staticQtMetaObject.indexOfEnumerator("ConnectionType");
    QMetaEnum enumerator = this->staticQtMetaObject.enumerator(index);

    foreach (QStringList link, this->m_links)
        if (link[1] == "OUT.") {
            QString connectionTypeString;

            if (link.length() > 2)
                connectionTypeString = link[2];
            else
                connectionTypeString = "AutoConnection";

            int value = enumerator.keyToValue(connectionTypeString.toStdString().c_str());

            Qt::ConnectionType connectionType;

            if (value < 0)
                connectionType = Qt::AutoConnection;
            else
                connectionType = static_cast<Qt::ConnectionType>(value);

            outputoutputConnectionTypes << connectionType;
        }

    return outputoutputConnectionTypes;
}

QMetaMethod Pipeline::methodByName(QObject *object, const QString &methodName, QMetaMethod::MethodType methodType)
{
    QMetaMethod rMethod;

    for (int i = 0; i < object->metaObject()->methodCount(); i++) {
        QMetaMethod method = object->metaObject()->method(i);
        QString name(method.name());

        if (method.methodType() == methodType
            && name == methodName) {
            rMethod = method;

            break;
        }
    }

    return rMethod;
}

QVariant Pipeline::solveProperty(const QVariant &property) const
{
    if (property.type() != QVariant::List)
        return property;

    QVariantList propList = property.toList();

    if (propList.size() < 1)
        return QVariant(QVariantList());

    QString type = propList[0].toString();

    if (type == "") {
        QVariantList list;

        for (int i = 1; i < propList.size(); i++)
            list << this->solveProperty(propList[i]);

        return QVariant(list);
    }
    else if (type == "frac") {
        if (propList.size() < 3)
            return QVariant::fromValue(QbFrac());

        return QVariant::fromValue(QbFrac(propList[1].toDouble(),
                                          propList[2].toDouble()));
    }
    else if (type == "size") {
        if (propList.size() < 3)
            return QVariant::fromValue(QSize());

        return QVariant::fromValue(QSize(propList[1].toDouble(),
                                         propList[2].toDouble()));
    }
    else if (type == "sizeF") {
        if (propList.size() < 3)
            return QVariant::fromValue(QSizeF());

        return QVariant::fromValue(QSizeF(propList[1].toDouble(),
                                          propList[2].toDouble()));
    }
    else if (type == "point") {
        if (propList.size() < 3)
            return QVariant::fromValue(QPoint());

        return QVariant::fromValue(QPoint(propList[1].toDouble(),
                                          propList[2].toDouble()));
    }
    else if (type == "pointF") {
        if (propList.size() < 3)
            return QVariant::fromValue(QPointF());

        return QVariant::fromValue(QPointF(propList[1].toDouble(),
                                           propList[2].toDouble()));
    }
    else if (type == "rect") {
        if (propList.size() < 3)
            return QVariant::fromValue(QRect());
        else if (propList.size() == 3) {
            QVariant arg1 = this->solveProperty(propList[1]);

            if (arg1.type() != QVariant::Point)
                return QVariant::fromValue(QRect());

            QVariant arg2 = this->solveProperty(propList[2]);

            if (arg2.type() == QVariant::Point)
                return QVariant::fromValue(QRect(arg1.toPoint(), arg2.toPoint()));
            else if (arg2.type() == QVariant::Size)
                return QVariant::fromValue(QRect(arg1.toPoint(), arg2.toSize()));

            return QVariant::fromValue(QRect());
        }
        else if (propList.size() > 4)
            return QVariant::fromValue(QRect(propList[1].toDouble(),
                                             propList[2].toDouble(),
                                             propList[3].toDouble(),
                                             propList[4].toDouble()));

        return QVariant::fromValue(QRect());
    }
    else if (type == "rectF") {
        if (propList.size() < 3)
            return QVariant::fromValue(QRectF());
        else if (propList.size() == 3) {
            QVariant arg1 = this->solveProperty(propList[1]);

            if (arg1.type() != QVariant::PointF)
                return QVariant::fromValue(QRectF());

            QVariant arg2 = this->solveProperty(propList[2]);

            if (arg2.type() == QVariant::PointF)
                return QVariant::fromValue(QRectF(arg1.toPointF(), arg2.toPointF()));
            else if (arg2.type() == QVariant::SizeF)
                return QVariant::fromValue(QRectF(arg1.toPointF(), arg2.toSizeF()));

            return QVariant::fromValue(QRectF());
        }
        else if (propList.size() > 4)
            return QVariant::fromValue(QRectF(propList[1].toDouble(),
                                              propList[2].toDouble(),
                                              propList[3].toDouble(),
                                              propList[4].toDouble()));

        return QVariant::fromValue(QRectF());
    }
    else if (type == "line") {
        if (propList.size() < 3)
            return QVariant::fromValue(QLine());
        else if (propList.size() == 3) {
            QVariant arg1 = this->solveProperty(propList[1]);

            if (arg1.type() != QVariant::Point)
                return QVariant::fromValue(QLine());

            QVariant arg2 = this->solveProperty(propList[2]);

            if (arg2.type() == QVariant::Point)
                return QVariant::fromValue(QLine(arg1.toPoint(), arg2.toPoint()));

            return QVariant::fromValue(QLine());
        }
        else if (propList.size() > 4)
            return QVariant::fromValue(QLine(propList[1].toDouble(),
                                             propList[2].toDouble(),
                                             propList[3].toDouble(),
                                             propList[4].toDouble()));

        return QVariant::fromValue(QLine());
    }
    else if (type == "lineF") {
        if (propList.size() < 3)
            return QVariant::fromValue(QLineF());
        else if (propList.size() == 3) {
            QVariant arg1 = this->solveProperty(propList[1]);

            if (arg1.type() != QVariant::PointF)
                return QVariant::fromValue(QLineF());

            QVariant arg2 = this->solveProperty(propList[2]);

            if (arg2.type() == QVariant::PointF)
                return QVariant::fromValue(QLineF(arg1.toPointF(), arg2.toPointF()));

            return QVariant::fromValue(QLineF());
        }
        else if (propList.size() > 4)
            return QVariant::fromValue(QLineF(propList[1].toDouble(),
                                              propList[2].toDouble(),
                                              propList[3].toDouble(),
                                              propList[4].toDouble()));

        return QVariant::fromValue(QLineF());
    }
    else if (type == "date") {
        if (propList.size() < 4)
            return QVariant::fromValue(QDate());

        return QVariant::fromValue(QDate(propList[1].toDouble(),
                                         propList[2].toDouble(),
                                         propList[3].toDouble()));
    }
    else if (type == "time") {
        if (propList.size() < 3)
            return QVariant::fromValue(QTime());
        else if (propList.size() == 3)
            return QVariant::fromValue(QTime(propList[1].toDouble(),
                                             propList[2].toDouble()));
        else if (propList.size() == 4)
            return QVariant::fromValue(QTime(propList[1].toDouble(),
                                             propList[2].toDouble(),
                                             propList[3].toDouble()));
        else if (propList.size() > 4)
            return QVariant::fromValue(QTime(propList[1].toDouble(),
                                             propList[2].toDouble(),
                                             propList[3].toDouble(),
                                             propList[4].toDouble()));

        return QVariant::fromValue(QTime());
    }
    else if (type == "dateTime") {
        if (propList.size() < 2)
            return QVariant::fromValue(QDateTime());

        QVariant arg1 = this->solveProperty(propList[1]);

        if (propList.size() == 2)
            return QVariant::fromValue(QDateTime(arg1.toDate()));

        QVariant arg2 = this->solveProperty(propList[2]);

        return QVariant::fromValue(QDateTime(arg1.toDate(),
                                             arg2.toTime()));
    }
    else if (type == "rgb") {
        if (propList.size() < 2)
            return QVariant::fromValue(qRgba(0, 0, 0, 0));
        else if (propList.size() == 2)
            return QVariant::fromValue(QColor(propList[1].toString()).rgba());
        else if (propList.size() == 4)
            return QVariant::fromValue(qRgb(propList[1].toDouble(),
                                            propList[2].toDouble(),
                                            propList[3].toDouble()));
        else if (propList.size() > 4)
            return QVariant::fromValue(qRgba(propList[1].toDouble(),
                                             propList[2].toDouble(),
                                             propList[3].toDouble(),
                                             propList[4].toDouble()));

        return QVariant::fromValue(qRgba(0, 0, 0, 0));
    }
    else if (type == "color") {
        if (propList.size() < 2)
            return QVariant::fromValue(QColor());
        else if (propList.size() == 2)
            return QVariant::fromValue(QColor(propList[1].toString()));
        else if (propList.size() == 4)
            return QVariant::fromValue(QColor(propList[1].toDouble(),
                                              propList[2].toDouble(),
                                              propList[3].toDouble()));
        else if (propList.size() > 4)
            return QVariant::fromValue(QColor(propList[1].toDouble(),
                                              propList[2].toDouble(),
                                              propList[3].toDouble(),
                                              propList[4].toDouble()));

        return QVariant::fromValue(QColor());
    }
    else if (type == "bits") {
        if (propList.size() < 2)
            return QVariant::fromValue(QBitArray());

        QString bitsString = propList[1].toString();
        QBitArray bits;

        bitsString.replace(QRegExp("\\s+"), "");

        if (bitsString.length() > 0)
        {
            bits.resize(bitsString.length());

            for (int i = 0; i < bitsString.length(); i++)
                bits.setBit(i, (bitsString[i] == '0')? false: true);
        }

        return QVariant::fromValue(bits);
    }
    else if (type == "bytes")
        return QVariant::fromValue(propList[1].toByteArray());
    else if (type == "url")
        return QVariant::fromValue(propList[1].toUrl());

    return QVariant(QVariantList());
}

void Pipeline::addLinks(const QStringList &links)
{
    QStringList link;
    QString connectionType = "AutoConnection";

    foreach (QString element,  links) {
        if (element.endsWith("?"))
            connectionType = element.remove("?");
        else
            link << element;

        if (link.length() == 2) {
            link << connectionType;
            this->m_links << link;
            link.removeFirst();
            link.removeLast();
        }
    }
}

bool Pipeline::linkAll()
{
    foreach (QStringList link, this->m_links)
        if (link[0] != "IN."
            && link[1] != "OUT.") {
            if (!this->m_elements.contains(link[0])) {
                this->m_error = QString("No element named '%1'").arg(link[0]);

                return false;
            } else if (!this->m_elements.contains(link[1])) {
                this->m_error = QString("No element named '%1'").arg(link[1]);

                return false;
            } else {
                QString connectionTypeString;

                if (link.length() > 2)
                    connectionTypeString = link[2];
                else
                    connectionTypeString = "AutoConnection";

                int index = this->staticQtMetaObject.indexOfEnumerator("ConnectionType");
                QMetaEnum enumerator = this->staticQtMetaObject.enumerator(index);

                int value = enumerator.keyToValue(connectionTypeString.toStdString().c_str());

                if (value < 0)
                {
                    this->m_error = QString("Invalid connection type: '%1'").arg(connectionTypeString);

                    return false;
                }

                Qt::ConnectionType connectionType = static_cast<Qt::ConnectionType>(value);

                this->m_elements[link[0]]->link(this->m_elements[link[1]], connectionType);
            }
        }

    return true;
}

bool Pipeline::unlinkAll()
{
    foreach (QStringList link, this->m_links)
        if (link[0] != "IN."
            && link[1] != "OUT.") {
            if (!this->m_elements.contains(link[0])) {
                this->m_error = QString("No element named '%1'").arg(link[0]);

                return false;
            } else if (!this->m_elements.contains(link[1])) {
                this->m_error = QString("No element named '%1'").arg(link[1]);

                return false;
            } else
                this->m_elements[link[0]]->unlink(this->m_elements[link[1]]);
        }

    return true;
}

bool Pipeline::connectAll()
{
    foreach (QStringList connection, this->m_connections) {
        QbElement *sender = this->m_elements[connection[0]].data();
        QbElement *receiver = this->m_elements[connection[2]].data();

        if (!sender) {
            this->m_error = QString("No element named '%1'").arg(connection[0]);

            return false;
        }

        if (!receiver) {
            this->m_error = QString("No element named '%1'").arg(connection[2]);

            return false;
        }

        QMetaMethod signal = this->methodByName(sender, connection[1], QMetaMethod::Signal);
        QMetaMethod slot = this->methodByName(receiver, connection[3], QMetaMethod::Slot);

        QObject::connect(sender, signal, receiver, slot);
    }

    return true;
}

bool Pipeline::disconnectAll()
{
    foreach (QStringList connection, this->m_connections) {
        QbElement *sender = this->m_elements[connection[0]].data();
        QbElement *receiver = this->m_elements[connection[2]].data();

        if (!sender) {
            this->m_error = QString("No element named '%1'.").arg(connection[0]);

            return false;
        }

        if (!receiver) {
            this->m_error = QString("No element named '%1'.").arg(connection[2]);

            return false;
        }

        QMetaMethod signal = this->methodByName(sender, connection[1], QMetaMethod::Signal);
        QMetaMethod slot = this->methodByName(receiver, connection[3], QMetaMethod::Slot);

        QObject::disconnect(sender, signal, receiver, slot);
    }

    return true;
}

void Pipeline::cleanAll()
{
    this->unlinkAll();
    this->disconnectAll();
    this->resetElements();
    this->resetLinks();
    this->m_connections.clear();
    this->resetProperties();
    this->resetError();
}

void Pipeline::setElements(const QMap<QString, QbElementPtr> &elements)
{
    this->m_elements = elements;
}

void Pipeline::setLinks(const QList<QStringList> &links)
{
    this->m_links = links;
}

void Pipeline::setProperties(const QVariantMap &properties)
{
    this->m_properties = properties;
}

void Pipeline::setError(const QString &error)
{
    this->m_error = error;
}

void Pipeline::setThreads(const ThreadsMap &threads)
{
    this->m_threads = threads;
}

void Pipeline::resetElements()
{
    this->setElements(QMap<QString, QbElementPtr>());
}

void Pipeline::resetLinks()
{
    this->setLinks(QList<QStringList>());
}

void Pipeline::resetProperties()
{
    this->setProperties(QVariantMap());
}

void Pipeline::resetError()
{
    this->setError("");
}

void Pipeline::resetThreads()
{
    this->m_threads.clear();
}
