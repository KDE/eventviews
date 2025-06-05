/*
  SPDX-FileCopyrightText: 2007 Loïc Corbasson <loic.corbasson@gmail.com>

  SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "calendardecoration.h"

using namespace EventViews::CalendarDecoration;

Element::Element(const QString &id)
    : mId(id)
{
}

Element::~Element() = default;

QString Element::id() const
{
    return mId;
}

QString Element::elementInfo() const
{
    return {};
}

QString Element::shortText() const
{
    return {};
}

QString Element::longText() const
{
    return {};
}

QString Element::extensiveText() const
{
    return {};
}

QPixmap Element::newPixmap(const QSize &)
{
    return {};
}

QUrl Element::url() const
{
    return {};
}

////////////////////////////////////////////////////////////////////////////////

StoredElement::StoredElement(const QString &id)
    : Element(id)
{
}

StoredElement::StoredElement(const QString &id, const QString &shortText)
    : Element(id)
    , mShortText(shortText)
{
}

StoredElement::StoredElement(const QString &id, const QString &shortText, const QString &longText)
    : Element(id)
    , mShortText(shortText)
    , mLongText(longText)
{
}

StoredElement::StoredElement(const QString &id, const QString &shortText, const QString &longText, const QString &extensiveText)
    : Element(id)
    , mShortText(shortText)
    , mLongText(longText)
    , mExtensiveText(extensiveText)
{
}

StoredElement::StoredElement(const QString &id, const QPixmap &pixmap)
    : Element(id)
    , mPixmap(pixmap)
{
}

void StoredElement::setShortText(const QString &t)
{
    mShortText = t;
}

QString StoredElement::shortText() const
{
    return mShortText;
}

void StoredElement::setLongText(const QString &t)
{
    mLongText = t;
}

QString StoredElement::longText() const
{
    return mLongText;
}

void StoredElement::setExtensiveText(const QString &t)
{
    mExtensiveText = t;
}

QString StoredElement::extensiveText() const
{
    return mExtensiveText;
}

void StoredElement::setPixmap(const QPixmap &p)
{
    mPixmap = p;
}

QPixmap StoredElement::pixmap() const
{
    return mPixmap;
}

void StoredElement::setUrl(const QUrl &u)
{
    mUrl = u;
}

QUrl StoredElement::url() const
{
    return mUrl;
}

////////////////////////////////////////////////////////////////////////////////

Decoration::Decoration(QObject *parent, const QVariantList &args)
    : QObject(parent)
{
    Q_UNUSED(args)
}

Decoration::~Decoration()
{
    // Deleted by label directly.
#if 0
    for (Element::List lst : std::as_const(mDayElements)) {
        qDeleteAll(lst);
        lst.clear();
    }
    for (Element::List lst : std::as_const(mWeekElements)) {
        qDeleteAll(lst);
        lst.clear();
    }
    for (Element::List lst : std::as_const(mMonthElements)) {
        qDeleteAll(lst);
        lst.clear();
    }
    for (Element::List lst : std::as_const(mYearElements)) {
        qDeleteAll(lst);
        lst.clear();
    }
#endif
    mDayElements.clear();
    mWeekElements.clear();
    mMonthElements.clear();
    mYearElements.clear();
}

Element::List Decoration::dayElements(const QDate &date)
{
    QMap<QDate, Element::List>::ConstIterator it;
    it = mDayElements.constFind(date);
    if (it == mDayElements.constEnd()) {
        return registerDayElements(createDayElements(date), date);
    } else {
        return *it;
    }
}

Element::List Decoration::weekElements(const QDate &d)
{
    QDate const date = weekDate(d);
    QMap<QDate, Element::List>::ConstIterator it;
    it = mWeekElements.constFind(date);
    if (it == mWeekElements.constEnd()) {
        return registerWeekElements(createWeekElements(date), date);
    } else {
        return *it;
    }
}

Element::List Decoration::monthElements(const QDate &d)
{
    QDate const date = monthDate(d);
    QMap<QDate, Element::List>::ConstIterator it;
    it = mMonthElements.constFind(date);
    if (it == mMonthElements.constEnd()) {
        return registerMonthElements(createMonthElements(date), date);
    } else {
        return *it;
    }
}

Element::List Decoration::yearElements(const QDate &d)
{
    QDate const date = yearDate(d);
    QMap<QDate, Element::List>::ConstIterator it;
    it = mYearElements.constFind(date);
    if (it == mYearElements.constEnd()) {
        return registerYearElements(createYearElements(date), date);
    } else {
        return *it;
    }
}

Element::List Decoration::registerDayElements(const Element::List &e, const QDate &d)
{
    mDayElements.insert(d, e);
    return e;
}

Element::List Decoration::registerWeekElements(const Element::List &e, const QDate &d)
{
    mWeekElements.insert(weekDate(d), e);
    return e;
}

Element::List Decoration::registerMonthElements(const Element::List &e, const QDate &d)
{
    mMonthElements.insert(monthDate(d), e);
    return e;
}

Element::List Decoration::registerYearElements(const Element::List &e, const QDate &d)
{
    mYearElements.insert(yearDate(d), e);
    return e;
}

Element::List Decoration::createDayElements(const QDate &)
{
    return {};
}

Element::List Decoration::createWeekElements(const QDate &)
{
    return {};
}

Element::List Decoration::createMonthElements(const QDate &)
{
    return {};
}

Element::List Decoration::createYearElements(const QDate &)
{
    return {};
}

void Decoration::configure(QWidget *)
{
}

QDate Decoration::weekDate(QDate date)
{
    QDate const result = date;
    return result.addDays(date.dayOfWeek() - 1);
}

QDate Decoration::monthDate(QDate date)
{
    return {date.year(), date.month(), 1};
}

QDate Decoration::yearDate(QDate date)
{
    return {date.year(), 1, 1};
}

#include "moc_calendardecoration.cpp"
