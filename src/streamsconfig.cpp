/* Webcamoid, webcam capture application.
 * Copyright (C) 2011-2014  Gonzalo Exequiel Pedone
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
 * Web-Site 2: http://opendesktop.org/content/show.php/Webcamoid?content=144796
 */

#include "ui_streamsconfig.h"

#include "streamsconfig.h"

StreamsConfig::StreamsConfig(MediaTools *mediaTools, QWidget *parent):
    QWidget(parent),
    ui(new Ui::StreamsConfig)
{
    this->ui->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(NULL, this);
    this->m_isInit = true;
    QStringList streams = this->m_mediaTools->streams();

    this->ui->tbwCustomStreams->setRowCount(streams.length());

    int row = 0;

    foreach (QString stream, streams) {
        if (!this->m_mediaTools->canModify(stream))
            continue;

        QString name = this->m_mediaTools->streamDescription(stream);

        this->ui->tbwCustomStreams->setItem(row,
                                            0,
                                            new QTableWidgetItem(name));

        this->ui->tbwCustomStreams->setItem(row,
                                            1,
                                            new QTableWidgetItem(stream));

        row++;
    }

    this->m_isInit = false;
    this->ui->tbwCustomStreams->resizeRowsToContents();
    this->ui->tbwCustomStreams->resizeColumnsToContents();
}

void StreamsConfig::update()
{
    this->ui->tbwCustomStreams->resizeRowsToContents();
    this->ui->tbwCustomStreams->resizeColumnsToContents();

    if (this->m_isInit)
        return;

    this->m_mediaTools->resetStreams();

    for (int row = 0; row < this->ui->tbwCustomStreams->rowCount(); row++) {
        QTableWidgetItem *item;
        QString description;
        QString devName;

        if ((item = this->ui->tbwCustomStreams->item(row, 0)))
            description = item->text();

        if ((item = this->ui->tbwCustomStreams->item(row, 1)))
            devName = item->text();

        this->m_mediaTools->setStream(devName, description);
    }
}

void StreamsConfig::on_btnAdd_clicked()
{
    int row = this->ui->tbwCustomStreams->rowCount();
    this->ui->tbwCustomStreams->insertRow(row);
    this->ui->tbwCustomStreams->setItem(row, 0, new QTableWidgetItem());
    this->ui->tbwCustomStreams->setItem(row, 1, new QTableWidgetItem());

    this->update();
}

void StreamsConfig::on_btnRemove_clicked()
{
    this->ui->tbwCustomStreams->removeRow(this->ui->tbwCustomStreams->currentRow());
    this->update();
}

void StreamsConfig::on_btnUp_clicked()
{
    int currentRow = this->ui->tbwCustomStreams->currentRow();
    int nextRow = currentRow - 1;

    if (nextRow < 0)
        return;

    for (int column = 0; column < this->ui->tbwCustomStreams->columnCount(); column++) {
        QTableWidgetItem *currentItem = this->ui->tbwCustomStreams->takeItem(currentRow, column);
        QTableWidgetItem *nextItem = this->ui->tbwCustomStreams->takeItem(nextRow, column);

        this->ui->tbwCustomStreams->setItem(currentRow, column, nextItem);
        this->ui->tbwCustomStreams->setItem(nextRow, column, currentItem);
    }

    this->ui->tbwCustomStreams->
            setCurrentCell(nextRow, this->ui->tbwCustomStreams->currentColumn());

    this->update();
}

void StreamsConfig::on_btnDown_clicked()
{
    int currentRow = this->ui->tbwCustomStreams->currentRow();
    int nextRow = currentRow + 1;

    if (nextRow >= this->ui->tbwCustomStreams->rowCount())
        return;

    for (int column = 0; column < this->ui->tbwCustomStreams->columnCount(); column++) {
        QTableWidgetItem *currentItem = this->ui->tbwCustomStreams->takeItem(currentRow, column);
        QTableWidgetItem *nextItem = this->ui->tbwCustomStreams->takeItem(nextRow, column);

        this->ui->tbwCustomStreams->setItem(currentRow, column, nextItem);
        this->ui->tbwCustomStreams->setItem(nextRow, column, currentItem);
    }

    this->ui->tbwCustomStreams->
            setCurrentCell(nextRow, this->ui->tbwCustomStreams->currentColumn());

    this->update();
}

void StreamsConfig::on_tbwCustomStreams_cellChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)

    this->update();
}
