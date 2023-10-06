/*
  SPDX-FileCopyrightText: 2007 Loïc Corbasson <loic.corbasson@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "eventviews_export.h"

#include <QUrl>

#include <QDate>
#include <QMap>
#include <QPixmap>
#include <QVariant>

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
    [[nodiscard]] virtual QString id() const;

    /**
      Description of element.
    */
    [[nodiscard]] virtual QString elementInfo() const;

    /**
      Return a short text for a given date,
      usually only a few words.
    */
    [[nodiscard]] virtual QString shortText() const;

    /**
      Return a long text for a given date.
      This text can be of any length,
      but usually it will have one or a few lines.

      Can for example be used as a tool tip.
    */
    [[nodiscard]] virtual QString longText() const;

    /**
      Return an extensive text for a given date.
      This text can be of any length,
      but usually it will have one or a few paragraphs.
    */
    [[nodiscard]] virtual QString extensiveText() const;

    /**
      Return a pixmap for a given date and a given size.
    */
    [[nodiscard]] virtual QPixmap newPixmap(const QSize &);

    /**
      Return a URL pointing to more information about the content of the
      element.
     */
    [[nodiscard]] virtual QUrl url() const;

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
    [[nodiscard]] QString shortText() const override;

    virtual void setLongText(const QString &t);
    [[nodiscard]] QString longText() const override;

    virtual void setExtensiveText(const QString &t);
    [[nodiscard]] QString extensiveText() const override;

    virtual void setPixmap(const QPixmap &p);
    [[nodiscard]] virtual QPixmap pixmap() const;

    virtual void setUrl(const QUrl &u);
    [[nodiscard]] QUrl url() const override;

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
class EVENTVIEWS_EXPORT Decoration : public QObject
{
    Q_OBJECT

public:
    using List = QList<Decoration *>;

    Decoration(QObject *parent = nullptr, const QVariantList &args = {});
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

    virtual void configure(QWidget *);

    virtual QString info() const = 0;

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

}
}
