/*
  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#include "alternatelabel.h"

using namespace EventViews;

AlternateLabel::AlternateLabel(const QString &shortlabel, const QString &longlabel, const QString &extensivelabel, QWidget *parent)
    : QLabel(parent)
    , mTextTypeFixed(false)
    , mShortText(shortlabel)
    , mLongText(longlabel)
    , mExtensiveText(extensivelabel)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    if (mExtensiveText.isEmpty()) {
        mExtensiveText = mLongText;
    }
    const QFontMetrics &fm = fontMetrics();
    // We use at least averageCharWidth * 2 here to avoid misalignment
    // for single char labels.
    setMinimumWidth(qMax(fm.averageCharWidth() * 2, fm.boundingRect(shortlabel).width()) + getIndent());

    squeezeTextToLabel();
}

AlternateLabel::~AlternateLabel()
{
}

void AlternateLabel::useShortText()
{
    mTextTypeFixed = true;
    QLabel::setText(mShortText);
    setToolTip(mExtensiveText);
}

void AlternateLabel::useLongText()
{
    mTextTypeFixed = true;
    QLabel::setText(mLongText);
    this->setToolTip(mExtensiveText);
}

void AlternateLabel::useExtensiveText()
{
    mTextTypeFixed = true;
    QLabel::setText(mExtensiveText);
    this->setToolTip(QString());
}

void AlternateLabel::useDefaultText()
{
    mTextTypeFixed = false;
    squeezeTextToLabel();
}

void AlternateLabel::squeezeTextToLabel()
{
    if (mTextTypeFixed) {
        return;
    }

    QFontMetrics fm(fontMetrics());
    int labelWidth = size().width() - getIndent();
    int textWidth = fm.boundingRect(mLongText).width();
    int longTextWidth = fm.boundingRect(mExtensiveText).width();
    if (longTextWidth <= labelWidth) {
        QLabel::setText(mExtensiveText);
        this->setToolTip(QString());
    } else if (textWidth <= labelWidth) {
        QLabel::setText(mLongText);
        this->setToolTip(mExtensiveText);
    } else {
        QLabel::setText(mShortText);
        this->setToolTip(mExtensiveText);
    }
}

void AlternateLabel::resizeEvent(QResizeEvent *)
{
    squeezeTextToLabel();
}

AlternateLabel::TextType AlternateLabel::largestFittingTextType() const
{
    QFontMetrics fm(fontMetrics());
    const int labelWidth = size().width() - getIndent();
    const int longTextWidth = fm.boundingRect(mLongText).width();
    const int extensiveTextWidth = fm.boundingRect(mExtensiveText).width();
    if (extensiveTextWidth <= labelWidth) {
        return Extensive;
    } else if (longTextWidth <= labelWidth) {
        return Long;
    } else {
        return Short;
    }
}

void AlternateLabel::setFixedType(TextType type)
{
    switch (type) {
    case Extensive:
        useExtensiveText();
        break;
    case Long:
        useLongText();
        break;
    case Short:
        useShortText();
        break;
    }
}

int AlternateLabel::getIndent() const
{
    return indent() == -1 ? fontMetrics().boundingRect(QStringLiteral("x")).width() / 2 : indent();
}
