/* Webcamoid, webcam capture application.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#ifndef AKPALETTEGROUP_H
#define AKPALETTEGROUP_H

#include <QPalette>

#include "../akcommons.h"

class AkPaletteGroupPrivate;

class AKCOMMONS_EXPORT AkPaletteGroup: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor highlightedText
               READ highlightedText
               WRITE setHighlightedText
               RESET resetHighlightedText
               NOTIFY highlightedTextChanged)
    Q_PROPERTY(QColor highlight
               READ highlight
               WRITE setHighlight
               RESET resetHighlight
               NOTIFY highlightChanged)
    Q_PROPERTY(QColor text
               READ text
               WRITE setText
               RESET resetText
               NOTIFY textChanged)
    Q_PROPERTY(QColor placeholderText
               READ placeholderText
               WRITE setPlaceholderText
               RESET resetPlaceholderText
               NOTIFY placeholderTextChanged)
    Q_PROPERTY(QColor base
               READ base
               WRITE setBase
               RESET resetBase
               NOTIFY baseChanged)
    Q_PROPERTY(QColor windowText
               READ windowText
               WRITE setWindowText
               RESET resetWindowText
               NOTIFY windowTextChanged)
    Q_PROPERTY(QColor window
               READ window
               WRITE setWindow
               RESET resetWindow
               NOTIFY windowChanged)
    Q_PROPERTY(QColor buttonText
               READ buttonText
               WRITE setButtonText
               RESET resetButtonText
               NOTIFY buttonTextChanged)
    Q_PROPERTY(QColor light
               READ light
               WRITE setLight
               RESET resetLight
               NOTIFY lightChanged)
    Q_PROPERTY(QColor midlight
               READ midlight
               WRITE setMidlight
               RESET resetMidlight
               NOTIFY midlightChanged)
    Q_PROPERTY(QColor button
               READ button
               WRITE setButton
               RESET resetButton
               NOTIFY buttonChanged)
    Q_PROPERTY(QColor mid
               READ mid
               WRITE setMid
               RESET resetMid
               NOTIFY midChanged)
    Q_PROPERTY(QColor dark
               READ dark
               WRITE setDark
               RESET resetDark
               NOTIFY darkChanged)
    Q_PROPERTY(QColor shadow
               READ shadow
               WRITE setShadow
               RESET resetShadow
               NOTIFY shadowChanged)
    Q_PROPERTY(QColor toolTipText
               READ toolTipText
               WRITE setToolTipText
               RESET resetToolTipText
               NOTIFY toolTipTextChanged)
    Q_PROPERTY(QColor toolTipBase
               READ toolTipBase
               WRITE setToolTipBase
               RESET resetToolTipBase
               NOTIFY toolTipBaseChanged)
    Q_PROPERTY(QColor link
               READ link
               WRITE setLink
               RESET resetLink
               NOTIFY linkChanged)
    Q_PROPERTY(QColor linkVisited
               READ linkVisited
               WRITE setLinkVisited
               RESET resetLinkVisited
               NOTIFY linkVisitedChanged)

    public:
        explicit AkPaletteGroup(QObject *parent=nullptr);
        AkPaletteGroup(QPalette::ColorGroup colorGroup);
        AkPaletteGroup(const AkPaletteGroup &other);
        ~AkPaletteGroup();
        AkPaletteGroup &operator =(const AkPaletteGroup &other);
        bool operator ==(const AkPaletteGroup &other) const;

        Q_INVOKABLE QColor highlightedText() const;
        Q_INVOKABLE QColor highlight() const;
        Q_INVOKABLE QColor text() const;
        Q_INVOKABLE QColor placeholderText() const;
        Q_INVOKABLE QColor base() const;
        Q_INVOKABLE QColor windowText() const;
        Q_INVOKABLE QColor window() const;
        Q_INVOKABLE QColor buttonText() const;
        Q_INVOKABLE QColor light() const;
        Q_INVOKABLE QColor midlight() const;
        Q_INVOKABLE QColor button() const;
        Q_INVOKABLE QColor mid() const;
        Q_INVOKABLE QColor dark() const;
        Q_INVOKABLE QColor shadow() const;
        Q_INVOKABLE QColor toolTipText() const;
        Q_INVOKABLE QColor toolTipBase() const;
        Q_INVOKABLE QColor link() const;
        Q_INVOKABLE QColor linkVisited() const;

    private:
        AkPaletteGroupPrivate *d;

    signals:
        void highlightedTextChanged(const QColor &highlightedText);
        void highlightChanged(const QColor &highlight);
        void textChanged(const QColor &text);
        void placeholderTextChanged(const QColor &placeholderText);
        void baseChanged(const QColor &base);
        void windowTextChanged(const QColor &windowText);
        void windowChanged(const QColor &window);
        void buttonTextChanged(const QColor &buttonText);
        void lightChanged(const QColor &light);
        void midlightChanged(const QColor &midlight);
        void buttonChanged(const QColor &button);
        void midChanged(const QColor &mid);
        void darkChanged(const QColor &dark);
        void shadowChanged(const QColor &shadow);
        void toolTipTextChanged(const QColor &toolTipText);
        void toolTipBaseChanged(const QColor &toolTipBase);
        void linkChanged(const QColor &link);
        void linkVisitedChanged(const QColor &linkVisited);

    public slots:
        void setHighlightedText(const QColor &highlightedText);
        void setHighlight(const QColor &highlight);
        void setText(const QColor &text);
        void setPlaceholderText(const QColor &placeholderText);
        void setBase(const QColor &base);
        void setWindowText(const QColor &windowText);
        void setWindow(const QColor &window);
        void setButtonText(const QColor &buttonText);
        void setLight(const QColor &light);
        void setMidlight(const QColor &midlight);
        void setButton(const QColor &button);
        void setMid(const QColor &mid);
        void setDark(const QColor &dark);
        void setShadow(const QColor &shadow);
        void setToolTipText(const QColor &toolTipText);
        void setToolTipBase(const QColor &toolTipBase);
        void setLink(const QColor &link);
        void setLinkVisited(const QColor &linkVisited);
        void resetHighlightedText();
        void resetHighlight();
        void resetText();
        void resetPlaceholderText();
        void resetBase();
        void resetWindowText();
        void resetWindow();
        void resetButtonText();
        void resetLight();
        void resetMidlight();
        void resetButton();
        void resetMid();
        void resetDark();
        void resetShadow();
        void resetToolTipText();
        void resetToolTipBase();
        void resetLink();
        void resetLinkVisited();
        static void registerTypes();

    private slots:
        void updatePalette(const QPalette &palette);
};

Q_DECLARE_METATYPE(AkPaletteGroup)

#endif // AKPALETTEGROUP_H
