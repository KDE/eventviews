/*
  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2007 Loïc Corbasson <loic.corbasson@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#include "decorationlabel.h"

#include <QDesktopServices>

#include <QMouseEvent>
#include <QResizeEvent>

using namespace EventViews;

DecorationLabel::DecorationLabel(CalendarDecoration::Element *e, QWidget *parent)
    : QLabel(parent)
    , mDecorationElement(e)
    , mShortText(e->shortText())
    , mLongText(e->longText())
    , mExtensiveText(e->extensiveText())
    , mPixmap(e->newPixmap(size()))
    , mUrl(e->url())
{
    setUrl(mUrl);

    connect(e, &CalendarDecoration::Element::gotNewExtensiveText, this, &DecorationLabel::setExtensiveText);
    connect(e, &CalendarDecoration::Element::gotNewLongText, this, &DecorationLabel::setLongText);
    connect(e, &CalendarDecoration::Element::gotNewPixmap, this, &DecorationLabel::setPixmap);
    connect(e, &CalendarDecoration::Element::gotNewShortText, this, &DecorationLabel::setShortText);
    connect(e, &CalendarDecoration::Element::gotNewUrl, this, &DecorationLabel::setUrl);
    squeezeContentsToLabel();
}

DecorationLabel::DecorationLabel(const QString &shortText,
                                 const QString &longText,
                                 const QString &extensiveText,
                                 const QPixmap &pixmap,
                                 const QUrl &url,
                                 QWidget *parent)
    : QLabel(parent)
    , mShortText(shortText)
    , mLongText(longText)
    , mExtensiveText(extensiveText)
    , mPixmap(pixmap)
{
    setUrl(url);

    squeezeContentsToLabel();
}

DecorationLabel::~DecorationLabel()
{
    delete mDecorationElement;
}

void DecorationLabel::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);

    switch (event->button()) {
    case Qt::LeftButton:
        if (!mUrl.isEmpty()) {
            QDesktopServices::openUrl(mUrl);
            setForegroundRole(QPalette::LinkVisited);
        }
        break;
    case Qt::MiddleButton:
    case Qt::RightButton:
    default:
        break;
    }
}

void DecorationLabel::resizeEvent(QResizeEvent *event)
{
    mPixmap = mDecorationElement->newPixmap(event->size());
    QLabel::resizeEvent(event);
    squeezeContentsToLabel();
}

void DecorationLabel::setExtensiveText(const QString &text)
{
    mExtensiveText = text;
    squeezeContentsToLabel();
}

void DecorationLabel::setLongText(const QString &text)
{
    mLongText = text;
    squeezeContentsToLabel();
}

void DecorationLabel::setPixmap(const QPixmap &pixmap)
{
    mPixmap = pixmap.scaled(size(), Qt::KeepAspectRatio);
    squeezeContentsToLabel();
}

void DecorationLabel::setShortText(const QString &text)
{
    mShortText = text;
    squeezeContentsToLabel();
}

void DecorationLabel::setText(const QString &text)
{
    setLongText(text);
}

void DecorationLabel::setUrl(const QUrl &url)
{
    mUrl = url;
    QFont f = font();
    if (url.isEmpty()) {
        setForegroundRole(QPalette::WindowText);
        f.setUnderline(false);
#ifndef QT_NO_CURSOR
        setCursor(QCursor(Qt::ArrowCursor));
#endif
    } else {
        setForegroundRole(QPalette::Link);
        f.setUnderline(true);
#ifndef QT_NO_CURSOR
        setCursor(QCursor(Qt::PointingHandCursor));
#endif
    }
    setFont(f);
}

void DecorationLabel::squeezeContentsToLabel()
{
    if (!mAutomaticSqueeze) { // The content type to use has been set manually
        return;
    }

    QFontMetrics fm(fontMetrics());

    int labelWidth = size().width();
    int longTextWidth = fm.boundingRect(mLongText).width();
    int extensiveTextWidth = fm.boundingRect(mExtensiveText).width();

    if (!mPixmap.isNull()) {
        usePixmap(true);
    } else if ((!mExtensiveText.isEmpty()) && (extensiveTextWidth <= labelWidth)) {
        useExtensiveText(true);
    } else if ((!mLongText.isEmpty()) && (longTextWidth <= labelWidth)) {
        useLongText(true);
    } else {
        useShortText(true);
    }

    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    QSize msh = QLabel::minimumSizeHint();
    msh.setHeight(fontMetrics().lineSpacing());
    msh.setWidth(0);
    setMinimumSize(msh);
    setSizePolicy(sizePolicy().horizontalPolicy(), QSizePolicy::MinimumExpanding);
}

void DecorationLabel::useDefaultText()
{
    mAutomaticSqueeze = false;
    squeezeContentsToLabel();
}

void DecorationLabel::useExtensiveText(bool allowAutomaticSqueeze)
{
    mAutomaticSqueeze = allowAutomaticSqueeze;
    QLabel::setText(mExtensiveText);
    setToolTip(QString());
}

void DecorationLabel::useLongText(bool allowAutomaticSqueeze)
{
    mAutomaticSqueeze = allowAutomaticSqueeze;
    QLabel::setText(mLongText);
    setToolTip(mExtensiveText.isEmpty() ? QString() : mExtensiveText);
}

void DecorationLabel::usePixmap(bool allowAutomaticSqueeze)
{
    mAutomaticSqueeze = allowAutomaticSqueeze;
    QLabel::setPixmap(mPixmap);
    setToolTip(mExtensiveText.isEmpty() ? mLongText : mExtensiveText);
}

void DecorationLabel::useShortText(bool allowAutomaticSqueeze)
{
    mAutomaticSqueeze = allowAutomaticSqueeze;
    QLabel::setText(mShortText);
    setToolTip(mExtensiveText.isEmpty() ? mLongText : mExtensiveText);
}
