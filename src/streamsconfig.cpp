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

#include "streamsconfig.h"

StreamsConfig::StreamsConfig(MediaTools *mediaTools, QWidget *parent): QWidget(parent)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(true, this);
    this->m_isInit = true;
    QVariantList streams = this->m_mediaTools->streams();

    this->tbwCustomStreams->setRowCount(streams.length());

    int row = 0;

    foreach (QVariant stream, streams)
    {
        QString param = stream.toStringList().at(1);

        this->tbwCustomStreams->setItem(row,
                                        0,
                                        new QTableWidgetItem(param));

        param = stream.toStringList().at(0);

        this->tbwCustomStreams->setItem(row,
                                        1,
                                        new QTableWidgetItem(param));

        row++;
    }

    this->m_isInit = false;
    this->tbwCustomStreams->resizeRowsToContents();
    this->tbwCustomStreams->resizeColumnsToContents();
}

void StreamsConfig::update()
{
    this->tbwCustomStreams->resizeRowsToContents();
    this->tbwCustomStreams->resizeColumnsToContents();

    if (this->m_isInit)
        return;

    this->m_mediaTools->clearCustomStreams();

    for (int row = 0; row < this->tbwCustomStreams->rowCount(); row++)
    {
        QString description = this->tbwCustomStreams->item(row, 0)->text();
        QString devName = this->tbwCustomStreams->item(row, 1)->text();

        this->m_mediaTools->setCustomStream(devName, description);
    }
}

void StreamsConfig::on_btnAdd_clicked()
{
    this->tbwCustomStreams->insertRow(this->tbwCustomStreams->rowCount());
    this->update();
}

void StreamsConfig::on_btnRemove_clicked()
{
    this->tbwCustomStreams->removeRow(this->tbwCustomStreams->currentRow());
    this->update();
}

void StreamsConfig::on_btnUp_clicked()
{
    int currentRow = this->tbwCustomStreams->currentRow();
    int nextRow = currentRow - 1;

    if (nextRow < 0)
        return;

    for (int column = 0; column < this->tbwCustomStreams->columnCount(); column++)
    {
        QString currentText = this->tbwCustomStreams->item(currentRow, column)->text();
        QString nextText = this->tbwCustomStreams->item(nextRow, column)->text();

        this->tbwCustomStreams->item(currentRow, column)->setText(nextText);
        this->tbwCustomStreams->item(nextRow, column)->setText(currentText);
    }

    this->tbwCustomStreams->
            setCurrentCell(nextRow, this->tbwCustomStreams->currentColumn());

    this->update();
}

void StreamsConfig::on_btnDown_clicked()
{
    int currentRow = this->tbwCustomStreams->currentRow();
    int nextRow = currentRow + 1;

    if (nextRow >= this->tbwCustomStreams->rowCount())
        return;

    for (int column = 0; column < this->tbwCustomStreams->columnCount(); column++)
    {
        QString currentText = this->tbwCustomStreams->item(currentRow, column)->text();
        QString nextText = this->tbwCustomStreams->item(nextRow, column)->text();

        this->tbwCustomStreams->item(currentRow, column)->setText(nextText);
        this->tbwCustomStreams->item(nextRow, column)->setText(currentText);
    }

    this->tbwCustomStreams->
            setCurrentCell(nextRow, this->tbwCustomStreams->currentColumn());

    this->update();
}

void StreamsConfig::on_tbwCustomStreams_cellChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)

    this->update();
}
