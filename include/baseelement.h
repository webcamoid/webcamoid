#ifndef BASEELEMENT_H
#define BASEELEMENT_H

#include <QObject>

#include "plugin.h"

class BaseElement: public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList pluginsPaths
               READ pluginsPaths
               WRITE setPluginsPaths
               RESET resetPluginsPaths)

    public:
        explicit BaseElement(QObject *parent=0);

        Q_INVOKABLE QStringList pluginsPaths();
        Q_INVOKABLE Element *createElement(QString pluginId);
        Q_INVOKABLE void destroyElement(Element *element);
        static void connect(Element *elementSrc, Element *elementDst);
        static void disconnect(Element *elementSrc, Element *elementDst);

    private:
        QStringList m_pluginsPaths;
        QPluginLoader m_pluginLoader;
        QMap<QString, Plugin *> m_plugins;
        QMap<Element *, QString> m_elements;
        QMap<QString, QString> m_pluginPath;

        bool isLoaded(QString pluginId);
        bool load(QString pluginId);
        bool unload(QString pluginId);

    public slots:
        void setPluginsPaths(QStringList pluginsPaths);
        void resetPluginsPaths();
};

#endif // BASEELEMENT_H
