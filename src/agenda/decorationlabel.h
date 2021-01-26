/*
  SPDX-FileCopyrightText: 2000, 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2007 Loïc Corbasson <loic.corbasson@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#ifndef EVENTVIEWS_DECORATIONLABEL_H
#define EVENTVIEWS_DECORATIONLABEL_H

#include "calendardecoration.h"

#include <QLabel>

namespace EventViews
{
class DecorationLabel : public QLabel
{
    Q_OBJECT
public:
    explicit DecorationLabel(EventViews::CalendarDecoration::Element *e, QWidget *parent = nullptr);

    explicit DecorationLabel(const QString &shortText,
                             const QString &longText = QString(),
                             const QString &extensiveText = QString(),
                             const QPixmap &pixmap = QPixmap(),
                             const QUrl &url = QUrl(),
                             QWidget *parent = nullptr);
    ~DecorationLabel() override;

public Q_SLOTS:
    void setExtensiveText(const QString &);
    void setLongText(const QString &);
    void setPixmap(const QPixmap &);
    void setShortText(const QString &);
    void setText(const QString &);
    void setUrl(const QUrl &);
    void useShortText(bool allowAutomaticSqueeze = false);
    void useLongText(bool allowAutomaticSqueeze = false);
    void useExtensiveText(bool allowAutomaticSqueeze = false);
    void usePixmap(bool allowAutomaticSqueeze = false);
    void useDefaultText();

protected:
    void resizeEvent(QResizeEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    virtual void squeezeContentsToLabel();
    bool mAutomaticSqueeze = true;
    EventViews::CalendarDecoration::Element *mDecorationElement = nullptr;
    QString mShortText;
    QString mLongText;
    QString mExtensiveText;
    QPixmap mPixmap;
    QUrl mUrl;
};
}

#endif
