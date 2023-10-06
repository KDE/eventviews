/*
  SPDX-FileCopyrightText: 2000, 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include <QLabel>

namespace EventViews
{
class AlternateLabel : public QLabel
{
    Q_OBJECT
public:
    AlternateLabel(const QString &shortlabel, const QString &longlabel, const QString &extensivelabel = QString(), QWidget *parent = nullptr);
    ~AlternateLabel() override;

    enum TextType { Short = 0, Long = 1, Extensive = 2 };

    [[nodiscard]] TextType largestFittingTextType() const;
    void setFixedType(TextType type);

public Q_SLOTS:
    void useShortText();
    void useLongText();
    void useExtensiveText();
    void useDefaultText();

protected:
    void resizeEvent(QResizeEvent *) override;
    virtual void squeezeTextToLabel();
    bool mTextTypeFixed = false;
    const QString mShortText;
    const QString mLongText;
    QString mExtensiveText;

private:
    int getIndent() const;
};
}
