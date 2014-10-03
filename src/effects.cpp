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

#include "ui_effects.h"

#include "effects.h"

Effects::Effects(MediaTools *mediaTools, QWidget *parent):
    QWidget(parent),
    ui(new Ui::Effects)
{
    this->ui->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(this);

    QObject::connect(this->m_mediaTools,
                     SIGNAL(deviceChanged(QString)),
                     this,
                     SLOT(deviceChanged(QString)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(effectPreviewReady(const QbPacket &)),
                     this,
                     SLOT(setEffectPreview(const QbPacket &)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(applyPreviewReady(const QbPacket &)),
                     this,
                     SLOT(setApplyPreview(const QbPacket &)));

    QObject::connect(this,
                     SIGNAL(effectVisibilityChanged(bool)),
                     this->m_mediaTools,
                     SLOT(connectPreview(bool)));

    this->m_effects = this->m_mediaTools->availableEffects();

    foreach (QString effect, this->m_effects.keys()) {
        QListWidgetItem *listWidgetItem = new QListWidgetItem(this->m_effects[effect]);

        this->m_effectsNames << effect;
        this->m_effectsWidgets << listWidgetItem;
        this->ui->lswEffects->addItem(listWidgetItem);
    }

    this->ui->lswEffects->sortItems();

    foreach (QString effect, this->m_mediaTools->currentEffects()) {
        int index = this->m_effectsNames.indexOf(effect);

        if (index < 0)
            continue;

        QListWidgetItem *listWidgetItem = this->ui->lswEffects->takeItem(this->ui->lswEffects->row(this->m_effectsWidgets[index]));
        this->ui->lswApply->addItem(listWidgetItem);
    }

    if (this->m_effects.count() > 0)
        this->ui->lswEffects->setCurrentRow(0);
}

void Effects::update()
{
   if (!this->m_mediaTools)
       return;

   QStringList effects;

   for (int i = 0; i < this->ui->lswApply->count(); i++) {
       int index = this->m_effectsWidgets.indexOf(this->ui->lswApply->item(i));

       effects << this->m_effectsNames[index];
   }

   this->m_mediaTools->setEffects(effects);
   this->ui->lswEffects->setEnabled(this->ui->lswApply->count() < 5);
}

void Effects::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    this->updateEffectPreview();
    emit this->effectVisibilityChanged(true);
}

void Effects::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    this->m_mediaTools->resetEffectsPreview();
    emit this->effectVisibilityChanged(false);
}

void Effects::setEffectPreview(const QbPacket &packet)
{
    if (!this->m_mediaTools->device().isEmpty()) {
        QImage image = QbUtils::packetToImage(packet);
        this->ui->lblEffectsPreview->setPixmap(QPixmap::fromImage(image));
    }
}

void Effects::setApplyPreview(const QbPacket &packet)
{
    if (!this->m_mediaTools->device().isEmpty()) {
        QImage image = QbUtils::packetToImage(packet);
        this->ui->lblApplyPreview->setPixmap(QPixmap::fromImage(image));
    }
}

void Effects::updateEffectPreview()
{
    if (this->ui->lswEffects->count() < 1)
        this->m_mediaTools->setEffectsPreview(this->m_effects.keys()[0]);
    else if (this->ui->lswEffects->currentRow() >= 0) {
        QString effectName = this->ui->lswEffects->currentItem()->text();
        this->m_mediaTools->setEffectsPreview(this->m_effects.key(effectName));
    }
}

void Effects::deviceChanged(const QString &device)
{
    if (!device.isEmpty() && this->isVisible())
        this->updateEffectPreview();
}

void Effects::on_txtSearch_textChanged(QString text)
{
    QRegExp regexp(text, Qt::CaseInsensitive, QRegExp::Wildcard);

    for (int i = 0; i < this->ui->lswEffects->count(); i++)
        if (regexp.indexIn(this->ui->lswEffects->item(i)->text()) == -1)
            this->ui->lswEffects->item(i)->setHidden(true);
        else
            this->ui->lswEffects->item(i)->setHidden(false);

    for (int i = 0; i < this->ui->lswApply->count(); i++)
        if (regexp.indexIn(this->ui->lswApply->item(i)->text()) == -1)
            this->ui->lswApply->item(i)->setHidden(true);
        else
            this->ui->lswApply->item(i)->setHidden(false);
}

void Effects::on_btnAdd_clicked()
{
    foreach (QListWidgetItem *item, this->ui->lswEffects->selectedItems())
        this->ui->lswApply->addItem(this->ui->lswEffects->
                                    takeItem(this->ui->lswEffects->row(item)));

    this->update();
}

void Effects::on_btnRemove_clicked()
{
    foreach (QListWidgetItem *item, this->ui->lswApply->selectedItems())
        this->ui->lswEffects->addItem(this->ui->lswApply->
                                      takeItem(this->ui->lswApply->row(item)));

    this->ui->lswEffects->sortItems();
    this->update();
}

void Effects::on_btnUp_clicked()
{
    foreach (QListWidgetItem *item, this->ui->lswApply->selectedItems()) {
        int row = this->ui->lswApply->row(item);
        int row_ = (row >= 1)? row - 1: 0;
        QListWidgetItem *item_ = this->ui->lswApply->takeItem(row);
        this->ui->lswApply->insertItem(row_, item_);
        item_->setSelected(true);
    }

    this->update();
}

void Effects::on_btnDown_clicked()
{
    foreach (QListWidgetItem *item, this->ui->lswApply->selectedItems()) {
        int row = this->ui->lswApply->row(item);
        int row_ = (row < this->ui->lswApply->count() - 1)? row + 1: this->ui->lswApply->count() - 1;
        QListWidgetItem *item_ = this->ui->lswApply->takeItem(row);
        this->ui->lswApply->insertItem(row_, item_);
        item_->setSelected(true);
    }

    this->update();
}

void Effects::on_btnReset_clicked()
{
    while (this->ui->lswApply->count() > 0)
        this->ui->lswEffects->addItem(this->ui->lswApply->takeItem(0));

    this->ui->lswEffects->sortItems();
    this->update();
}

void Effects::on_lswEffects_itemClicked(QListWidgetItem *item)
{
    if (!item)
        return;

    this->updateEffectPreview();
}
