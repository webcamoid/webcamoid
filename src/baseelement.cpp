#include "baseelement.h"

BaseElement::BaseElement(QObject *parent): QObject(parent)
{
    this->resetPluginsPaths();
}

QStringList BaseElement::pluginsPaths()
{
    return this->m_pluginsPaths;
}

Element *BaseElement::createElement(QString pluginId)
{
    if (!this->load(pluginId))
        return NULL;

    Element *element = this->m_plugins[pluginId]->newElement();

    if (element)
        this->m_elements[element] = pluginId;

    return element;
}

void BaseElement::destroyElement(Element *element)
{
    if (!this->m_elements.contains(element))
        return;

    QString pluginId = this->m_elements[element];
    delete element;
    this->m_elements.remove(element);

    if (!this->m_elements.values().contains(pluginId))
        this->unload(pluginId);
}

void BaseElement::connect(Element *elementSrc, Element *elementDst)
{
    QObject::connect(elementSrc,
                     SIGNAL(oStream(const void *, int, QString)),
                     elementDst,
                     SLOT(iStream(const void *, int, QString)));
}

void BaseElement::disconnect(Element *elementSrc, Element *elementDst)
{
    QObject::disconnect(elementSrc,
                        SIGNAL(oStream(const void *, int, QString)),
                        elementDst,
                        SLOT(iStream(const void *, int, QString)));
}

bool BaseElement::isLoaded(QString pluginId)
{
    return this->m_plugins.contains(pluginId);
}

bool BaseElement::load(QString pluginId)
{
    if (this->isLoaded(pluginId))
        return true;

    QString fileName;

    foreach (QString path, this->m_pluginsPaths)
    {
        QString filePath = QString("%1/lib%2.so").arg(path).arg(pluginId);

        if (QFileInfo(filePath).exists())
        {
            fileName = filePath;

            break;
        }
    }

    if (fileName.isEmpty() || !QLibrary::isLibrary(fileName))
        return false;

    this->m_pluginPath[pluginId] = fileName;
    this->m_pluginLoader.setFileName(fileName);

    if (!this->m_pluginLoader.load())
    {
        qDebug() << this->m_pluginLoader.errorString();

        return false;
    }

    Plugin *plugin = qobject_cast<Plugin *>(this->m_pluginLoader.instance());

    if (!plugin)
        return false;

    this->m_plugins[pluginId] = plugin;

    return true;
}

bool BaseElement::unload(QString pluginId)
{
    if(!this->isLoaded(pluginId))
         return true;

    delete this->m_plugins[pluginId];
    this->m_plugins.remove(pluginId);
    this->m_pluginLoader.setFileName(this->m_pluginPath[pluginId]);
    this->m_pluginLoader.unload();

    return true;
}

void BaseElement::setPluginsPaths(QStringList pluginsPaths)
{
    this->m_pluginsPaths = pluginsPaths;
}

void BaseElement::resetPluginsPaths()
{
    this->setPluginsPaths(QStringList());
}
