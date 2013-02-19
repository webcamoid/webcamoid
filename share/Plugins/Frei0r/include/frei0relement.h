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

#ifndef FREI0RELEMENT_H
#define FREI0RELEMENT_H

#include "qb.h"
#include "frei0rdefs.h"

class Frei0rElement: public QbElement
{
    Q_OBJECT

    Q_PROPERTY(QString pluginName READ pluginName
                                  WRITE setPluginName
                                  RESET resetPluginName)

    Q_PROPERTY(QSize frameSize READ frameSize
                               WRITE setFrameSize
                               RESET resetFrameSize)

    Q_PROPERTY(double fps READ fps
                          WRITE setFps
                          RESET resetFps)

    Q_PROPERTY(QVariantList indexMap READ indexMap
                                     WRITE setIndexMap
                                     RESET resetIndexMap)

    Q_PROPERTY(QVariantMap params READ params
                                  WRITE setParams
                                  RESET resetParams)

    Q_PROPERTY(QVariantMap info READ info)

    Q_PROPERTY(QStringList plugins READ plugins)

    Q_PROPERTY(QStringList frei0rPaths READ frei0rPaths
                                       WRITE setFrei0rPaths
                                       RESET resetFrei0rPaths)

    public:
        explicit Frei0rElement();
        ~Frei0rElement();

        Q_INVOKABLE QString pluginName() const;
        Q_INVOKABLE QSize frameSize() const;
        Q_INVOKABLE double fps() const;
        Q_INVOKABLE QVariantList indexMap() const;
        Q_INVOKABLE QVariantMap params();
        Q_INVOKABLE QVariantMap info();
        Q_INVOKABLE QStringList plugins() const;
        Q_INVOKABLE QStringList frei0rPaths() const;

    protected:
        bool init();
        void uninit();
        bool initBuffers();
        void uninitBuffers();

    private:
        QString m_pluginName;
        QSize m_frameSize;
        double m_fps;
        double m_t;
        double m_duration;
        QVariantList m_indexMap;
        QVariantMap m_params;
        QVariantMap m_info;
        QStringList m_frei0rPaths;
        QByteArray m_iBuffer0;
        QByteArray m_iBuffer1;
        QByteArray m_iBuffer2;
        QByteArray m_oBuffer;
        QbCaps m_curInputCaps;
        QbElementPtr m_capsConvert;
        QLibrary m_library;
        QTimer m_timer;
        f0r_instance_t m_f0rInstance;

        void cleanAll();

        typedef int (*f0r_init_t)();
        typedef void (*f0r_deinit_t)();
        typedef void (*f0r_get_plugin_info_t)(f0r_plugin_info_t* info);

        typedef void (*f0r_get_param_info_t)(f0r_param_info_t* info,
                                             int paramIndex);

        typedef f0r_instance_t (*f0r_construct_t)(unsigned int width,
                                                  unsigned int height);

        typedef void (*f0r_destruct_t)(f0r_instance_t instance);

        typedef void (*f0r_set_param_value_t)(f0r_instance_t instance,
                                              f0r_param_t param,
                                              int paramIndex);

        typedef void (*f0r_get_param_value_t)(f0r_instance_t instance,
                                              f0r_param_t param,
                                              int paramIndex);

        typedef void (*f0r_update_t)(f0r_instance_t instance,
                                     double time,
                                     const uint32_t* inframe,
                                     uint32_t* outframe);

        typedef void (*f0r_update2_t)(f0r_instance_t instance,
                                      double time,
                                      const uint32_t* inframe1,
                                      const uint32_t* inframe2,
                                      const uint32_t* inframe3,
                                      uint32_t* outframe);

        f0r_init_t f0rInit;
        f0r_deinit_t f0rDeinit;
        f0r_get_plugin_info_t f0rGetPluginInfo;
        f0r_get_param_info_t f0rGetParamInfo;
        f0r_construct_t f0rConstruct;
        f0r_destruct_t f0rDestruct;
        f0r_set_param_value_t f0rSetParamValue;
        f0r_get_param_value_t f0rGetParamValue;
        f0r_update_t f0rUpdate;
        f0r_update2_t f0rUpdate2;

    public slots:
        void setPluginName(QString pluginName);
        void setFrameSize(QSize frameSize);
        void setFps(double fps);
        void setIndexMap(QVariantList indexMap);
        void setParams(QVariantMap params);
        void setFrei0rPaths(QStringList frei0rPaths);
        void resetPluginName();
        void resetFrameSize();
        void resetFps();
        void resetIndexMap();
        void resetParams();
        void resetFrei0rPaths();

        void iStream(const QbPacket &packet);
        void setState(ElementState state);

    private slots:
        void processFrame(const QbPacket &packet=QbPacket());
};

#endif // FREI0RELEMENT_H
