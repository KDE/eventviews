/*
  SPDX-FileCopyrightText: 2007 Loïc Corbasson <loic.corbasson@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "eventviews_export.h"

#include <CalendarSupport/Plugin>

#include <QUrl>

#include <QDate>
#include <QPixmap>

namespace EventViews
{
namespace CalendarDecoration
{
/**
  @class Element

  @brief Class for calendar decoration elements

  It provides entities like texts and pictures for a given date.
  Implementations can implement all functions or only a subset.
 */
class EVENTVIEWS_EXPORT Element : public QObject
{
    Q_OBJECT

public:
    using List = QList<Element *>;

    explicit Element(const QString &id);
    ~Element() override;

    /**
      Return a name for easy identification.
      This will be used for example for internal configuration (position, etc.),
      so don't i18n it and make it unique for your decoration.
    */
    Q_REQUIRED_RESULT virtual QString id() const;

    /**
      Description of element.
    */
    Q_REQUIRED_RESULT virtual QString elementInfo() const;

    /**
      Return a short text for a given date,
      usually only a few words.
    */
    Q_REQUIRED_RESULT virtual QString shortText();

    /**
      Return a long text for a given date.
      This text can be of any length,
      but usually it will have one or a few lines.

      Can for example be used as a tool tip.
    */
    Q_REQUIRED_RESULT virtual QString longText();

    /**
      Return an extensive text for a given date.
      This text can be of any length,
      but usually it will have one or a few paragraphs.
    */
    Q_REQUIRED_RESULT virtual QString extensiveText();

    /**
      Return a pixmap for a given date and a given size.
    */
    Q_REQUIRED_RESULT virtual QPixmap newPixmap(const QSize &);

    /**
      Return a URL pointing to more information about the content of the
      element.
     */
    Q_REQUIRED_RESULT virtual QUrl url();

Q_SIGNALS:
    void gotNewPixmap(const QPixmap &);
    void gotNewShortText(const QString &);
    void gotNewLongText(const QString &);
    void gotNewExtensiveText(const QString &);
    void gotNewUrl(const QUrl &);

protected:
    QString mId;
};

/**
  This class provides a stored element, which contains all data for the given
  date/month/year.
*/
class EVENTVIEWS_EXPORT StoredElement : public Element
{
    Q_OBJECT
public:
    explicit StoredElement(const QString &id);
    StoredElement(const QString &id, const QString &shortText);
    StoredElement(const QString &id, const QString &shortText, const QString &longText);
    StoredElement(const QString &id, const QString &shortText, const QString &longText, const QString &extensiveText);
    StoredElement(const QString &id, const QPixmap &pixmap);

    virtual void setShortText(const QString &t);
    QString shortText() override;

    virtual void setLongText(const QString &t);
    QString longText() override;

    virtual void setExtensiveText(const QString &t);
    QString extensiveText() override;

    virtual void setPixmap(const QPixmap &p);
    virtual QPixmap pixmap();

    virtual void setUrl(const QUrl &u);
    QUrl url() override;

protected:
    QString mShortText;
    QString mLongText;
    QString mExtensiveText;
    QPixmap mPixmap;
    QUrl mUrl;
};

/**
  @class Decoration

  @brief This class provides the interface for a date dependent decoration.

  The decoration is made of various decoration elements,
  which show a defined text/picture/widget for a given date.
 */
class EVENTVIEWS_EXPORT Decoration : public CalendarSupport::Plugin
{
public:
    static int interfaceVersion()
    {
        return 2;
    }

    static QString serviceType()
    {
        return QStringLiteral("Calendar/Decoration");
    }

    using List = QList<Decoration *>;

    Decoration();
    ~Decoration() override;

    /**
      Return all Elements for the given day.
    */
    virtual Element::List dayElements(const QDate &date);

    /**
      Return all elements for the week the given date belongs to.
    */
    virtual Element::List weekElements(const QDate &d);

    /**
      Return all elements for the month the given date belongs to.
    */
    virtual Element::List monthElements(const QDate &d);

    /**
      Return all elements for the year the given date belongs to.
    */
    virtual Element::List yearElements(const QDate &d);

protected:
    /**
      Register the given elements for the given date. They will be deleted when
      this object is destroyed.
    */
    Element::List registerDayElements(const Element::List &e, const QDate &d);

    /**
      Register the given elements for the week the given date belongs to. They
      will be deleted when this object is destroyed.
    */
    Element::List registerWeekElements(const Element::List &e, const QDate &d);

    /**
      Register the given elements for the month the given date belongs to. They
      will be deleted when this object is destroyed.
    */
    Element::List registerMonthElements(const Element::List &e, const QDate &d);

    /**
      Register the given elements for the year the given date belongs to. They
      will be deleted when this object is destroyed.
    */
    Element::List registerYearElements(const Element::List &e, const QDate &d);

    /**
      Create day elements for given date.
    */
    virtual Element::List createDayElements(const QDate &);

    /**
      Create elements for the week the given date belongs to.
    */
    virtual Element::List createWeekElements(const QDate &);

    /**
      Create elements for the month the given date belongs to.
    */
    virtual Element::List createMonthElements(const QDate &);

    /**
      Create elements for the year the given date belongs to.
    */
    virtual Element::List createYearElements(const QDate &);

    /**
      Map all dates of the same week to a single date.
    */
    QDate weekDate(QDate date);

    /**
      Map all dates of the same month to a single date.
    */
    QDate monthDate(QDate date);

    /**
      Map all dates of the same year to a single date.
    */
    QDate yearDate(QDate date);

private:
    QMap<QDate, Element::List> mDayElements;
    QMap<QDate, Element::List> mWeekElements;
    QMap<QDate, Element::List> mMonthElements;
    QMap<QDate, Element::List> mYearElements;
};

class EVENTVIEWS_EXPORT DecorationFactory : public CalendarSupport::PluginFactory
{
    Q_OBJECT
public:
    Decoration *createPluginFactory() override = 0;
};
}
}

