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

#include "ui_streamsconfig.h"

#include "streamsconfig.h"

StreamsConfig::StreamsConfig(MediaTools *mediaTools, QWidget *parent):
    QWidget(parent),
    ui(new Ui::StreamsConfig)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->ui->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(true, this);
    this->m_isInit = true;
    QVariantList streams = this->m_mediaTools->streams();

    this->ui->tbwCustomStreams->setRowCount(streams.length());

    int row = 0;

    foreach (QVariant stream, streams)
    {
        QString name = stream.toList().at(1).toString();

        this->ui->tbwCustomStreams->setItem(row,
                                            0,
                                            new QTableWidgetItem(name));

        QString uri = stream.toList().at(0).toString();

        this->ui->tbwCustomStreams->setItem(row,
                                            1,
                                            new QTableWidgetItem(uri));

        bool hasAudio = stream.toList().at(2).toBool();
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(hasAudio? Qt::Checked: Qt::Unchecked);

        this->ui->tbwCustomStreams->setItem(row,
                                            2,
                                            item);

        bool playAudio = stream.toList().at(3).toBool();
        item = new QTableWidgetItem();
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(playAudio? Qt::Checked: Qt::Unchecked);

        this->ui->tbwCustomStreams->setItem(row,
                                            3,
                                            item);

        row++;
    }

    this->m_isInit = false;
    this->ui->tbwCustomStreams->resizeRowsToContents();
    this->ui->tbwCustomStreams->resizeColumnsToContents();
}

StreamsConfig::~StreamsConfig()
{
    delete this->ui;
}

void StreamsConfig::update()
{
    this->ui->tbwCustomStreams->resizeRowsToContents();
    this->ui->tbwCustomStreams->resizeColumnsToContents();

    if (this->m_isInit)
        return;

    this->m_mediaTools->clearCustomStreams();

    for (int row = 0; row < this->ui->tbwCustomStreams->rowCount(); row++)
    {
        QTableWidgetItem *item;
        QString description;
        QString devName;
        bool hasAudio = false;
        bool playAudio = false;

        if ((item = this->ui->tbwCustomStreams->item(row, 0)))
            description = item->text();

        if ((item = this->ui->tbwCustomStreams->item(row, 1)))
            devName = item->text();

        if ((item = this->ui->tbwCustomStreams->item(row, 2)))
            hasAudio = item->checkState() == Qt::Checked? true: false;

        if ((item = this->ui->tbwCustomStreams->item(row, 3)))
            playAudio = item->checkState() == Qt::Checked? true: false;

        this->m_mediaTools->setCustomStream(devName,
                                            description,
                                            hasAudio,
                                            playAudio);
    }
}

void StreamsConfig::on_btnAdd_clicked()
{
    int row = this->ui->tbwCustomStreams->rowCount();
    this->ui->tbwCustomStreams->insertRow(row);
    this->ui->tbwCustomStreams->setItem(row, 0, new QTableWidgetItem());
    this->ui->tbwCustomStreams->setItem(row, 1, new QTableWidgetItem());

    QTableWidgetItem *item = new QTableWidgetItem();
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setCheckState(Qt::Unchecked);
    this->ui->tbwCustomStreams->setItem(row, 2, item);

    item = new QTableWidgetItem();
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setCheckState(Qt::Unchecked);
    this->ui->tbwCustomStreams->setItem(row, 3, item);

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

    for (int column = 0; column < this->ui->tbwCustomStreams->columnCount(); column++)
    {
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

    for (int column = 0; column < this->ui->tbwCustomStreams->columnCount(); column++)
    {
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
