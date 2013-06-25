/* Webcamod, webcam capture plasmoid.
 * Copyright (C) 2011-2013  Gonzalo Exequiel Pedone
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

#include "ui_effects.h"

#include "effects.h"

Effects::Effects(MediaTools *mediaTools, QWidget *parent):
    QWidget(parent),
    ui(new Ui::Effects)
{
    this->m_appEnvironment = new AppEnvironment(this);

    this->ui->setupUi(this);

    this->m_mediaTools = mediaTools? mediaTools: new MediaTools(this);

    QObject::connect(this->m_mediaTools,
                     SIGNAL(deviceChanged(QString)),
                     this,
                     SLOT(deviceChanged(QString)));

    QObject::connect(this->m_mediaTools,
                     SIGNAL(previewFrameReady(const QImage &, QString)),
                     this,
                     SLOT(setEffectPreview(const QImage &, QString)));

    QMap<QString, QString> effects = this->m_mediaTools->availableEffects();

    foreach (QString effect, effects.keys())
    {
        QListWidgetItem *listWidgetItem = new QListWidgetItem(effects[effect]);

        this->m_effectsNames << effect;
        this->m_effectsWidgets << listWidgetItem;
        this->ui->lswEffects->addItem(listWidgetItem);
    }

    this->ui->lswEffects->sortItems();

    foreach (QString effect, this->m_mediaTools->currentEffects())
    {
        int index = this->m_effectsNames.indexOf(effect);

        if (index < 0)
            continue;

        QListWidgetItem *listWidgetItem = this->ui->lswEffects->takeItem(this->ui->lswEffects->row(this->m_effectsWidgets[index]));
        this->ui->lswApply->addItem(listWidgetItem);
    }
}

void Effects::update()
{
   if (!this->m_mediaTools)
       return;

   QStringList effects;

   for (int i = 0; i < this->ui->lswApply->count(); i++)
   {
       int index = this->m_effectsWidgets.indexOf(this->ui->lswApply->item(i));

       effects << this->m_effectsNames[index];
   }

   this->m_mediaTools->setEffects(effects);
}

void Effects::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    this->m_mediaTools->setEffectsPreview(true);
}

void Effects::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    this->m_mediaTools->resetEffectsPreview();
}

void Effects::setEffectPreview(const QImage &image, QString effect)
{
    if (!this->m_mediaTools->device().isEmpty())
    {
        int index = this->m_effectsNames.indexOf(effect);

        if (index < 0)
            return;

        this->m_effectsWidgets[index]->setIcon(QIcon(QPixmap::fromImage(image)));
    }
}

void Effects::deviceChanged(QString device)
{
    if (device.isEmpty())
        foreach (QListWidgetItem *effectWidget, this->m_effectsWidgets)
            effectWidget->setIcon(QIcon());
    else if (this->isVisible())
        this->m_mediaTools->setEffectsPreview(true);
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
    foreach (QListWidgetItem *item, this->ui->lswApply->selectedItems())
    {
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
    foreach (QListWidgetItem *item, this->ui->lswApply->selectedItems())
    {
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
