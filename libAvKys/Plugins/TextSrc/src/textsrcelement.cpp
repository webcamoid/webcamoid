/* Webcamoid, camera capture application.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QRegularExpression>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QMutex>
#include <QQmlContext>
#include <QSettings>
#include <QStringConverter>
#include <QThreadPool>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QWaitCondition>
#include <QtConcurrent>
#include <ak.h>
#include <akcaps.h>
#include <akfrac.h>
#include <akpacket.h>
#include <akvideocaps.h>
#include <akvideopacket.h>

#include "textsrcelement.h"
#include "textrenderer.h"

#define MAX_FILE_BYTES  (1 << 20)
#define MAX_TEXT_CHARS  100000
#define HEARTBEAT_MS    250
#define MAX_SUMMARY_CHARS 40

class TextSrcElementPrivate
{
    public:
        TextSrcElement *self;
        QString m_media;
        QString m_text {"Text"};
        QString m_textFile;
        QString m_fileText;
        QString m_fileError;
        QString m_summary {"Text"};
        TextSrcElement::TextSource m_textSource {TextSrcElement::TextSourceInline};
        TextRenderer::Params m_params;
        QImage m_image;
        AkFrac m_fps {30000, 1001};
        qint64 m_id {-1};
        bool m_dirty {true};
        bool m_run {false};
        bool m_loading {false};
        bool m_removed {false};
        mutable QMutex m_mutex;
        QWaitCondition m_cond;
        QThreadPool m_threadPool;
        QFuture<void> m_thread;
        QFileSystemWatcher m_watcher;
        QTimer m_reloadTimer;
        QTimer m_saveTimer;

        explicit TextSrcElementPrivate(TextSrcElement *self);
        QString settingsKey() const;
        void scheduleSave();
        void saveSettings();
        void loadSettings();
        void reloadFile();
        void updateWatcher();
        void updateSummary();
        void start();
        void stop();
        void run();
        void sendFrame(const QImage &image);
        static QString readTextFile(const QString &path, QString *error);
};

TextSrcElement::TextSrcElement():
    AkMultimediaSourceElement()
{
    this->d = new TextSrcElementPrivate(this);
}

TextSrcElement::~TextSrcElement()
{
    this->d->stop();

    if (this->d->m_saveTimer.isActive()) {
        this->d->m_saveTimer.stop();
        this->d->saveSettings();
    }

    delete this->d;
}

QStringList TextSrcElement::medias()
{
    if (this->d->m_media.isEmpty())
        return {};

    return {this->d->m_media};
}

QString TextSrcElement::media() const
{
    return this->d->m_media;
}

QList<int> TextSrcElement::streams()
{
    if (this->d->m_media.isEmpty())
        return {};

    return {0};
}

int TextSrcElement::defaultStream(AkCaps::CapsType type)
{
    if (type == AkCaps::CapsVideo)
        return 0;

    return -1;
}

QString TextSrcElement::description(const QString &media)
{
    if (media.isEmpty() || media != this->d->m_media)
        return {};

    return QString("Text");
}

AkCaps TextSrcElement::caps(int stream)
{
    if (stream != 0 || this->d->m_media.isEmpty())
        return {};

    QMutexLocker locker(&this->d->m_mutex);
    auto size = TextRenderer::clampedSize(this->d->m_params.width,
                                          this->d->m_params.height);

    return AkVideoCaps(AkVideoCaps::Format_argbpack,
                       size.width(),
                       size.height(),
                       this->d->m_fps);
}

QStringList TextSrcElement::fontFamilies() const
{
    return QFontDatabase::families();
}

QString TextSrcElement::fileError() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_fileError;
}

QString TextSrcElement::summary() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_summary;
}

QString TextSrcElement::controlInterfaceProvide(const QString &controlId) const
{
    Q_UNUSED(controlId)

    return QString("qrc:/TextSrc/share/qml/main.qml");
}

void TextSrcElement::controlInterfaceConfigure(QQmlContext *context,
                                               const QString &controlId) const
{
    Q_UNUSED(controlId)

    context->setContextProperty("TextSrc", const_cast<QObject *>(qobject_cast<const QObject *>(this)));
    context->setContextProperty("controlId", this->objectName());
}

void TextSrcElement::removeSettings()
{
    this->d->m_removed = true;
    this->d->m_saveTimer.stop();

    if (this->d->m_media.isEmpty())
        return;

    QSettings settings;
    settings.beginGroup("TextSrc");
    settings.remove(this->d->settingsKey());
    settings.endGroup();
}

void TextSrcElement::setMedia(const QString &media)
{
    if (this->d->m_media == media)
        return;

    auto state = this->state();
    this->setState(AkElement::ElementStateNull);
    this->d->m_media = media;

    if (!media.isEmpty())
        this->d->loadSettings();

    emit this->mediaChanged(media);
    emit this->mediasChanged(this->medias());
    emit this->streamsChanged(this->streams());

    if (!media.isEmpty() && state != AkElement::ElementStateNull)
        this->setState(state);
}

void TextSrcElement::resetMedia()
{
    this->setMedia({});
}

bool TextSrcElement::setState(AkElement::ElementState state)
{
    if (this->d->m_media.isEmpty())
        return false;

    auto curState = this->state();

    if (state == curState)
        return false;

    if (state == AkElement::ElementStatePlaying) {
        this->d->m_id = Ak::id();
        this->d->start();
    } else if (curState == AkElement::ElementStatePlaying) {
        this->d->stop();
    }

    return AkElement::setState(state);
}

QString TextSrcElement::text() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_text;
}

TextSrcElement::TextSource TextSrcElement::textSource() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_textSource;
}

QString TextSrcElement::textFile() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_textFile;
}

QString TextSrcElement::fontFamily() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.fontFamily;
}

int TextSrcElement::fontSize() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.fontSize;
}

bool TextSrcElement::bold() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.bold;
}

bool TextSrcElement::italic() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.italic;
}

bool TextSrcElement::underline() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.underline;
}

bool TextSrcElement::strikeout() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.strikeout;
}

qreal TextSrcElement::letterSpacing() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.letterSpacing;
}

qreal TextSrcElement::lineSpacing() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.lineSpacing;
}

bool TextSrcElement::antialiasing() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.antialiasing;
}

TextSrcElement::HorizontalAlignment TextSrcElement::horizontalAlignment() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return static_cast<TextSrcElement::HorizontalAlignment>(this->d->m_params.horizontalAlignment);
}

TextSrcElement::VerticalAlignment TextSrcElement::verticalAlignment() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return static_cast<TextSrcElement::VerticalAlignment>(this->d->m_params.verticalAlignment);
}

bool TextSrcElement::wordWrap() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.wordWrap;
}

int TextSrcElement::padding() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.padding;
}

int TextSrcElement::frameWidth() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.width;
}

int TextSrcElement::frameHeight() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.height;
}

QRgb TextSrcElement::foregroundColor() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.foregroundColor;
}

QRgb TextSrcElement::backgroundColor() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundColor;
}

TextSrcElement::BackgroundShape TextSrcElement::backgroundShape() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return static_cast<TextSrcElement::BackgroundShape>(this->d->m_params.backgroundShape);
}

int TextSrcElement::backgroundRadius() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundRadius;
}

bool TextSrcElement::backgroundBorderEnabled() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundBorderEnabled;
}

QRgb TextSrcElement::backgroundBorderColor() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundBorderColor;
}

int TextSrcElement::backgroundBorderWidth() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundBorderWidth;
}

bool TextSrcElement::backgroundShadowEnabled() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundShadowEnabled;
}

QRgb TextSrcElement::backgroundShadowColor() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundShadowColor;
}

int TextSrcElement::backgroundShadowOffsetX() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundShadowOffsetX;
}

int TextSrcElement::backgroundShadowOffsetY() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundShadowOffsetY;
}

int TextSrcElement::backgroundShadowBlur() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.backgroundShadowBlur;
}

TextSrcElement::TailStyle TextSrcElement::tailStyle() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return static_cast<TextSrcElement::TailStyle>(this->d->m_params.tailStyle);
}

int TextSrcElement::tailAngle() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.tailAngle;
}

int TextSrcElement::tailLength() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.tailLength;
}

int TextSrcElement::tailWidth() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.tailWidth;
}

bool TextSrcElement::fitBackground() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.fitBackground;
}

bool TextSrcElement::outlineEnabled() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.outlineEnabled;
}

QRgb TextSrcElement::outlineColor() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.outlineColor;
}

int TextSrcElement::outlineWidth() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.outlineWidth;
}

bool TextSrcElement::shadowEnabled() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.shadowEnabled;
}

QRgb TextSrcElement::shadowColor() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.shadowColor;
}

int TextSrcElement::shadowOffsetX() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.shadowOffsetX;
}

int TextSrcElement::shadowOffsetY() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.shadowOffsetY;
}

int TextSrcElement::shadowBlur() const
{
    QMutexLocker locker(&this->d->m_mutex);

    return this->d->m_params.shadowBlur;
}

void TextSrcElement::setText(const QString &text)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_text == text)
            return;

        this->d->m_text = text;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->textChanged(text);
    this->d->updateSummary();
}

void TextSrcElement::setTextSource(const TextSource &textSource)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_textSource == textSource)
            return;

        this->d->m_textSource = textSource;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->textSourceChanged(textSource);
    this->d->reloadFile();
    this->d->updateWatcher();
    this->d->updateSummary();
}

void TextSrcElement::setTextFile(const QString &textFile)
{
    auto value = textFile;

    if (value.startsWith("file:"))
        value = QUrl(value).toLocalFile();

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_textFile == value)
            return;

        this->d->m_textFile = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->textFileChanged(value);
    this->d->reloadFile();
    this->d->updateWatcher();
    this->d->updateSummary();
}

void TextSrcElement::setFontFamily(const QString &fontFamily)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.fontFamily == fontFamily)
            return;

        this->d->m_params.fontFamily = fontFamily;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->fontFamilyChanged(fontFamily);
}

void TextSrcElement::setFontSize(int fontSize)
{
    auto value = qBound(1, fontSize, 2048);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.fontSize == value)
            return;

        this->d->m_params.fontSize = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->fontSizeChanged(value);
}

void TextSrcElement::setBold(bool bold)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.bold == bold)
            return;

        this->d->m_params.bold = bold;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->boldChanged(bold);
}

void TextSrcElement::setItalic(bool italic)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.italic == italic)
            return;

        this->d->m_params.italic = italic;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->italicChanged(italic);
}

void TextSrcElement::setUnderline(bool underline)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.underline == underline)
            return;

        this->d->m_params.underline = underline;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->underlineChanged(underline);
}

void TextSrcElement::setStrikeout(bool strikeout)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.strikeout == strikeout)
            return;

        this->d->m_params.strikeout = strikeout;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->strikeoutChanged(strikeout);
}

void TextSrcElement::setLetterSpacing(qreal letterSpacing)
{
    auto value = qBound(-100.0, letterSpacing, 500.0);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.letterSpacing == value)
            return;

        this->d->m_params.letterSpacing = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->letterSpacingChanged(value);
}

void TextSrcElement::setLineSpacing(qreal lineSpacing)
{
    auto value = qBound(10.0, lineSpacing, 1000.0);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.lineSpacing == value)
            return;

        this->d->m_params.lineSpacing = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->lineSpacingChanged(value);
}

void TextSrcElement::setAntialiasing(bool antialiasing)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.antialiasing == antialiasing)
            return;

        this->d->m_params.antialiasing = antialiasing;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->antialiasingChanged(antialiasing);
}

void TextSrcElement::setHorizontalAlignment(const HorizontalAlignment &horizontalAlignment)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.horizontalAlignment == static_cast<TextRenderer::HorizontalAlignment>(horizontalAlignment))
            return;

        this->d->m_params.horizontalAlignment = static_cast<TextRenderer::HorizontalAlignment>(horizontalAlignment);
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->horizontalAlignmentChanged(horizontalAlignment);
}

void TextSrcElement::setVerticalAlignment(const VerticalAlignment &verticalAlignment)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.verticalAlignment == static_cast<TextRenderer::VerticalAlignment>(verticalAlignment))
            return;

        this->d->m_params.verticalAlignment = static_cast<TextRenderer::VerticalAlignment>(verticalAlignment);
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->verticalAlignmentChanged(verticalAlignment);
}

void TextSrcElement::setWordWrap(bool wordWrap)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.wordWrap == wordWrap)
            return;

        this->d->m_params.wordWrap = wordWrap;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->wordWrapChanged(wordWrap);
}

void TextSrcElement::setPadding(int padding)
{
    auto value = qBound(0, padding, 2048);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.padding == value)
            return;

        this->d->m_params.padding = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->paddingChanged(value);
}

void TextSrcElement::setFrameWidth(int frameWidth)
{
    auto value = qBound(16, frameWidth, 4096);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.width == value)
            return;

        this->d->m_params.width = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->frameWidthChanged(value);
    emit this->streamsChanged(this->streams());
}

void TextSrcElement::setFrameHeight(int frameHeight)
{
    auto value = qBound(16, frameHeight, 4096);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.height == value)
            return;

        this->d->m_params.height = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->frameHeightChanged(value);
    emit this->streamsChanged(this->streams());
}

void TextSrcElement::setForegroundColor(QRgb foregroundColor)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.foregroundColor == foregroundColor)
            return;

        this->d->m_params.foregroundColor = foregroundColor;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->foregroundColorChanged(foregroundColor);
}

void TextSrcElement::setBackgroundColor(QRgb backgroundColor)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundColor == backgroundColor)
            return;

        this->d->m_params.backgroundColor = backgroundColor;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundColorChanged(backgroundColor);
}

void TextSrcElement::setBackgroundShape(const BackgroundShape &backgroundShape)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundShape == static_cast<TextRenderer::BackgroundShape>(backgroundShape))
            return;

        this->d->m_params.backgroundShape = static_cast<TextRenderer::BackgroundShape>(backgroundShape);
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundShapeChanged(backgroundShape);
}

void TextSrcElement::setBackgroundRadius(int backgroundRadius)
{
    auto value = qBound(0, backgroundRadius, 2048);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundRadius == value)
            return;

        this->d->m_params.backgroundRadius = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundRadiusChanged(value);
}

void TextSrcElement::setBackgroundBorderEnabled(bool backgroundBorderEnabled)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundBorderEnabled == backgroundBorderEnabled)
            return;

        this->d->m_params.backgroundBorderEnabled = backgroundBorderEnabled;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundBorderEnabledChanged(backgroundBorderEnabled);
}

void TextSrcElement::setBackgroundBorderColor(QRgb backgroundBorderColor)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundBorderColor == backgroundBorderColor)
            return;

        this->d->m_params.backgroundBorderColor = backgroundBorderColor;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundBorderColorChanged(backgroundBorderColor);
}

void TextSrcElement::setBackgroundBorderWidth(int backgroundBorderWidth)
{
    auto value = qBound(0, backgroundBorderWidth, 256);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundBorderWidth == value)
            return;

        this->d->m_params.backgroundBorderWidth = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundBorderWidthChanged(value);
}

void TextSrcElement::setBackgroundShadowEnabled(bool backgroundShadowEnabled)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundShadowEnabled == backgroundShadowEnabled)
            return;

        this->d->m_params.backgroundShadowEnabled = backgroundShadowEnabled;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundShadowEnabledChanged(backgroundShadowEnabled);
}

void TextSrcElement::setBackgroundShadowColor(QRgb backgroundShadowColor)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundShadowColor == backgroundShadowColor)
            return;

        this->d->m_params.backgroundShadowColor = backgroundShadowColor;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundShadowColorChanged(backgroundShadowColor);
}

void TextSrcElement::setBackgroundShadowOffsetX(int backgroundShadowOffsetX)
{
    auto value = qBound(-512, backgroundShadowOffsetX, 512);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundShadowOffsetX == value)
            return;

        this->d->m_params.backgroundShadowOffsetX = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundShadowOffsetXChanged(value);
}

void TextSrcElement::setBackgroundShadowOffsetY(int backgroundShadowOffsetY)
{
    auto value = qBound(-512, backgroundShadowOffsetY, 512);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundShadowOffsetY == value)
            return;

        this->d->m_params.backgroundShadowOffsetY = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundShadowOffsetYChanged(value);
}

void TextSrcElement::setBackgroundShadowBlur(int backgroundShadowBlur)
{
    auto value = qBound(0, backgroundShadowBlur, 256);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.backgroundShadowBlur == value)
            return;

        this->d->m_params.backgroundShadowBlur = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->backgroundShadowBlurChanged(value);
}

void TextSrcElement::setTailStyle(const TailStyle &tailStyle)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.tailStyle == static_cast<TextRenderer::TailStyle>(tailStyle))
            return;

        this->d->m_params.tailStyle = static_cast<TextRenderer::TailStyle>(tailStyle);
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->tailStyleChanged(tailStyle);
}

void TextSrcElement::setTailAngle(int tailAngle)
{
    auto value = qBound(0, tailAngle, 360);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.tailAngle == value)
            return;

        this->d->m_params.tailAngle = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->tailAngleChanged(value);
}

void TextSrcElement::setTailLength(int tailLength)
{
    auto value = qBound(0, tailLength, 1024);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.tailLength == value)
            return;

        this->d->m_params.tailLength = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->tailLengthChanged(value);
}

void TextSrcElement::setTailWidth(int tailWidth)
{
    auto value = qBound(1, tailWidth, 1024);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.tailWidth == value)
            return;

        this->d->m_params.tailWidth = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->tailWidthChanged(value);
}

void TextSrcElement::setFitBackground(bool fitBackground)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.fitBackground == fitBackground)
            return;

        this->d->m_params.fitBackground = fitBackground;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->fitBackgroundChanged(fitBackground);
}

void TextSrcElement::setOutlineEnabled(bool outlineEnabled)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.outlineEnabled == outlineEnabled)
            return;

        this->d->m_params.outlineEnabled = outlineEnabled;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->outlineEnabledChanged(outlineEnabled);
}

void TextSrcElement::setOutlineColor(QRgb outlineColor)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.outlineColor == outlineColor)
            return;

        this->d->m_params.outlineColor = outlineColor;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->outlineColorChanged(outlineColor);
}

void TextSrcElement::setOutlineWidth(int outlineWidth)
{
    auto value = qBound(0, outlineWidth, 256);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.outlineWidth == value)
            return;

        this->d->m_params.outlineWidth = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->outlineWidthChanged(value);
}

void TextSrcElement::setShadowEnabled(bool shadowEnabled)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.shadowEnabled == shadowEnabled)
            return;

        this->d->m_params.shadowEnabled = shadowEnabled;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->shadowEnabledChanged(shadowEnabled);
}

void TextSrcElement::setShadowColor(QRgb shadowColor)
{
    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.shadowColor == shadowColor)
            return;

        this->d->m_params.shadowColor = shadowColor;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->shadowColorChanged(shadowColor);
}

void TextSrcElement::setShadowOffsetX(int shadowOffsetX)
{
    auto value = qBound(-512, shadowOffsetX, 512);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.shadowOffsetX == value)
            return;

        this->d->m_params.shadowOffsetX = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->shadowOffsetXChanged(value);
}

void TextSrcElement::setShadowOffsetY(int shadowOffsetY)
{
    auto value = qBound(-512, shadowOffsetY, 512);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.shadowOffsetY == value)
            return;

        this->d->m_params.shadowOffsetY = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->shadowOffsetYChanged(value);
}

void TextSrcElement::setShadowBlur(int shadowBlur)
{
    auto value = qBound(0, shadowBlur, 256);

    {
        QMutexLocker locker(&this->d->m_mutex);

        if (this->d->m_params.shadowBlur == value)
            return;

        this->d->m_params.shadowBlur = value;
        this->d->m_dirty = true;
    }

    this->d->m_cond.wakeAll();
    this->d->scheduleSave();
    emit this->shadowBlurChanged(value);
}

void TextSrcElement::resetText()
{
    this->setText(QString("Text"));
}

void TextSrcElement::resetTextSource()
{
    this->setTextSource(TextSourceInline);
}

void TextSrcElement::resetTextFile()
{
    this->setTextFile(QString());
}

void TextSrcElement::resetFontFamily()
{
    this->setFontFamily(QGuiApplication::font().family());
}

void TextSrcElement::resetFontSize()
{
    this->setFontSize(64);
}

void TextSrcElement::resetBold()
{
    this->setBold(false);
}

void TextSrcElement::resetItalic()
{
    this->setItalic(false);
}

void TextSrcElement::resetUnderline()
{
    this->setUnderline(false);
}

void TextSrcElement::resetStrikeout()
{
    this->setStrikeout(false);
}

void TextSrcElement::resetLetterSpacing()
{
    this->setLetterSpacing(0.0);
}

void TextSrcElement::resetLineSpacing()
{
    this->setLineSpacing(100.0);
}

void TextSrcElement::resetAntialiasing()
{
    this->setAntialiasing(true);
}

void TextSrcElement::resetHorizontalAlignment()
{
    this->setHorizontalAlignment(HorizontalAlignmentCenter);
}

void TextSrcElement::resetVerticalAlignment()
{
    this->setVerticalAlignment(VerticalAlignmentMiddle);
}

void TextSrcElement::resetWordWrap()
{
    this->setWordWrap(true);
}

void TextSrcElement::resetPadding()
{
    this->setPadding(16);
}

void TextSrcElement::resetFrameWidth()
{
    this->setFrameWidth(1280);
}

void TextSrcElement::resetFrameHeight()
{
    this->setFrameHeight(720);
}

void TextSrcElement::resetForegroundColor()
{
    this->setForegroundColor(qRgba(255, 255, 255, 255));
}

void TextSrcElement::resetBackgroundColor()
{
    this->setBackgroundColor(qRgba(0, 0, 0, 0));
}

void TextSrcElement::resetBackgroundShape()
{
    this->setBackgroundShape(BackgroundShapeRectangle);
}

void TextSrcElement::resetBackgroundRadius()
{
    this->setBackgroundRadius(0);
}

void TextSrcElement::resetBackgroundBorderEnabled()
{
    this->setBackgroundBorderEnabled(false);
}

void TextSrcElement::resetBackgroundBorderColor()
{
    this->setBackgroundBorderColor(qRgba(0, 0, 0, 255));
}

void TextSrcElement::resetBackgroundBorderWidth()
{
    this->setBackgroundBorderWidth(3);
}

void TextSrcElement::resetBackgroundShadowEnabled()
{
    this->setBackgroundShadowEnabled(false);
}

void TextSrcElement::resetBackgroundShadowColor()
{
    this->setBackgroundShadowColor(qRgba(0, 0, 0, 130));
}

void TextSrcElement::resetBackgroundShadowOffsetX()
{
    this->setBackgroundShadowOffsetX(5);
}

void TextSrcElement::resetBackgroundShadowOffsetY()
{
    this->setBackgroundShadowOffsetY(5);
}

void TextSrcElement::resetBackgroundShadowBlur()
{
    this->setBackgroundShadowBlur(8);
}

void TextSrcElement::resetTailStyle()
{
    this->setTailStyle(TailStyleNone);
}

void TextSrcElement::resetTailAngle()
{
    this->setTailAngle(100);
}

void TextSrcElement::resetTailLength()
{
    this->setTailLength(60);
}

void TextSrcElement::resetTailWidth()
{
    this->setTailWidth(50);
}

void TextSrcElement::resetFitBackground()
{
    this->setFitBackground(true);
}

void TextSrcElement::resetOutlineEnabled()
{
    this->setOutlineEnabled(false);
}

void TextSrcElement::resetOutlineColor()
{
    this->setOutlineColor(qRgba(0, 0, 0, 255));
}

void TextSrcElement::resetOutlineWidth()
{
    this->setOutlineWidth(3);
}

void TextSrcElement::resetShadowEnabled()
{
    this->setShadowEnabled(false);
}

void TextSrcElement::resetShadowColor()
{
    this->setShadowColor(qRgba(0, 0, 0, 160));
}

void TextSrcElement::resetShadowOffsetX()
{
    this->setShadowOffsetX(4);
}

void TextSrcElement::resetShadowOffsetY()
{
    this->setShadowOffsetY(4);
}

void TextSrcElement::resetShadowBlur()
{
    this->setShadowBlur(6);
}

TextSrcElementPrivate::TextSrcElementPrivate(TextSrcElement *self):
    self(self)
{
    this->m_params.fontFamily = QGuiApplication::font().family();
    this->m_threadPool.setMaxThreadCount(2);

    this->m_reloadTimer.setSingleShot(true);
    this->m_reloadTimer.setInterval(150);
    this->m_saveTimer.setSingleShot(true);
    this->m_saveTimer.setInterval(500);

    QObject::connect(&this->m_reloadTimer,
                     &QTimer::timeout,
                     [this] () {
                         this->reloadFile();
                         this->updateWatcher();
                     });
    QObject::connect(&this->m_saveTimer,
                     &QTimer::timeout,
                     [this] () {
                         this->saveSettings();
                     });
    QObject::connect(&this->m_watcher,
                     &QFileSystemWatcher::fileChanged,
                     [this] () {
                         this->m_reloadTimer.start();
                     });
    QObject::connect(&this->m_watcher,
                     &QFileSystemWatcher::directoryChanged,
                     [this] () {
                         this->m_reloadTimer.start();
                     });
}

QString TextSrcElementPrivate::settingsKey() const
{
    auto key = this->m_media;

    for (auto &c: key)
        if (!c.isLetterOrNumber() && c != '_' && c != '-')
            c = '_';

    return key;
}

void TextSrcElementPrivate::scheduleSave()
{
    if (this->m_loading || this->m_removed || this->m_media.isEmpty())
        return;

    this->m_saveTimer.start();
}

void TextSrcElementPrivate::saveSettings()
{
    if (this->m_removed || this->m_media.isEmpty())
        return;

    QSettings settings;
    settings.beginGroup("TextSrc");
    settings.beginGroup(this->settingsKey());
    settings.setValue("text", self->text());
    settings.setValue("textSource", int(self->textSource()));
    settings.setValue("textFile", self->textFile());
    settings.setValue("fontFamily", self->fontFamily());
    settings.setValue("fontSize", self->fontSize());
    settings.setValue("bold", self->bold());
    settings.setValue("italic", self->italic());
    settings.setValue("underline", self->underline());
    settings.setValue("strikeout", self->strikeout());
    settings.setValue("letterSpacing", self->letterSpacing());
    settings.setValue("lineSpacing", self->lineSpacing());
    settings.setValue("antialiasing", self->antialiasing());
    settings.setValue("horizontalAlignment", int(self->horizontalAlignment()));
    settings.setValue("verticalAlignment", int(self->verticalAlignment()));
    settings.setValue("wordWrap", self->wordWrap());
    settings.setValue("padding", self->padding());
    settings.setValue("frameWidth", self->frameWidth());
    settings.setValue("frameHeight", self->frameHeight());
    settings.setValue("foregroundColor", uint(self->foregroundColor()));
    settings.setValue("backgroundColor", uint(self->backgroundColor()));
    settings.setValue("backgroundShape", int(self->backgroundShape()));
    settings.setValue("backgroundRadius", self->backgroundRadius());
    settings.setValue("backgroundBorderEnabled", self->backgroundBorderEnabled());
    settings.setValue("backgroundBorderColor", uint(self->backgroundBorderColor()));
    settings.setValue("backgroundBorderWidth", self->backgroundBorderWidth());
    settings.setValue("backgroundShadowEnabled", self->backgroundShadowEnabled());
    settings.setValue("backgroundShadowColor", uint(self->backgroundShadowColor()));
    settings.setValue("backgroundShadowOffsetX", self->backgroundShadowOffsetX());
    settings.setValue("backgroundShadowOffsetY", self->backgroundShadowOffsetY());
    settings.setValue("backgroundShadowBlur", self->backgroundShadowBlur());
    settings.setValue("tailStyle", int(self->tailStyle()));
    settings.setValue("tailAngle", self->tailAngle());
    settings.setValue("tailLength", self->tailLength());
    settings.setValue("tailWidth", self->tailWidth());
    settings.setValue("fitBackground", self->fitBackground());
    settings.setValue("outlineEnabled", self->outlineEnabled());
    settings.setValue("outlineColor", uint(self->outlineColor()));
    settings.setValue("outlineWidth", self->outlineWidth());
    settings.setValue("shadowEnabled", self->shadowEnabled());
    settings.setValue("shadowColor", uint(self->shadowColor()));
    settings.setValue("shadowOffsetX", self->shadowOffsetX());
    settings.setValue("shadowOffsetY", self->shadowOffsetY());
    settings.setValue("shadowBlur", self->shadowBlur());
    settings.endGroup();
    settings.endGroup();
}

void TextSrcElementPrivate::loadSettings()
{
    QSettings settings;
    settings.beginGroup("TextSrc");
    settings.beginGroup(this->settingsKey());
    this->m_loading = true;

    self->setText(settings.value("text", QString("Text")).toString());
    self->setTextSource(static_cast<TextSrcElement::TextSource>(settings.value("textSource", int(TextSrcElement::TextSourceInline)).toInt()));
    self->setTextFile(settings.value("textFile", QString()).toString());
    self->setFontFamily(settings.value("fontFamily", QGuiApplication::font().family()).toString());
    self->setFontSize(settings.value("fontSize", 64).toInt());
    self->setBold(settings.value("bold", false).toBool());
    self->setItalic(settings.value("italic", false).toBool());
    self->setUnderline(settings.value("underline", false).toBool());
    self->setStrikeout(settings.value("strikeout", false).toBool());
    self->setLetterSpacing(settings.value("letterSpacing", 0.0).toReal());
    self->setLineSpacing(settings.value("lineSpacing", 100.0).toReal());
    self->setAntialiasing(settings.value("antialiasing", true).toBool());
    self->setHorizontalAlignment(static_cast<TextSrcElement::HorizontalAlignment>(settings.value("horizontalAlignment", int(TextSrcElement::HorizontalAlignmentCenter)).toInt()));
    self->setVerticalAlignment(static_cast<TextSrcElement::VerticalAlignment>(settings.value("verticalAlignment", int(TextSrcElement::VerticalAlignmentMiddle)).toInt()));
    self->setWordWrap(settings.value("wordWrap", true).toBool());
    self->setPadding(settings.value("padding", 16).toInt());
    self->setFrameWidth(settings.value("frameWidth", 1280).toInt());
    self->setFrameHeight(settings.value("frameHeight", 720).toInt());
    self->setForegroundColor(QRgb(settings.value("foregroundColor", uint(qRgba(255, 255, 255, 255))).toUInt()));
    self->setBackgroundColor(QRgb(settings.value("backgroundColor", uint(qRgba(0, 0, 0, 0))).toUInt()));
    self->setBackgroundShape(static_cast<TextSrcElement::BackgroundShape>(settings.value("backgroundShape", int(TextSrcElement::BackgroundShapeRectangle)).toInt()));
    self->setBackgroundRadius(settings.value("backgroundRadius", 0).toInt());
    self->setBackgroundBorderEnabled(settings.value("backgroundBorderEnabled", false).toBool());
    self->setBackgroundBorderColor(QRgb(settings.value("backgroundBorderColor", uint(qRgba(0, 0, 0, 255))).toUInt()));
    self->setBackgroundBorderWidth(settings.value("backgroundBorderWidth", 3).toInt());
    self->setBackgroundShadowEnabled(settings.value("backgroundShadowEnabled", false).toBool());
    self->setBackgroundShadowColor(QRgb(settings.value("backgroundShadowColor", uint(qRgba(0, 0, 0, 130))).toUInt()));
    self->setBackgroundShadowOffsetX(settings.value("backgroundShadowOffsetX", 5).toInt());
    self->setBackgroundShadowOffsetY(settings.value("backgroundShadowOffsetY", 5).toInt());
    self->setBackgroundShadowBlur(settings.value("backgroundShadowBlur", 8).toInt());
    self->setTailStyle(static_cast<TextSrcElement::TailStyle>(settings.value("tailStyle", int(TextSrcElement::TailStyleNone)).toInt()));
    self->setTailAngle(settings.value("tailAngle", 100).toInt());
    self->setTailLength(settings.value("tailLength", 60).toInt());
    self->setTailWidth(settings.value("tailWidth", 50).toInt());
    self->setFitBackground(settings.value("fitBackground", true).toBool());
    self->setOutlineEnabled(settings.value("outlineEnabled", false).toBool());
    self->setOutlineColor(QRgb(settings.value("outlineColor", uint(qRgba(0, 0, 0, 255))).toUInt()));
    self->setOutlineWidth(settings.value("outlineWidth", 3).toInt());
    self->setShadowEnabled(settings.value("shadowEnabled", false).toBool());
    self->setShadowColor(QRgb(settings.value("shadowColor", uint(qRgba(0, 0, 0, 160))).toUInt()));
    self->setShadowOffsetX(settings.value("shadowOffsetX", 4).toInt());
    self->setShadowOffsetY(settings.value("shadowOffsetY", 4).toInt());
    self->setShadowBlur(settings.value("shadowBlur", 6).toInt());

    this->m_loading = false;
    settings.endGroup();
    settings.endGroup();
}

QString TextSrcElementPrivate::readTextFile(const QString &path,
                                            QString *error)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly)) {
        *error = file.errorString();

        return {};
    }

    auto data = file.read(MAX_FILE_BYTES);
    QString text;
    auto encoding = QStringConverter::encodingForData(data);

    if (encoding) {
        QStringDecoder decoder(*encoding);
        text = decoder(data);
    } else {
        QStringDecoder decoder(QStringConverter::Utf8);
        text = decoder(data);

        // Not valid UTF-8, assume the system encoding (eg. ANSI on Windows).
        if (decoder.hasError()) {
            QStringDecoder systemDecoder(QStringConverter::System);
            text = systemDecoder(data);
        }
    }

    if (text.startsWith(QChar(0xfeff)))
        text.remove(0, 1);

    if (text.size() > MAX_TEXT_CHARS)
        text.truncate(MAX_TEXT_CHARS);

    // A trailing line break would add an empty line to the text block.
    while (text.endsWith('\n') || text.endsWith('\r'))
        text.chop(1);

    return text;
}

void TextSrcElementPrivate::reloadFile()
{
    QString path;

    {
        QMutexLocker locker(&this->m_mutex);

        if (this->m_textSource != TextSrcElement::TextSourceFile)
            return;

        path = this->m_textFile;
    }

    QString text;
    QString errorString;

    if (!path.isEmpty())
        text = this->readTextFile(path, &errorString);

    bool errorChanged = false;

    {
        QMutexLocker locker(&this->m_mutex);
        this->m_fileText = text;
        errorChanged = this->m_fileError != errorString;
        this->m_fileError = errorString;
        this->m_dirty = true;
    }

    this->m_cond.wakeAll();

    if (errorChanged) {
        emit self->fileErrorChanged(errorString);

        if (!errorString.isEmpty())
            emit self->error(errorString);
    }

    this->updateSummary();
}

/* A short, single line description of the text, used as the title of the
 * source when it has no description of its own.
 */
void TextSrcElementPrivate::updateSummary()
{
    QString text;
    QString file;

    {
        QMutexLocker locker(&this->m_mutex);
        text = this->m_textSource == TextSrcElement::TextSourceFile?
                   this->m_fileText:
                   this->m_text;
        file = this->m_textSource == TextSrcElement::TextSourceFile?
                   this->m_textFile:
                   QString();
    }

    QString summary;
    static const QRegularExpression lineBreak("[\\r\\n\\x{2028}\\x{2029}]");

    for (auto &line: text.split(lineBreak, Qt::SkipEmptyParts)) {
        summary = line.simplified();

        if (!summary.isEmpty())
            break;
    }

    if (summary.isEmpty())
        summary = file.isEmpty()? QString("Text"): QFileInfo(file).fileName();

    if (summary.size() > MAX_SUMMARY_CHARS)
        summary = summary.left(MAX_SUMMARY_CHARS - 1).trimmed() + QChar(0x2026);

    bool changed = false;

    {
        QMutexLocker locker(&this->m_mutex);
        changed = this->m_summary != summary;
        this->m_summary = summary;
    }

    if (changed)
        emit self->summaryChanged(summary);
}

void TextSrcElementPrivate::updateWatcher()
{
    auto files = this->m_watcher.files();
    auto dirs = this->m_watcher.directories();

    if (!files.isEmpty())
        this->m_watcher.removePaths(files);

    if (!dirs.isEmpty())
        this->m_watcher.removePaths(dirs);

    if (this->m_textSource != TextSrcElement::TextSourceFile
        || this->m_textFile.isEmpty())
        return;

    /* Many editors save by replacing the file, which drops it from the
     * watcher, so also watch the folder to catch that.
     */
    QFileInfo info(this->m_textFile);

    if (info.exists())
        this->m_watcher.addPath(info.absoluteFilePath());

    if (info.absoluteDir().exists())
        this->m_watcher.addPath(info.absolutePath());
}

void TextSrcElementPrivate::start()
{
    {
        QMutexLocker locker(&this->m_mutex);

        if (this->m_run)
            return;

        this->m_run = true;
    }

    this->m_thread = QtConcurrent::run(&this->m_threadPool,
                                       [this] () {
                                           this->run();
                                       });
}

void TextSrcElementPrivate::stop()
{
    {
        QMutexLocker locker(&this->m_mutex);

        if (!this->m_run)
            return;

        this->m_run = false;
    }

    this->m_cond.wakeAll();
    this->m_thread.waitForFinished();
}

void TextSrcElementPrivate::run()
{
    QElapsedTimer sinceLastFrame;
    bool sendNow = true;

    forever {
        TextRenderer::Params params;
        bool render = false;

        {
            QMutexLocker locker(&this->m_mutex);

            if (!this->m_run)
                break;

            if (this->m_dirty || this->m_image.isNull()) {
                params = this->m_params;
                params.text =
                    this->m_textSource == TextSrcElement::TextSourceFile?
                        this->m_fileText:
                        this->m_text;
                this->m_dirty = false;
                render = true;
            }
        }

        if (render) {
            auto image = TextRenderer::render(params);
            QMutexLocker locker(&this->m_mutex);
            this->m_image = image;
            sendNow = true;
        }

        QImage image;

        {
            QMutexLocker locker(&this->m_mutex);
            image = this->m_image;
        }

        /* The compositor keeps the last frame of every source, so a still
         * text only needs to be sent when it changes. A periodic refresh
         * is sent anyway to cover sources that get re-registered.
         */
        if (sendNow
            || !sinceLastFrame.isValid()
            || sinceLastFrame.elapsed() >= HEARTBEAT_MS) {
            this->sendFrame(image);
            sinceLastFrame.restart();
            sendNow = false;
        }

        QMutexLocker locker(&this->m_mutex);

        if (this->m_run && !this->m_dirty)
            this->m_cond.wait(&this->m_mutex, HEARTBEAT_MS / 2);
    }
}

void TextSrcElementPrivate::sendFrame(const QImage &image)
{
    if (image.isNull())
        return;

    AkVideoCaps caps(AkVideoCaps::Format_argbpack,
                     image.width(),
                     image.height(),
                     this->m_fps);
    AkVideoPacket packet(caps);
    auto iLineSize = size_t(image.bytesPerLine());
    auto oLineSize = packet.lineSize(0);
    auto height = image.height();

    if (iLineSize == oLineSize) {
        memcpy(packet.data(), image.constBits(), oLineSize * height);
    } else {
        auto lineSize = qMin(iLineSize, oLineSize);

        for (int y = 0; y < height; ++y)
            memcpy(packet.line(0, y), image.constScanLine(y), lineSize);
    }

    auto pts = qRound64(QTime::currentTime().msecsSinceStartOfDay()
                        * this->m_fps.value() / 1e3);
    packet.setPts(pts);
    packet.setDuration(1);
    packet.setTimeBase(this->m_fps.invert());
    packet.setIndex(0);
    packet.setId(this->m_id);

    emit self->oStream(packet);
}

#include "moc_textsrcelement.cpp"
