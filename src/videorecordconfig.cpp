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

#include "videorecordconfig.h"

VideoRecordConfig::VideoRecordConfig(MediaTools *mediaTools, QWidget *parent): QWidget(parent)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(true, this);
    this->m_isInit = true;
    QVariantList videoRecordFormats = this->m_mediaTools->videoRecordFormats();

    this->tbwVideoFormats->setRowCount(videoRecordFormats.length());

    int row = 0;

    foreach (QVariant fmt, videoRecordFormats)
    {
        int column = 0;

        foreach (QString param, fmt.toStringList())
        {
            this->tbwVideoFormats->setItem(row,
                                           column,
                                           new QTableWidgetItem(param));

            column++;
        }

        row++;
    }

    this->m_isInit = false;
    this->tbwVideoFormats->resizeRowsToContents();
    this->tbwVideoFormats->resizeColumnsToContents();
}

void VideoRecordConfig::update()
{
    this->tbwVideoFormats->resizeRowsToContents();
    this->tbwVideoFormats->resizeColumnsToContents();

    if (this->m_isInit)
        return;

    this->m_mediaTools->clearVideoRecordFormats();

    for (int row = 0; row < this->tbwVideoFormats->rowCount(); row++)
    {
        QString suffix = this->tbwVideoFormats->item(row, 0)->text();
        QString videoEncoder = this->tbwVideoFormats->item(row, 1)->text();
        QString audioEncoder = this->tbwVideoFormats->item(row, 2)->text();
        QString muxer = this->tbwVideoFormats->item(row, 3)->text();

        this->m_mediaTools->setVideoRecordFormat(suffix,
                                          videoEncoder,
                                          audioEncoder,
                                          muxer);
    }
}

void VideoRecordConfig::on_btnAdd_clicked()
{
    this->tbwVideoFormats->insertRow(this->tbwVideoFormats->rowCount());
    this->update();
}

void VideoRecordConfig::on_btnRemove_clicked()
{
    this->tbwVideoFormats->removeRow(this->tbwVideoFormats->currentRow());
    this->update();
}

void VideoRecordConfig::on_btnUp_clicked()
{
    int currentRow = this->tbwVideoFormats->currentRow();
    int nextRow = currentRow - 1;

    if (nextRow < 0)
        return;

    for (int column = 0; column < this->tbwVideoFormats->columnCount(); column++)
    {
        QString currentText = this->tbwVideoFormats->item(currentRow, column)->text();
        QString nextText = this->tbwVideoFormats->item(nextRow, column)->text();

        this->tbwVideoFormats->item(currentRow, column)->setText(nextText);
        this->tbwVideoFormats->item(nextRow, column)->setText(currentText);
    }

    this->tbwVideoFormats->
            setCurrentCell(nextRow, this->tbwVideoFormats->currentColumn());

    this->update();
}

void VideoRecordConfig::on_btnDown_clicked()
{
    int currentRow = this->tbwVideoFormats->currentRow();
    int nextRow = currentRow + 1;

    if (nextRow >= this->tbwVideoFormats->rowCount())
        return;

    for (int column = 0; column < this->tbwVideoFormats->columnCount(); column++)
    {
        QString currentText = this->tbwVideoFormats->item(currentRow, column)->text();
        QString nextText = this->tbwVideoFormats->item(nextRow, column)->text();

        this->tbwVideoFormats->item(currentRow, column)->setText(nextText);
        this->tbwVideoFormats->item(nextRow, column)->setText(currentText);
    }

    this->tbwVideoFormats->
            setCurrentCell(nextRow, this->tbwVideoFormats->currentColumn());

    this->update();
}

void VideoRecordConfig::on_tbwVideoFormats_cellChanged(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)

    this->update();
}
