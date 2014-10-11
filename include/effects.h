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

#ifndef EFFECTS_H
#define EFFECTS_H

#include <QWidget>
#include <QListWidgetItem>
#include <qbutils.h>

#include "mediatools.h"

namespace Ui
{
    class Effects;
}

class Effects: public QWidget
{
    Q_OBJECT

    public:
        explicit Effects(MediaTools *mediaTools=NULL, QWidget *parent=NULL);

    private:
        QSharedPointer<Ui::Effects> ui;

        MediaTools *m_mediaTools;
        QStringList m_effectsNames;
        QList<QListWidgetItem *> m_effectsWidgets;
        QMap<QString, QString> m_effects;

        void update();

    protected:
        void showEvent(QShowEvent *event);
        void hideEvent(QHideEvent *event);

    signals:
        void showPreview(const QString &effect);
        void effectVisibilityChanged(bool visible);

    public slots:
        void setEffectPreview(const QbPacket &packet);
        void setApplyPreview(const QbPacket &packet);

    private slots:
        void updateEffectPreview();
        void streamStateChanged();
        void on_txtSearch_textChanged(QString text);
        void on_btnAdd_clicked();
        void on_btnRemove_clicked();
        void on_btnUp_clicked();
        void on_btnDown_clicked();
        void on_btnReset_clicked();
        void on_lswEffects_itemClicked(QListWidgetItem *item);
};

#endif // EFFECTS_H
