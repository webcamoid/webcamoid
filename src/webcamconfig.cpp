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

#include <linux/videodev2.h>

#include "webcamconfig.h"

WebcamConfig::WebcamConfig(MediaTools *mediaTools, QWidget *parent): QWidget(parent)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(this);
    this->m_captureDevices = this->m_mediaTools->captureDevices();
    this->m_resetting = false;

    foreach (QVariant captureDevice, this->m_captureDevices)
    {
        if (captureDevice.toList().at(2) != MediaTools::StreamTypeWebcam)
            continue;

        QWidget *page = new QWidget();
        QGridLayout *gridLayout = new QGridLayout(page);

        int cindex = 0;

        QLabel *lblVideoFormat = new QLabel(page);
        lblVideoFormat->setText(this->tr("Video Format"));
        gridLayout->addWidget(lblVideoFormat, cindex, 0, 1, 1);

        this->m_videoFormats[captureDevice.toList().at(0).toString()] = \
             this->m_mediaTools->videoFormats(captureDevice.toList().at(0).toString());

        QWidget *wdgVideoFormat = new QWidget(page);

        wdgVideoFormat->setSizePolicy(QSizePolicy::Preferred,
                                      QSizePolicy::Maximum);

        QHBoxLayout *hlyVideoFormat = new QHBoxLayout(wdgVideoFormat);

        QComboBox *cbxVideoFormat = new QComboBox(wdgVideoFormat);
        cbxVideoFormat->setProperty("deviceName", captureDevice.toList().at(0));
        cbxVideoFormat->setProperty("controlName", "");
        cbxVideoFormat->setProperty("controlDefaultValue", 0);
        cbxVideoFormat->setProperty("deviceOption", "videoFormat");

        foreach (QVariant videoFormat, this->m_videoFormats[captureDevice.toList().at(0).toString()].toList())
            cbxVideoFormat->addItem(QString("%1x%2 %3").arg(videoFormat.toList().at(0).toInt())
                                                       .arg(videoFormat.toList().at(1).toInt())
                                                       .arg(this->m_mediaTools->fcc2s(videoFormat.toList().at(2).toUInt())));

        QVariantList currentVideoFormat = this->m_mediaTools->videoFormat(captureDevice.toList().at(0).toString());

        cbxVideoFormat->setCurrentIndex(this->m_videoFormats[captureDevice.toList().at(0).toString()]
                                            .toList()
                                            .indexOf(currentVideoFormat));

        QObject::connect(cbxVideoFormat,
                         SIGNAL(currentIndexChanged(int)),
                         this,
                         SLOT(comboboxCurrentIndexChanged(int)));

        hlyVideoFormat->addWidget(cbxVideoFormat);

        QSpacerItem *hspVideoFormat = new QSpacerItem(0,
                                                      0,
                                                      QSizePolicy::Expanding,
                                                      QSizePolicy::Minimum);

        hlyVideoFormat->addItem(hspVideoFormat);

        gridLayout->addWidget(wdgVideoFormat, cindex, 1, 1, 1);

        QPushButton *btnResetDevice = new QPushButton(page);
        btnResetDevice->setProperty("deviceName", captureDevice.toList().at(0));
        btnResetDevice->setProperty("controlName", "");
        btnResetDevice->setProperty("deviceOption", "resetDevice");
        btnResetDevice->setText(this->tr("Reset"));

        QObject::connect(btnResetDevice,
                         SIGNAL(clicked()),
                         this,
                         SLOT(pushButtonClicked()));

        gridLayout->addWidget(btnResetDevice, cindex, 2, 1, 1);

        cindex = 1;

        foreach (QVariant control, this->m_mediaTools->listControls(captureDevice.toList().at(0).toString()))
        {
            if (control.toList().at(1).toInt() == V4L2_CTRL_TYPE_INTEGER ||
                control.toList().at(1).toInt() == V4L2_CTRL_TYPE_INTEGER64)
            {
                QLabel *lblControl = new QLabel(page);
                lblControl->setText(control.toList().at(0).toString());
                gridLayout->addWidget(lblControl, cindex, 0, 1, 1);

                QSlider *sldControl = new QSlider(page);
                sldControl->setProperty("deviceName", captureDevice.toList().at(0).toString());
                sldControl->setProperty("controlName", control.toList().at(0).toString());
                sldControl->setProperty("controlDefaultValue", control.toList().at(5).toInt());
                sldControl->setOrientation(Qt::Horizontal);
                sldControl->setRange(control.toList().at(2).toInt(), control.toList().at(3).toInt());
                sldControl->setSingleStep(control.toList().at(4).toInt());
                sldControl->setValue(control.toList().at(6).toInt());

                QObject::connect(sldControl,
                                 SIGNAL(sliderMoved(int)),
                                 this,
                                 SLOT(sliderMoved(int)));

                gridLayout->addWidget(sldControl, cindex, 1, 1, 1);

                QSpinBox *spbControl = new QSpinBox(page);
                spbControl->setProperty("deviceName", captureDevice.toList().at(0).toString());
                spbControl->setProperty("controlName", control.toList().at(0).toString());
                spbControl->setProperty("controlDefaultValue", control.toList().at(5).toInt());
                spbControl->setRange(control.toList().at(2).toInt(), control.toList().at(3).toInt());
                spbControl->setSingleStep(control.toList().at(4).toInt());
                spbControl->setValue(control.toList().at(6).toInt());

                QObject::connect(spbControl,
                                 SIGNAL(valueChanged(int)),
                                 this,
                                 SLOT(spinboxValueChanged(int)));

                gridLayout->addWidget(spbControl, cindex, 2, 1, 1);

                QObject::connect(sldControl,
                                 SIGNAL(sliderMoved(int)),
                                 spbControl,
                                 SLOT(setValue(int)));

                QObject::connect(spbControl,
                                 SIGNAL(valueChanged(int)),
                                 sldControl,
                                 SLOT(setValue(int)));
            }
            else if (control.toList().at(1) == V4L2_CTRL_TYPE_BOOLEAN)
            {
                QCheckBox *chkControl = new QCheckBox(page);
                chkControl->setProperty("deviceName", captureDevice.toList().at(0));
                chkControl->setProperty("controlName", control.toList().at(0));
                chkControl->setProperty("controlDefaultValue", control.toList().at(5));
                chkControl->setText(control.toList().at(0).toString());
                chkControl->setChecked((control.toList().at(6) == 0)? false: true);

                QObject::connect(chkControl,
                                 SIGNAL(toggled(bool)),
                                 this,
                                 SLOT(checkboxToggled(bool)));

                gridLayout->addWidget(chkControl, cindex, 0, 1, 3);
            }
            else if (control.toList().at(1) == V4L2_CTRL_TYPE_MENU)
            {
                QLabel *lblControl = new QLabel(page);
                lblControl->setText(control.toList().at(0).toString());
                gridLayout->addWidget(lblControl, cindex, 0, 1, 1);

                QWidget *wdgControl = new QWidget(page);

                wdgControl->setSizePolicy(QSizePolicy::Preferred,
                                          QSizePolicy::Maximum);

                QHBoxLayout *hlyControl = new QHBoxLayout(wdgControl);

                QComboBox *cbxControl = new QComboBox(wdgControl);
                cbxControl->setProperty("deviceName", captureDevice.toList().at(0));
                cbxControl->setProperty("controlName", control.toList().at(0));
                cbxControl->setProperty("controlDefaultValue", control.toList().at(5));
                cbxControl->addItems(control.toList().at(7).toStringList());
                cbxControl->setCurrentIndex(control.toList().at(6).toInt());

                QObject::connect(cbxControl,
                                 SIGNAL(currentIndexChanged(int)),
                                 this,
                                 SLOT(comboboxCurrentIndexChanged(int)));

                hlyControl->addWidget(cbxControl);

                QSpacerItem *hspControl = new QSpacerItem(0,
                                                          0,
                                                          QSizePolicy::Expanding,
                                                          QSizePolicy::Minimum);

                hlyControl->addItem(hspControl);
                gridLayout->addWidget(wdgControl, cindex, 1, 1, 1);
            }
            else
                continue;

            cindex++;
        }

        QSpacerItem *spacerItem = new QSpacerItem(0,
                                                  0,
                                                  QSizePolicy::Preferred,
                                                  QSizePolicy::MinimumExpanding);

        this->gridLayout->addItem(spacerItem, cindex, 0, 1, 1);
        this->tabWebcams->addTab(page, captureDevice.toList().at(1).toString());
    }

    this->tabWebcams->setCurrentIndex(0);
}

void WebcamConfig::resetControls(QString deviceName)
{
    foreach (QObject *children, this->findChildren<QObject *>())
    {
        QVariant variantChildrenDeviceName = children->property("deviceName");
        QVariant variantChildrenControlDefaultValue = children->property("controlDefaultValue");

        if (variantChildrenDeviceName.isValid() &&
            variantChildrenControlDefaultValue.isValid())
        {
            QString childrenDeviceName = variantChildrenDeviceName.toString();

            int childrenControlDefaultValue = \
                            variantChildrenControlDefaultValue.toInt();

            if (childrenDeviceName == deviceName)
            {
                if (QComboBox *comboBox = qobject_cast<QComboBox *>(children))
                    comboBox->setCurrentIndex(childrenControlDefaultValue);
                else if (QSlider *slider = qobject_cast<QSlider *>(children))
                    slider->setValue(childrenControlDefaultValue);
                else if (QSpinBox *spinBox = qobject_cast<QSpinBox *>(children))
                    spinBox->setValue(childrenControlDefaultValue);
                else if (QCheckBox *checkBox = qobject_cast<QCheckBox *>(children))
                    checkBox->setChecked((childrenControlDefaultValue == 0)? false: true);
            }
        }
    }
}

void WebcamConfig::pushButtonClicked()
{
    QObject *control = this->sender();
    QString deviceName = control->property("deviceName").toString();
    QString controlName = control->property("controlName").toString();
    QString deviceOption = control->property("deviceOption").toString();

    if (controlName.isEmpty() &&
        deviceOption == "resetDevice")
    {
            this->m_resetting = true;
            this->m_mediaTools->reset(deviceName);
            this->resetControls(deviceName);
            this->m_resetting = false;
    }
}

void WebcamConfig::sliderMoved(int value)
{
    QObject *control = this->sender();
    QString deviceName = control->property("deviceName").toString();
    QString controlName = control->property("controlName").toString();

    if (!this->m_resetting)
    {
        QMap<QString, uint> controlMap;

        controlMap[controlName] = value;
        this->m_mediaTools->setControls(deviceName, controlMap);
    }
}

void WebcamConfig::spinboxValueChanged(int i)
{
    QObject *control = this->sender();
    QString deviceName = control->property("deviceName").toString();
    QString controlName = control->property("controlName").toString();

    if (!this->m_resetting)
    {
        QMap<QString, uint> controlMap;

        controlMap[controlName] = i;
        this->m_mediaTools->setControls(deviceName, controlMap);
    }
}

void WebcamConfig::checkboxToggled(bool checked)
{
    QObject *control = this->sender();
    QString deviceName = control->property("deviceName").toString();
    QString controlName = control->property("controlName").toString();

    if (!this->m_resetting)
    {
        QMap<QString, uint> controlMap;

        controlMap[controlName] = checked? 1: 0;
        this->m_mediaTools->setControls(deviceName, controlMap);
    }
}

void WebcamConfig::comboboxCurrentIndexChanged(int index)
{
    QObject *control = this->sender();
    QString deviceName = control->property("deviceName").toString();
    QString controlName = control->property("controlName").toString();
    QString deviceOption = control->property("deviceOption").toString();

    if (controlName.isEmpty())
    {
        if (deviceOption == "videoFormat" && !this->m_resetting)
            this->m_mediaTools->setVideoFormat(this->m_videoFormats[deviceName].toList().at(index).toList(), deviceName);
    }
    else
    {
        QMap<QString, uint> controlMap;

        controlMap[controlName] = index;
        this->m_mediaTools->setControls(deviceName, controlMap);
    }
}
