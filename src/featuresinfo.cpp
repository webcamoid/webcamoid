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

#include "featuresinfo.h"

FeaturesInfo::FeaturesInfo(V4L2Tools *tools, QWidget *parent): QWidget(parent)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->setupUi(this);

    this->m_tools = (tools)? tools: new V4L2Tools(true, this);
    this->on_btnRecheck_clicked();
}

void FeaturesInfo::on_btnRecheck_clicked()
{
    QVariantMap features = this->m_tools->featuresMatrix();
    this->tbwFeatures->setRowCount(features.size());

    QStringList featuresList = features.keys();

    featuresList.sort();
    int row = 0;

    foreach (QString module, featuresList)
    {
        QString iconName = (features[module].toList().at(0).toBool())? "ok": "cancel";

        this->tbwFeatures->setItem(row,
                                   0,
                                   new QTableWidgetItem(QIcon::fromTheme(iconName), features[module].toList().at(1).toString()));

        this->tbwFeatures->setItem(row,
                                   1,
                                   new QTableWidgetItem(features[module].toList().at(2).toString()));

        row++;
    }

    this->tbwFeatures->resizeRowsToContents();
    this->tbwFeatures->resizeColumnsToContents();
}
