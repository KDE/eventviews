/*
  SPDX-FileCopyrightText: 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>
  SPDX-FileContributor: Sergio Martins <sergio@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#include "agendaview.h"
#include "agenda.h"
#include "agendaitem.h"
#include "alternatelabel.h"
#include "calendardecoration.h"
#include "decorationlabel.h"
#include "prefs.h"
#include "timelabels.h"
#include "timelabelszone.h"

#include "calendarview_debug.h"

#include <Akonadi/CalendarUtils>
#include <Akonadi/ETMCalendar>
#include <Akonadi/IncidenceChanger>
#include <CalendarSupport/CollectionSelection>
#include <CalendarSupport/KCalPrefs>
#include <CalendarSupport/Utils>

#include <KCalendarCore/CalFilter>
#include <KCalendarCore/CalFormat>
#include <KCalendarCore/OccurrenceIterator>

#include <KIconLoader> // for SmallIcon()
#include <KMessageBox>
#include <KPluginFactory>
#include <KSqueezedTextLabel>

#include <KLocalizedString>
#include <QApplication>
#include <QDrag>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollBar>
#include <QSplitter>
#include <QStyle>
#include <QTimer>

#include <chrono>
#include <vector>

using namespace std::chrono_literals;

using namespace EventViews;

enum { SPACING = 2 };
enum {
    SHRINKDOWN = 2 // points less for the timezone font
};

// Layout which places the widgets in equally sized columns,
// matching the calculation of the columns in the agenda.
class AgendaHeaderLayout : public QLayout
{
public:
    explicit AgendaHeaderLayout(QWidget *parent);
    ~AgendaHeaderLayout() override;

public: // QLayout API
    int count() const override;
    QLayoutItem *itemAt(int index) const override;

    void addItem(QLayoutItem *item) override;
    QLayoutItem *takeAt(int index) override;

public: // QLayoutItem API
    QSize sizeHint() const override;
    QSize minimumSize() const override;

    void invalidate() override;
    void setGeometry(const QRect &rect) override;

private:
    void updateCache() const;

private:
    QList<QLayoutItem *> mItems;

    mutable bool mIsDirty = false;
    mutable QSize mSizeHint;
    mutable QSize mMinSize;
};

AgendaHeaderLayout::AgendaHeaderLayout(QWidget *parent)
    : QLayout(parent)
{
}

AgendaHeaderLayout::~AgendaHeaderLayout()
{
    while (!mItems.isEmpty()) {
        delete mItems.takeFirst();
    }
}

void AgendaHeaderLayout::addItem(QLayoutItem *item)
{
    mItems.append(item);
    invalidate();
}

int AgendaHeaderLayout::count() const
{
    return mItems.size();
}

QLayoutItem *AgendaHeaderLayout::itemAt(int index) const
{
    return mItems.value(index);
}

QLayoutItem *AgendaHeaderLayout::takeAt(int index)
{
    if (index < 0 || index >= mItems.size()) {
        return nullptr;
    }

    auto item = mItems.takeAt(index);
    if (item) {
        invalidate();
    }
    return item;
}

void AgendaHeaderLayout::invalidate()
{
    QLayout::invalidate();
    mIsDirty = true;
}

void AgendaHeaderLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);

    if (mItems.isEmpty()) {
        return;
    }

    const QMargins margins = contentsMargins();

    // same logic as Agenda uses to distribute the width
    const int contentWidth = rect.width() - margins.left() - margins.right();
    const double agendaGridSpacingX = static_cast<double>(contentWidth) / mItems.size();
    int x = margins.left();
    const int contentHeight = rect.height() - margins.top() - margins.bottom();
    const int y = rect.y() + margins.top();
    for (int i = 0; i < mItems.size(); ++i) {
        auto item = mItems.at(i);
        const int nextX = margins.left() + static_cast<int>((i + 1) * agendaGridSpacingX);
        const int width = nextX - x;
        item->setGeometry(QRect(x, y, width, contentHeight));
        x = nextX;
    }
}

QSize AgendaHeaderLayout::sizeHint() const
{
    if (mIsDirty) {
        updateCache();
    }
    return mSizeHint;
}

QSize AgendaHeaderLayout::minimumSize() const
{
    if (mIsDirty) {
        updateCache();
    }
    return mMinSize;
}

void AgendaHeaderLayout::updateCache() const
{
    QSize maxItemSizeHint(0, 0);
    QSize maxItemMinSize(0, 0);
    for (auto &item : mItems) {
        maxItemSizeHint = maxItemSizeHint.expandedTo(item->sizeHint());
        maxItemMinSize = maxItemMinSize.expandedTo(item->minimumSize());
    }
    const QMargins margins = contentsMargins();
    const int horizontalMargins = margins.left() + margins.right();
    const int verticalMargins = margins.top() + margins.bottom();
    mSizeHint = QSize(maxItemSizeHint.width() * mItems.size() + horizontalMargins, maxItemSizeHint.height() + verticalMargins);
    mMinSize = QSize(maxItemMinSize.width() * mItems.size() + horizontalMargins, maxItemMinSize.height() + verticalMargins);
    mIsDirty = false;
}

// Header (or footer) for the agenda.
// Optionally has an additional week header, if isSideBySide is set
class AgendaHeader : public QWidget
{
    Q_OBJECT
public:
    explicit AgendaHeader(bool isSideBySide, QWidget *parent);

    using DecorationList = QList<EventViews::CalendarDecoration::Decoration *>;

public:
    void setAgenda(Agenda *agenda);
    bool createDayLabels(const KCalendarCore::DateList &dates, bool withDayLabel, const QStringList &decos, const QStringList &enabledDecos);
    void setWeekWidth(int width);
    void updateDayLabelSizes();
    void updateMargins();

protected:
    void resizeEvent(QResizeEvent *resizeEvent) override;

private:
    static CalendarDecoration::Decoration *loadCalendarDecoration(const QString &name);

    void addDay(const DecorationList &decoList, QDate date, bool withDayLabel);
    void clear();
    void placeDecorations(const DecorationList &decoList, QDate date, QWidget *labelBox, bool forWeek);
    void loadDecorations(const QStringList &decorations, const QStringList &whiteList, DecorationList &decoList);

private:
    const bool mIsSideBySide;

    Agenda *mAgenda = nullptr;
    QWidget *mDayLabels = nullptr;
    AgendaHeaderLayout *mDayLabelsLayout = nullptr;
    QWidget *mWeekLabelBox = nullptr;

    QList<AlternateLabel *> mDateDayLabels;
};

AgendaHeader::AgendaHeader(bool isSideBySide, QWidget *parent)
    : QWidget(parent)
    , mIsSideBySide(isSideBySide)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(SPACING);

    if (!mIsSideBySide) {
        mWeekLabelBox = new QWidget(this);
        auto weekLabelBoxLayout = new QVBoxLayout(mWeekLabelBox);
        weekLabelBoxLayout->setContentsMargins(0, 0, 0, 0);
        weekLabelBoxLayout->setSpacing(0);
        layout->addWidget(mWeekLabelBox);
    }

    mDayLabels = new QWidget(this);
    mDayLabelsLayout = new AgendaHeaderLayout(mDayLabels);
    mDayLabelsLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mDayLabels);
    layout->setStretchFactor(mDayLabels, 1);
}

void AgendaHeader::setAgenda(Agenda *agenda)
{
    mAgenda = agenda;
}

void AgendaHeader::updateMargins()
{
    const int frameWidth = mAgenda ? mAgenda->scrollArea()->frameWidth() : 0;
    const int scrollBarWidth = (mIsSideBySide || !mAgenda || !mAgenda->verticalScrollBar()->isVisible()) ? 0 : mAgenda->verticalScrollBar()->width();
    const bool isLTR = (layoutDirection() == Qt::LeftToRight);
    const int leftSpacing = SPACING + frameWidth;
    const int rightSpacing = scrollBarWidth + frameWidth;
    mDayLabelsLayout->setContentsMargins(isLTR ? leftSpacing : rightSpacing, 0, isLTR ? rightSpacing : leftSpacing, 0);
}

void AgendaHeader::updateDayLabelSizes()
{
    if (mDateDayLabels.isEmpty()) {
        return;
    }
    // First, calculate the maximum text type that fits for all labels
    AlternateLabel::TextType overallType = AlternateLabel::Extensive;
    for (auto label : std::as_const(mDateDayLabels)) {
        AlternateLabel::TextType type = label->largestFittingTextType();
        if (type < overallType) {
            overallType = type;
        }
    }

    // Then, set that maximum text type to all the labels
    for (auto label : std::as_const(mDateDayLabels)) {
        label->setFixedType(overallType);
    }
}

void AgendaHeader::resizeEvent(QResizeEvent *resizeEvent)
{
    QWidget::resizeEvent(resizeEvent);
    updateDayLabelSizes();
}

void AgendaHeader::setWeekWidth(int width)
{
    if (!mWeekLabelBox) {
        return;
    }

    mWeekLabelBox->setFixedWidth(width);
}

void AgendaHeader::clear()
{
    auto childWidgets = mDayLabels->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    qDeleteAll(childWidgets);
    if (mWeekLabelBox) {
        childWidgets = mWeekLabelBox->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
        qDeleteAll(childWidgets);
    }
    mDateDayLabels.clear();
}

bool AgendaHeader::createDayLabels(const KCalendarCore::DateList &dates, bool withDayLabel, const QStringList &decoNames, const QStringList &enabledPlugins)
{
    clear();

    QList<CalendarDecoration::Decoration *> decos;
    loadDecorations(decoNames, enabledPlugins, decos);
    const bool hasDecos = !decos.isEmpty();

    for (const QDate &date : dates) {
        addDay(decos, date, withDayLabel);
    }

    // Week decoration labels
    if (mWeekLabelBox) {
        placeDecorations(decos, dates.first(), mWeekLabelBox, true);
    }

    qDeleteAll(decos);

    // trigger an update after all layout has been done and the final sizes are known
    QTimer::singleShot(0, this, &AgendaHeader::updateDayLabelSizes);

    return hasDecos;
}

void AgendaHeader::addDay(const DecorationList &decoList, QDate date, bool withDayLabel)
{
    auto topDayLabelBox = new QWidget(mDayLabels);
    auto topDayLabelBoxLayout = new QVBoxLayout(topDayLabelBox);
    topDayLabelBoxLayout->setContentsMargins(0, 0, 0, 0);
    topDayLabelBoxLayout->setSpacing(0);

    mDayLabelsLayout->addWidget(topDayLabelBox);

    if (withDayLabel) {
        int dW = date.dayOfWeek();
        QString veryLongStr = QLocale::system().toString(date, QLocale::LongFormat);
        QString longstr = i18nc("short_weekday short_monthname date (e.g. Mon Aug 13)",
                                "%1 %2 %3",
                                QLocale::system().dayName(dW, QLocale::ShortFormat),
                                QLocale::system().monthName(date.month(), QLocale::ShortFormat),
                                date.day());
        const QString shortstr = QString::number(date.day());

        auto dayLabel = new AlternateLabel(shortstr, longstr, veryLongStr, topDayLabelBox);
        topDayLabelBoxLayout->addWidget(dayLabel);
        dayLabel->setAlignment(Qt::AlignHCenter);
        if (date == QDate::currentDate()) {
            QFont font = dayLabel->font();
            font.setBold(true);
            dayLabel->setFont(font);
        }
        mDateDayLabels.append(dayLabel);

        // if a holiday region is selected, show the holiday name
        const QStringList texts = CalendarSupport::holiday(date);
        for (const QString &text : texts) {
            auto label = new KSqueezedTextLabel(text, topDayLabelBox);
            label->setTextElideMode(Qt::ElideRight);
            topDayLabelBoxLayout->addWidget(label);
            label->setAlignment(Qt::AlignCenter);
        }
    }

    placeDecorations(decoList, date, topDayLabelBox, false);
}

void AgendaHeader::placeDecorations(const DecorationList &decoList, QDate date, QWidget *labelBox, bool forWeek)
{
    for (CalendarDecoration::Decoration *deco : std::as_const(decoList)) {
        const CalendarDecoration::Element::List elements = forWeek ? deco->weekElements(date) : deco->dayElements(date);
        if (!elements.isEmpty()) {
            auto decoHBox = new QWidget(labelBox);
            labelBox->layout()->addWidget(decoHBox);
            auto layout = new QHBoxLayout(decoHBox);
            layout->setSpacing(0);
            layout->setContentsMargins(0, 0, 0, 0);
            decoHBox->setMinimumWidth(1);

            for (CalendarDecoration::Element *it : elements) {
                auto label = new DecorationLabel(it, decoHBox);
                label->setAlignment(Qt::AlignBottom);
                label->setMinimumWidth(1);
                layout->addWidget(label);
            }
        }
    }
}

void AgendaHeader::loadDecorations(const QStringList &decorations, const QStringList &whiteList, DecorationList &decoList)
{
    for (const QString &decoName : decorations) {
        if (whiteList.contains(decoName)) {
            CalendarDecoration::Decoration *deco = loadCalendarDecoration(decoName);
            if (deco != nullptr) {
                decoList << deco;
            }
        }
    }
}

CalendarDecoration::Decoration *AgendaHeader::loadCalendarDecoration(const QString &name)
{
    auto result = KPluginFactory::instantiatePlugin<CalendarDecoration::Decoration>(KPluginMetaData(QStringLiteral("pim6/korganizer/") + name));

    if (result) {
        return result.plugin;
    } else {
        qCDebug(CALENDARVIEW_LOG) << "Factory creation failed" << result.errorString;
        return nullptr;
    }
}

class EventViews::EventIndicatorPrivate
{
public:
    EventIndicatorPrivate(EventIndicator *parent, EventIndicator::Location loc)
        : mLocation(loc)
        , q(parent)
    {
        mEnabled.resize(mColumns);

        QChar ch;
        // Dashed up and down arrow characters
        ch = QChar(mLocation == EventIndicator::Top ? 0x21e1 : 0x21e3);
        QFont font = q->font();
        font.setPointSize(KIconLoader::global()->currentSize(KIconLoader::Dialog));
        QFontMetrics fm(font);
        QRect rect = fm.boundingRect(ch).adjusted(-2, -2, 2, 2);
        mPixmap = QPixmap(rect.size());
        mPixmap.fill(Qt::transparent);
        QPainter p(&mPixmap);
        p.setOpacity(0.33);
        p.setFont(font);
        p.setPen(q->palette().text().color());
        p.drawText(-rect.left(), -rect.top(), ch);
    }

    void adjustGeometry()
    {
        QRect rect;
        rect.setWidth(q->parentWidget()->width());
        rect.setHeight(q->height());
        rect.setLeft(0);
        rect.setTop(mLocation == EventIndicator::Top ? 0 : q->parentWidget()->height() - rect.height());
        q->setGeometry(rect);
    }

public:
    int mColumns = 1;
    const EventIndicator::Location mLocation;
    QPixmap mPixmap;
    QList<bool> mEnabled;

private:
    EventIndicator *const q;
};

EventIndicator::EventIndicator(Location loc, QWidget *parent)
    : QWidget(parent)
    , d(new EventIndicatorPrivate(this, loc))
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedHeight(d->mPixmap.height());
    parent->installEventFilter(this);
}

EventIndicator::~EventIndicator() = default;

void EventIndicator::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    const double cellWidth = static_cast<double>(width()) / d->mColumns;
    const bool isRightToLeft = QApplication::isRightToLeft();
    const uint pixmapOffset = isRightToLeft ? 0 : (cellWidth - d->mPixmap.width());
    for (int i = 0; i < d->mColumns; ++i) {
        if (d->mEnabled[i]) {
            const int xOffset = (isRightToLeft ? (d->mColumns - 1 - i) : i) * cellWidth;
            painter.drawPixmap(xOffset + pixmapOffset, 0, d->mPixmap);
        }
    }
}

bool EventIndicator::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        d->adjustGeometry();
    }
    return false;
}

void EventIndicator::changeColumns(int columns)
{
    d->mColumns = columns;
    d->mEnabled.resize(d->mColumns);

    show();
    raise();
    update();
}

void EventIndicator::enableColumn(int column, bool enable)
{
    Q_ASSERT(column < d->mEnabled.count());
    d->mEnabled[column] = enable;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

class EventViews::AgendaViewPrivate : public Akonadi::ETMCalendar::CalendarObserver
{
    AgendaView *const q;

public:
    explicit AgendaViewPrivate(AgendaView *parent, bool isInteractive, bool isSideBySide)
        : q(parent)
        , mUpdateItem(0)
        , mIsSideBySide(isSideBySide)
        , mIsInteractive(isInteractive)
        , mViewCalendar(MultiViewCalendar::Ptr(new MultiViewCalendar()))
    {
        mViewCalendar->mAgendaView = q;
        mViewCalendar->setETMCalendar(q->calendar());
    }

public:
    // view widgets
    QVBoxLayout *mMainLayout = nullptr;
    AgendaHeader *mTopDayLabelsFrame = nullptr;
    AgendaHeader *mBottomDayLabelsFrame = nullptr;
    QWidget *mAllDayFrame = nullptr;
    QSpacerItem *mAllDayRightSpacer = nullptr;
    QWidget *mTimeBarHeaderFrame = nullptr;
    QSplitter *mSplitterAgenda = nullptr;
    QList<QLabel *> mTimeBarHeaders;

    Agenda *mAllDayAgenda = nullptr;
    Agenda *mAgenda = nullptr;

    TimeLabelsZone *mTimeLabelsZone = nullptr;

    KCalendarCore::DateList mSelectedDates; // List of dates to be displayed
    KCalendarCore::DateList mSaveSelectedDates; // Save the list of dates between updateViews
    int mViewType;
    EventIndicator *mEventIndicatorTop = nullptr;
    EventIndicator *mEventIndicatorBottom = nullptr;

    QList<int> mMinY;
    QList<int> mMaxY;

    QList<bool> mHolidayMask;

    QDateTime mTimeSpanBegin;
    QDateTime mTimeSpanEnd;
    bool mTimeSpanInAllDay = true;
    bool mAllowAgendaUpdate = true;

    Akonadi::Item mUpdateItem;

    const bool mIsSideBySide;

    QWidget *mDummyAllDayLeft = nullptr;
    bool mUpdateAllDayAgenda = true;
    bool mUpdateAgenda = true;
    bool mIsInteractive;
    bool mUpdateEventIndicatorsScheduled = false;

    // Contains days that have at least one all-day Event with TRANSP: OPAQUE (busy)
    // that has you as organizer or attendee so we can color background with a different
    // color
    QMap<QDate, KCalendarCore::Event::List> mBusyDays;

    EventViews::MultiViewCalendar::Ptr mViewCalendar;
    bool makesWholeDayBusy(const KCalendarCore::Incidence::Ptr &incidence) const;
    void clearView();
    void setChanges(EventView::Changes changes, const KCalendarCore::Incidence::Ptr &incidence = KCalendarCore::Incidence::Ptr());

    /**
        Returns a list of consecutive dates, starting with @p start and ending
        with @p end. If either start or end are invalid, a list with
        QDate::currentDate() is returned */
    static QList<QDate> generateDateList(QDate start, QDate end);

    void changeColumns(int numColumns);

    AgendaItem::List agendaItems(const QString &uid) const;

    // insertAtDateTime is in the view's timezone
    void insertIncidence(const KCalendarCore::Incidence::Ptr &, const QDateTime &recurrenceId, const QDateTime &insertAtDateTime, bool createSelected);
    void reevaluateIncidence(const KCalendarCore::Incidence::Ptr &incidence);

    bool datesEqual(const KCalendarCore::Incidence::Ptr &one, const KCalendarCore::Incidence::Ptr &two) const;

    /**
     * Returns false if the incidence is for sure outside of the visible timespan.
     * Returns true if it might be, meaning that to be sure, timezones must be
     * taken into account.
     * This is a very fast way of discarding incidences that are outside of the
     * timespan and only performing expensive timezone operations on the ones
     * that might be viisble
     */
    bool mightBeVisible(const KCalendarCore::Incidence::Ptr &incidence) const;

    void updateAllDayRightSpacer();

protected:
    /* reimplemented from KCalendarCore::Calendar::CalendarObserver */
    void calendarIncidenceAdded(const KCalendarCore::Incidence::Ptr &incidence) override;
    void calendarIncidenceChanged(const KCalendarCore::Incidence::Ptr &incidence) override;
    void calendarIncidenceDeleted(const KCalendarCore::Incidence::Ptr &incidence, const KCalendarCore::Calendar *calendar) override;

private:
    // quiet --overloaded-virtual warning
    using KCalendarCore::Calendar::CalendarObserver::calendarIncidenceDeleted;
};

bool AgendaViewPrivate::datesEqual(const KCalendarCore::Incidence::Ptr &one, const KCalendarCore::Incidence::Ptr &two) const
{
    const auto start1 = one->dtStart();
    const auto start2 = two->dtStart();
    const auto end1 = one->dateTime(KCalendarCore::Incidence::RoleDisplayEnd);
    const auto end2 = two->dateTime(KCalendarCore::Incidence::RoleDisplayEnd);

    if (start1.isValid() ^ start2.isValid()) {
        return false;
    }

    if (end1.isValid() ^ end2.isValid()) {
        return false;
    }

    if (start1.isValid() && start1 != start2) {
        return false;
    }

    if (end1.isValid() && end1 != end2) {
        return false;
    }

    return true;
}

AgendaItem::List AgendaViewPrivate::agendaItems(const QString &uid) const
{
    AgendaItem::List allDayAgendaItems = mAllDayAgenda->agendaItems(uid);
    return allDayAgendaItems.isEmpty() ? mAgenda->agendaItems(uid) : allDayAgendaItems;
}

bool AgendaViewPrivate::mightBeVisible(const KCalendarCore::Incidence::Ptr &incidence) const
{
    KCalendarCore::Todo::Ptr todo = incidence.dynamicCast<KCalendarCore::Todo>();

    // KDateTime::toTimeSpec() is expensive, so lets first compare only the date,
    // to see if the incidence is visible.
    // If it's more than 48h of diff, then for sure it won't be visible,
    // independently of timezone.
    // The largest difference between two timezones is about 24 hours.

    if (todo && todo->isOverdue()) {
        // Don't optimize this case. Overdue to-dos have their own rules for displaying themselves
        return true;
    }

    if (!incidence->recurs()) {
        // If DTEND/DTDUE is before the 1st visible column
        const QDate tdate = incidence->dateTime(KCalendarCore::Incidence::RoleEnd).date();
        if (tdate.daysTo(mSelectedDates.first()) > 2) {
            return false;
        }

        // if DTSTART is after the last visible column
        if (!todo && mSelectedDates.last().daysTo(incidence->dtStart().date()) > 2) {
            return false;
        }

        // if DTDUE is after the last visible column
        if (todo && mSelectedDates.last().daysTo(todo->dtDue().date()) > 2) {
            return false;
        }
    }

    return true;
}

void AgendaViewPrivate::changeColumns(int numColumns)
{
    // mMinY, mMaxY and mEnabled must all have the same size.
    // Make sure you preserve this order because mEventIndicatorTop->changeColumns()
    // can trigger a lot of stuff, and code will be executed when mMinY wasn't resized yet.
    mMinY.resize(numColumns);
    mMaxY.resize(numColumns);
    mEventIndicatorTop->changeColumns(numColumns);
    mEventIndicatorBottom->changeColumns(numColumns);
}

/** static */
QList<QDate> AgendaViewPrivate::generateDateList(QDate start, QDate end)
{
    QList<QDate> list;

    if (start.isValid() && end.isValid() && end >= start && start.daysTo(end) < AgendaView::MAX_DAY_COUNT) {
        QDate date = start;
        list.reserve(start.daysTo(end) + 1);
        while (date <= end) {
            list.append(date);
            date = date.addDays(1);
        }
    } else {
        list.append(QDate::currentDate());
    }

    return list;
}

void AgendaViewPrivate::reevaluateIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    if (!incidence || !mViewCalendar->isValid(incidence)) {
        qCWarning(CALENDARVIEW_LOG) << "invalid incidence or item not found." << incidence;
        return;
    }

    q->removeIncidence(incidence);
    q->displayIncidence(incidence, false);
    mAgenda->checkScrollBoundaries();
    q->updateEventIndicators();
}

void AgendaViewPrivate::calendarIncidenceAdded(const KCalendarCore::Incidence::Ptr &incidence)
{
    if (!incidence || !mViewCalendar->isValid(incidence)) {
        qCCritical(CALENDARVIEW_LOG) << "AgendaViewPrivate::calendarIncidenceAdded() Invalid incidence or item:" << incidence;
        Q_ASSERT(false);
        return;
    }

    if (incidence->hasRecurrenceId()) {
        if (auto mainIncidence = q->calendar2(incidence)->incidence(incidence->uid())) {
            // Reevaluate the main event instead, if it was inserted before this one.
            reevaluateIncidence(mainIncidence);
        } else if (q->displayIncidence(incidence, false)) {
            // Display disassociated occurrences because errors sometimes destroy
            // the main recurring incidence.
            mAgenda->checkScrollBoundaries();
            q->scheduleUpdateEventIndicators();
        }
    } else if (incidence->recurs()) {
        // Reevaluate recurring incidences to clean up any disassociated
        // occurrences that were inserted before it.
        reevaluateIncidence(incidence);
    } else if (q->displayIncidence(incidence, false)) {
        // Ordinary non-recurring non-disassociated instances.
        mAgenda->checkScrollBoundaries();
        q->scheduleUpdateEventIndicators();
    }
}

void AgendaViewPrivate::calendarIncidenceChanged(const KCalendarCore::Incidence::Ptr &incidence)
{
    if (!incidence || incidence->uid().isEmpty()) {
        qCCritical(CALENDARVIEW_LOG) << "AgendaView::calendarIncidenceChanged() Invalid incidence or empty UID. " << incidence;
        Q_ASSERT(false);
        return;
    }

    AgendaItem::List agendaItems = this->agendaItems(incidence->uid());
    if (agendaItems.isEmpty()) {
        qCWarning(CALENDARVIEW_LOG) << "AgendaView::calendarIncidenceChanged() Invalid agendaItem for incidence " << incidence->uid();
        return;
    }

    // Optimization: If the dates didn't change, just repaint it.
    // This optimization for now because we need to process collisions between agenda items.
    if (false && !incidence->recurs() && agendaItems.count() == 1) {
        KCalendarCore::Incidence::Ptr originalIncidence = agendaItems.first()->incidence();

        if (datesEqual(originalIncidence, incidence)) {
            for (const AgendaItem::QPtr &agendaItem : std::as_const(agendaItems)) {
                agendaItem->setIncidence(KCalendarCore::Incidence::Ptr(incidence->clone()));
                agendaItem->update();
            }
            return;
        }
    }

    if (incidence->hasRecurrenceId() && mViewCalendar->isValid(incidence)) {
        // Reevaluate the main event instead, if it exists
        KCalendarCore::Incidence::Ptr mainIncidence = q->calendar2(incidence)->incidence(incidence->uid());
        reevaluateIncidence(mainIncidence ? mainIncidence : incidence);
    } else {
        reevaluateIncidence(incidence);
    }

    // No need to call setChanges(), that triggers a fillAgenda()
    // setChanges(q->changes() | IncidencesEdited, incidence);
}

void AgendaViewPrivate::calendarIncidenceDeleted(const KCalendarCore::Incidence::Ptr &incidence, const KCalendarCore::Calendar *calendar)
{
    Q_UNUSED(calendar)
    if (!incidence || incidence->uid().isEmpty()) {
        qCWarning(CALENDARVIEW_LOG) << "invalid incidence or empty uid: " << incidence;
        Q_ASSERT(false);
        return;
    }

    q->removeIncidence(incidence);

    if (incidence->hasRecurrenceId()) {
        // Reevaluate the main event, if it exists. The exception was removed so the main recurrent series
        // will no be bigger.
        if (mViewCalendar->isValid(incidence->uid())) {
            KCalendarCore::Incidence::Ptr mainIncidence = q->calendar2(incidence->uid())->incidence(incidence->uid());
            if (mainIncidence) {
                reevaluateIncidence(mainIncidence);
            }
        }
    } else if (mightBeVisible(incidence)) {
        // No need to call setChanges(), that triggers a fillAgenda()
        // setChanges(q->changes() | IncidencesDeleted, CalendarSupport::incidence(incidence));
        mAgenda->checkScrollBoundaries();
        q->scheduleUpdateEventIndicators();
    }
}

void EventViews::AgendaViewPrivate::setChanges(EventView::Changes changes, const KCalendarCore::Incidence::Ptr &incidence)
{
    // We could just call EventView::setChanges(...) but we're going to do a little
    // optimization. If only an all day item was changed, only all day agenda
    // should be updated.

    // all bits = 1
    const int ones = ~0;

    const int incidenceOperations = AgendaView::IncidencesAdded | AgendaView::IncidencesEdited | AgendaView::IncidencesDeleted;

    // If changes has a flag turned on, other than incidence operations, then update both agendas
    if ((ones ^ incidenceOperations) & changes) {
        mUpdateAllDayAgenda = true;
        mUpdateAgenda = true;
    } else if (incidence) {
        mUpdateAllDayAgenda = mUpdateAllDayAgenda | incidence->allDay();
        mUpdateAgenda = mUpdateAgenda | !incidence->allDay();
    }

    q->EventView::setChanges(changes);
}

void AgendaViewPrivate::clearView()
{
    if (mUpdateAllDayAgenda) {
        mAllDayAgenda->clear();
    }

    if (mUpdateAgenda) {
        mAgenda->clear();
    }

    mBusyDays.clear();
}

void AgendaViewPrivate::insertIncidence(const KCalendarCore::Incidence::Ptr &incidence,
                                        const QDateTime &recurrenceId,
                                        const QDateTime &insertAtDateTime,
                                        bool createSelected)
{
    if (!q->filterByCollectionSelection(incidence)) {
        return;
    }

    KCalendarCore::Event::Ptr event = CalendarSupport::event(incidence);
    KCalendarCore::Todo::Ptr todo = CalendarSupport::todo(incidence);

    const QDate insertAtDate = insertAtDateTime.date();

    // In case incidence->dtStart() isn't visible (crosses boundaries)
    const int curCol = qMax(mSelectedDates.first().daysTo(insertAtDate), qint64(0));

    // The date for the event is not displayed, just ignore it
    if (curCol >= mSelectedDates.count()) {
        return;
    }

    if (mMinY.count() <= curCol) {
        mMinY.resize(mSelectedDates.count());
    }

    if (mMaxY.count() <= curCol) {
        mMaxY.resize(mSelectedDates.count());
    }

    // Default values, which can never be reached
    mMinY[curCol] = mAgenda->timeToY(QTime(23, 59)) + 1;
    mMaxY[curCol] = mAgenda->timeToY(QTime(0, 0)) - 1;

    int beginX;
    int endX;
    if (event) {
        const QDate firstVisibleDate = mSelectedDates.first();
        QDateTime dtEnd = event->dtEnd().toLocalTime();
        if (!event->allDay() && dtEnd > event->dtStart()) {
            // If dtEnd's time portion is 00:00:00, the event ends on the previous day
            // unless it also starts at 00:00:00 (a duration of 0).
            dtEnd = dtEnd.addMSecs(-1);
        }
        const int duration = event->dtStart().toLocalTime().daysTo(dtEnd);
        if (insertAtDate < firstVisibleDate) {
            beginX = curCol + firstVisibleDate.daysTo(insertAtDate);
            endX = beginX + duration;
        } else {
            beginX = curCol;
            endX = beginX + duration;
        }
    } else if (todo) {
        if (!todo->hasDueDate()) {
            return; // todo shall not be displayed if it has no date
        }
        beginX = endX = curCol;
    } else {
        return;
    }

    const QDate today = QDate::currentDate();
    if (todo && todo->isOverdue() && today >= insertAtDate) {
        mAllDayAgenda->insertAllDayItem(incidence, recurrenceId, curCol, curCol, createSelected);
    } else if (incidence->allDay()) {
        mAllDayAgenda->insertAllDayItem(incidence, recurrenceId, beginX, endX, createSelected);
    } else if (event && event->isMultiDay(QTimeZone::systemTimeZone())) {
        // TODO: We need a better isMultiDay(), one that receives the occurrence.

        // In the single-day handling code there's a neat comment on why
        // we're calculating the start time this way
        const QTime startTime = insertAtDateTime.time();

        // In the single-day handling code there's a neat comment on why we use the
        // duration instead of fetching the end time directly
        const int durationOfFirstOccurrence = event->dtStart().secsTo(event->dtEnd());
        QTime endTime = startTime.addSecs(durationOfFirstOccurrence);

        const int startY = mAgenda->timeToY(startTime);

        if (endTime == QTime(0, 0, 0)) {
            endTime = QTime(23, 59, 59);
        }
        const int endY = mAgenda->timeToY(endTime) - 1;
        if ((beginX <= 0 && curCol == 0) || beginX == curCol) {
            mAgenda->insertMultiItem(incidence, recurrenceId, beginX, endX, startY, endY, createSelected);
        }
        if (beginX == curCol) {
            mMaxY[curCol] = mAgenda->timeToY(QTime(23, 59));
            if (startY < mMinY[curCol]) {
                mMinY[curCol] = startY;
            }
        } else if (endX == curCol) {
            mMinY[curCol] = mAgenda->timeToY(QTime(0, 0));
            if (endY > mMaxY[curCol]) {
                mMaxY[curCol] = endY;
            }
        } else {
            mMinY[curCol] = mAgenda->timeToY(QTime(0, 0));
            mMaxY[curCol] = mAgenda->timeToY(QTime(23, 59));
        }
    } else {
        int startY = 0;
        int endY = 0;
        if (event) { // Single day events fall here
            // Don't use event->dtStart().toTimeSpec(timeSpec).time().
            // If it's a UTC recurring event it should have a different time when it crosses DST,
            // so we must use insertAtDate here, so we get the correct time.
            //
            // The nth occurrence doesn't always have the same time as the 1st occurrence.
            const QTime startTime = insertAtDateTime.time();

            // We could just fetch the end time directly from dtEnd() instead of adding a duration to the
            // start time. This way is best because it preserves the duration of the event. There are some
            // corner cases where the duration would be messed up, for example a UTC event that when
            // converted to local has dtStart() in day light saving time, but dtEnd() outside DST.
            // It could create events with 0 duration.
            const int durationOfFirstOccurrence = event->dtStart().secsTo(event->dtEnd());
            QTime endTime = startTime.addSecs(durationOfFirstOccurrence);

            startY = mAgenda->timeToY(startTime);
            if (durationOfFirstOccurrence != 0 && endTime == QTime(0, 0, 0)) {
                // If endTime is 00:00:00, the event ends on the previous day
                // unless it also starts at 00:00:00 (a duration of 0).
                endTime = endTime.addMSecs(-1);
            }
            endY = mAgenda->timeToY(endTime) - 1;
        }
        if (todo) {
            QTime t;
            if (todo->recurs()) {
                // The time we get depends on the insertAtDate, because of daylight savings changes
                const QDateTime ocurrrenceDateTime = QDateTime(insertAtDate, todo->dtDue().time(), todo->dtDue().timeZone());
                t = ocurrrenceDateTime.toLocalTime().time();
            } else {
                t = todo->dtDue().toLocalTime().time();
            }

            if (t == QTime(0, 0) && !todo->recurs()) {
                // To-dos due at 00h00 are drawn at the previous day and ending at
                // 23h59. For recurring to-dos, that's not being done because it wasn't
                // implemented yet in ::fillAgenda().
                t = QTime(23, 59);
            }

            const int halfHour = 1800;
            if (t.addSecs(-halfHour) < t) {
                startY = mAgenda->timeToY(t.addSecs(-halfHour));
                endY = mAgenda->timeToY(t) - 1;
            } else {
                startY = 0;
                endY = mAgenda->timeToY(t.addSecs(halfHour)) - 1;
            }
        }
        if (endY < startY) {
            endY = startY;
        }
        mAgenda->insertItem(incidence, recurrenceId, curCol, startY, endY, 1, 1, createSelected);
        if (startY < mMinY[curCol]) {
            mMinY[curCol] = startY;
        }
        if (endY > mMaxY[curCol]) {
            mMaxY[curCol] = endY;
        }
    }
}

void AgendaViewPrivate::updateAllDayRightSpacer()
{
    if (!mAllDayRightSpacer) {
        return;
    }

    // Make the all-day and normal agendas line up with each other
    auto verticalAgendaScrollBar = mAgenda->verticalScrollBar();
    int margin = verticalAgendaScrollBar->isVisible() ? verticalAgendaScrollBar->width() : 0;
    if (q->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents)) {
        // Needed for some styles. Oxygen needs it, Plastique does not.
        margin -= mAgenda->scrollArea()->frameWidth();
    }
    mAllDayRightSpacer->changeSize(margin, 0, QSizePolicy::Fixed);
    mAllDayFrame->layout()->invalidate(); // needed to pick up change of space size
}

////////////////////////////////////////////////////////////////////////////

AgendaView::AgendaView(QDate start, QDate end, bool isInteractive, bool isSideBySide, QWidget *parent)
    : EventView(parent)
    , d(new AgendaViewPrivate(this, isInteractive, isSideBySide))
{
    init(start, end);
}

AgendaView::AgendaView(const PrefsPtr &prefs, QDate start, QDate end, bool isInteractive, bool isSideBySide, QWidget *parent)
    : EventView(parent)
    , d(new AgendaViewPrivate(this, isInteractive, isSideBySide))
{
    setPreferences(prefs);
    init(start, end);
}

void AgendaView::init(QDate start, QDate end)
{
    d->mSelectedDates = AgendaViewPrivate::generateDateList(start, end);

    d->mMainLayout = new QVBoxLayout(this);
    d->mMainLayout->setContentsMargins(0, 0, 0, 0);

    /* Create day name labels for agenda columns */
    d->mTopDayLabelsFrame = new AgendaHeader(d->mIsSideBySide, this);
    d->mMainLayout->addWidget(d->mTopDayLabelsFrame);

    /* Create agenda splitter */
    d->mSplitterAgenda = new QSplitter(Qt::Vertical, this);
    d->mMainLayout->addWidget(d->mSplitterAgenda, 1);

    /* Create all-day agenda widget */
    d->mAllDayFrame = new QWidget(d->mSplitterAgenda);
    auto allDayFrameLayout = new QHBoxLayout(d->mAllDayFrame);
    allDayFrameLayout->setContentsMargins(0, 0, 0, 0);
    allDayFrameLayout->setSpacing(SPACING);

    // Alignment and description widgets
    if (!d->mIsSideBySide) {
        d->mTimeBarHeaderFrame = new QWidget(d->mAllDayFrame);
        allDayFrameLayout->addWidget(d->mTimeBarHeaderFrame);
        auto timeBarHeaderFrameLayout = new QHBoxLayout(d->mTimeBarHeaderFrame);
        timeBarHeaderFrameLayout->setContentsMargins(0, 0, 0, 0);
        timeBarHeaderFrameLayout->setSpacing(0);
        d->mDummyAllDayLeft = new QWidget(d->mAllDayFrame);
        allDayFrameLayout->addWidget(d->mDummyAllDayLeft);
    }

    // The widget itself
    auto allDayScrollArea = new AgendaScrollArea(true, this, d->mIsInteractive, d->mAllDayFrame);
    allDayFrameLayout->addWidget(allDayScrollArea);
    d->mAllDayAgenda = allDayScrollArea->agenda();

    /* Create the main agenda widget and the related widgets */
    auto agendaFrame = new QWidget(d->mSplitterAgenda);
    auto agendaLayout = new QHBoxLayout(agendaFrame);
    agendaLayout->setContentsMargins(0, 0, 0, 0);
    agendaLayout->setSpacing(SPACING);

    // Create agenda
    auto scrollArea = new AgendaScrollArea(false, this, d->mIsInteractive, agendaFrame);
    d->mAgenda = scrollArea->agenda();
    d->mAgenda->verticalScrollBar()->installEventFilter(this);

    // Create event indicator bars
    d->mEventIndicatorTop = new EventIndicator(EventIndicator::Top, scrollArea->viewport());
    d->mEventIndicatorBottom = new EventIndicator(EventIndicator::Bottom, scrollArea->viewport());

    // Create time labels
    d->mTimeLabelsZone = new TimeLabelsZone(this, preferences(), d->mAgenda);

    // This timeLabelsZoneLayout is for adding some spacing
    // to align timelabels, to agenda's grid
    auto timeLabelsZoneLayout = new QVBoxLayout();

    agendaLayout->addLayout(timeLabelsZoneLayout);
    agendaLayout->addWidget(scrollArea);

    timeLabelsZoneLayout->addSpacing(scrollArea->frameWidth());
    timeLabelsZoneLayout->addWidget(d->mTimeLabelsZone);
    timeLabelsZoneLayout->addSpacing(scrollArea->frameWidth());

    // Scrolling
    connect(d->mAgenda, &Agenda::zoomView, this, &AgendaView::zoomView);

    // Event indicator updates
    connect(d->mAgenda, &Agenda::lowerYChanged, this, &AgendaView::updateEventIndicatorTop);
    connect(d->mAgenda, &Agenda::upperYChanged, this, &AgendaView::updateEventIndicatorBottom);

    if (d->mIsSideBySide) {
        d->mTimeLabelsZone->hide();
    }

    /* Create a frame at the bottom which may be used by decorations */
    d->mBottomDayLabelsFrame = new AgendaHeader(d->mIsSideBySide, this);
    d->mBottomDayLabelsFrame->hide();

    d->mTopDayLabelsFrame->setAgenda(d->mAgenda);
    d->mBottomDayLabelsFrame->setAgenda(d->mAgenda);

    if (!d->mIsSideBySide) {
        d->mAllDayRightSpacer = new QSpacerItem(0, 0);
        d->mAllDayFrame->layout()->addItem(d->mAllDayRightSpacer);
    }

    updateTimeBarWidth();

    // Don't call it now, bottom agenda isn't fully up yet
    QMetaObject::invokeMethod(this, &AgendaView::alignAgendas, Qt::QueuedConnection);

    // Whoever changes this code, remember to leave createDayLabels()
    // inside the ctor, so it's always called before readSettings(), so
    // readSettings() works on the splitter that has the right amount of
    // widgets (createDayLabels() via placeDecorationFrame() removes widgets).
    createDayLabels(true);

    /* Connect the agendas */

    connect(d->mAllDayAgenda, &Agenda::newTimeSpanSignal, this, &AgendaView::newTimeSpanSelectedAllDay);

    connect(d->mAgenda, &Agenda::newTimeSpanSignal, this, &AgendaView::newTimeSpanSelected);

    connectAgenda(d->mAgenda, d->mAllDayAgenda);
    connectAgenda(d->mAllDayAgenda, d->mAgenda);
}

AgendaView::~AgendaView()
{
    for (const ViewCalendar::Ptr &cal : std::as_const(d->mViewCalendar->mSubCalendars)) {
        if (cal->getCalendar()) {
            cal->getCalendar()->unregisterObserver(d.get());
        }
    }
}

void AgendaView::showEvent(QShowEvent *showEvent)
{
    EventView::showEvent(showEvent);

    // agenda scrollbar width only set now, so redo margin calculation
    d->mTopDayLabelsFrame->updateMargins();
    d->mBottomDayLabelsFrame->updateMargins();
    d->updateAllDayRightSpacer();
}

bool AgendaView::eventFilter(QObject *object, QEvent *event)
{
    if ((object == d->mAgenda->verticalScrollBar()) && ((event->type() == QEvent::Show) || (event->type() == QEvent::Hide))) {
        d->mTopDayLabelsFrame->updateMargins();
        d->mBottomDayLabelsFrame->updateMargins();
        d->updateAllDayRightSpacer();
    }
    return false;
}

KCalendarCore::Calendar::Ptr AgendaView::calendar2(const KCalendarCore::Incidence::Ptr &incidence) const
{
    return d->mViewCalendar->findCalendar(incidence)->getCalendar();
}

KCalendarCore::Calendar::Ptr AgendaView::calendar2(const QString &incidenceIdentifier) const
{
    return d->mViewCalendar->findCalendar(incidenceIdentifier)->getCalendar();
}

void AgendaView::setCalendar(const Akonadi::ETMCalendar::Ptr &cal)
{
    if (calendar()) {
        calendar()->unregisterObserver(d.get());
    }
    Q_ASSERT(cal);
    EventView::setCalendar(cal);
    calendar()->registerObserver(d.get());
    d->mViewCalendar->setETMCalendar(cal);
    d->mAgenda->setCalendar(d->mViewCalendar);
    d->mAllDayAgenda->setCalendar(d->mViewCalendar);
}

void AgendaView::addCalendar(const ViewCalendar::Ptr &cal)
{
    d->mViewCalendar->addCalendar(cal);
    cal->getCalendar()->registerObserver(d.get());
}

void AgendaView::connectAgenda(Agenda *agenda, Agenda *otherAgenda)
{
    connect(agenda, &Agenda::showNewEventPopupSignal, this, &AgendaView::showNewEventPopupSignal);

    connect(agenda, &Agenda::showIncidencePopupSignal, this, &AgendaView::slotShowIncidencePopup);

    agenda->setCalendar(d->mViewCalendar);

    connect(agenda, &Agenda::newEventSignal, this, qOverload<>(&EventView::newEventSignal));

    connect(agenda, &Agenda::newStartSelectSignal, otherAgenda, &Agenda::clearSelection);
    connect(agenda, &Agenda::newStartSelectSignal, this, &AgendaView::timeSpanSelectionChanged);

    connect(agenda, &Agenda::editIncidenceSignal, this, &AgendaView::slotEditIncidence);
    connect(agenda, &Agenda::showIncidenceSignal, this, &AgendaView::slotShowIncidence);
    connect(agenda, &Agenda::deleteIncidenceSignal, this, &AgendaView::slotDeleteIncidence);

    // drag signals
    connect(agenda, &Agenda::startDragSignal, this, [this](const KCalendarCore::Incidence::Ptr &ptr) {
        startDrag(ptr);
    });

    // synchronize selections
    connect(agenda, &Agenda::incidenceSelected, otherAgenda, &Agenda::deselectItem);
    connect(agenda, &Agenda::incidenceSelected, this, &AgendaView::slotIncidenceSelected);

    // rescheduling of todos by d'n'd
    connect(agenda,
            qOverload<const KCalendarCore::Incidence::List &, const QPoint &, bool>(&Agenda::droppedIncidences),
            this,
            qOverload<const KCalendarCore::Incidence::List &, const QPoint &, bool>(&AgendaView::slotIncidencesDropped));
    connect(agenda,
            qOverload<const QList<QUrl> &, const QPoint &, bool>(&Agenda::droppedIncidences),
            this,
            qOverload<const QList<QUrl> &, const QPoint &, bool>(&AgendaView::slotIncidencesDropped));
}

void AgendaView::slotIncidenceSelected(const KCalendarCore::Incidence::Ptr &incidence, QDate date)
{
    Akonadi::Item item = d->mViewCalendar->item(incidence);
    if (item.isValid()) {
        Q_EMIT incidenceSelected(item, date);
    }
}

void AgendaView::slotShowIncidencePopup(const KCalendarCore::Incidence::Ptr &incidence, QDate date)
{
    Akonadi::Item item = d->mViewCalendar->item(incidence);
    // qDebug() << "wanna see the popup for " << incidence->uid() << item.id();
    if (item.isValid()) {
        Q_EMIT showIncidencePopupSignal(item, date);
    }
}

void AgendaView::slotShowIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    Akonadi::Item item = d->mViewCalendar->item(incidence);
    if (item.isValid()) {
        Q_EMIT showIncidenceSignal(item);
    }
}

void AgendaView::slotEditIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    Akonadi::Item item = d->mViewCalendar->item(incidence);
    if (item.isValid()) {
        Q_EMIT editIncidenceSignal(item);
    }
}

void AgendaView::slotDeleteIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    Akonadi::Item item = d->mViewCalendar->item(incidence);
    if (item.isValid()) {
        Q_EMIT deleteIncidenceSignal(item);
    }
}

void AgendaView::zoomInVertically()
{
    if (!d->mIsSideBySide) {
        preferences()->setHourSize(preferences()->hourSize() + 1);
    }
    d->mAgenda->updateConfig();
    d->mAgenda->checkScrollBoundaries();

    d->mTimeLabelsZone->updateAll();
    setChanges(changes() | ZoomChanged);
    updateView();
}

void AgendaView::zoomOutVertically()
{
    if (preferences()->hourSize() > 4 || d->mIsSideBySide) {
        if (!d->mIsSideBySide) {
            preferences()->setHourSize(preferences()->hourSize() - 1);
        }
        d->mAgenda->updateConfig();
        d->mAgenda->checkScrollBoundaries();

        d->mTimeLabelsZone->updateAll();
        setChanges(changes() | ZoomChanged);
        updateView();
    }
}

void AgendaView::zoomInHorizontally(QDate date)
{
    QDate begin;
    QDate newBegin;
    QDate dateToZoom = date;
    int ndays;
    int count;

    begin = d->mSelectedDates.first();
    ndays = begin.daysTo(d->mSelectedDates.constLast());

    // zoom with Action and are there a selected Incidence?, Yes, I zoom in to it.
    if (!dateToZoom.isValid()) {
        dateToZoom = d->mAgenda->selectedIncidenceDate();
    }

    if (!dateToZoom.isValid()) {
        if (ndays > 1) {
            newBegin = begin.addDays(1);
            count = ndays - 1;
            Q_EMIT zoomViewHorizontally(newBegin, count);
        }
    } else {
        if (ndays <= 2) {
            newBegin = dateToZoom;
            count = 1;
        } else {
            newBegin = dateToZoom.addDays(-ndays / 2 + 1);
            count = ndays - 1;
        }
        Q_EMIT zoomViewHorizontally(newBegin, count);
    }
}

void AgendaView::zoomOutHorizontally(QDate date)
{
    QDate begin;
    QDate newBegin;
    QDate dateToZoom = date;
    int ndays;
    int count;

    begin = d->mSelectedDates.first();
    ndays = begin.daysTo(d->mSelectedDates.constLast());

    // zoom with Action and are there a selected Incidence?, Yes, I zoom out to it.
    if (!dateToZoom.isValid()) {
        dateToZoom = d->mAgenda->selectedIncidenceDate();
    }

    if (!dateToZoom.isValid()) {
        newBegin = begin.addDays(-1);
        count = ndays + 3;
    } else {
        newBegin = dateToZoom.addDays(-ndays / 2 - 1);
        count = ndays + 3;
    }

    if (abs(count) >= 31) {
        qCDebug(CALENDARVIEW_LOG) << "change to the month view?";
    } else {
        // We want to center the date
        Q_EMIT zoomViewHorizontally(newBegin, count);
    }
}

void AgendaView::zoomView(const int delta, QPoint pos, const Qt::Orientation orient)
{
    // TODO find out why this is necessary. seems to be some kind of performance hack
    static QDate zoomDate;
    static auto t = new QTimer(this);

    // Zoom to the selected incidence, on the other way
    // zoom to the date on screen after the first mousewheel move.
    if (orient == Qt::Horizontal) {
        const QDate date = d->mAgenda->selectedIncidenceDate();
        if (date.isValid()) {
            zoomDate = date;
        } else {
            if (!t->isActive()) {
                zoomDate = d->mSelectedDates[pos.x()];
            }
            t->setSingleShot(true);
            t->start(1s);
        }
        if (delta > 0) {
            zoomOutHorizontally(zoomDate);
        } else {
            zoomInHorizontally(zoomDate);
        }
    } else {
        // Vertical zoom
        const QPoint posConstentsOld = d->mAgenda->gridToContents(pos);
        if (delta > 0) {
            zoomOutVertically();
        } else {
            zoomInVertically();
        }
        const QPoint posConstentsNew = d->mAgenda->gridToContents(pos);
        d->mAgenda->verticalScrollBar()->scroll(0, posConstentsNew.y() - posConstentsOld.y());
    }
}

void AgendaView::createDayLabels(bool force)
{
    // Check if mSelectedDates has changed, if not just return
    // Removes some flickering and gains speed (since this is called by each updateView())
    if (!force && d->mSaveSelectedDates == d->mSelectedDates) {
        return;
    }
    d->mSaveSelectedDates = d->mSelectedDates;

    const QStringList topStrDecos = preferences()->decorationsAtAgendaViewTop();
    const QStringList botStrDecos = preferences()->decorationsAtAgendaViewBottom();
    const QStringList selectedPlugins = preferences()->selectedPlugins();

    const bool hasTopDecos = d->mTopDayLabelsFrame->createDayLabels(d->mSelectedDates, true, topStrDecos, selectedPlugins);
    const bool hasBottomDecos = d->mBottomDayLabelsFrame->createDayLabels(d->mSelectedDates, false, botStrDecos, selectedPlugins);

    // no splitter handle if no top deco elements, so something which needs resizing
    if (hasTopDecos) {
        // inserts in the first position, takes ownership
        d->mSplitterAgenda->insertWidget(0, d->mTopDayLabelsFrame);
    } else {
        d->mTopDayLabelsFrame->setParent(this);
        d->mMainLayout->insertWidget(0, d->mTopDayLabelsFrame);
    }
    // avoid splitter handle if no bottom labels, so something which needs resizing
    if (hasBottomDecos) {
        // inserts in the last position
        d->mBottomDayLabelsFrame->setParent(d->mSplitterAgenda);
        d->mBottomDayLabelsFrame->show();
    } else {
        d->mBottomDayLabelsFrame->setParent(this);
        d->mBottomDayLabelsFrame->hide();
    }
}

void AgendaView::enableAgendaUpdate(bool enable)
{
    d->mAllowAgendaUpdate = enable;
}

int AgendaView::currentDateCount() const
{
    return d->mSelectedDates.count();
}

Akonadi::Item::List AgendaView::selectedIncidences() const
{
    Akonadi::Item::List selected;

    KCalendarCore::Incidence::Ptr agendaitem = d->mAgenda->selectedIncidence();
    if (agendaitem) {
        selected.append(d->mViewCalendar->item(agendaitem));
    }

    KCalendarCore::Incidence::Ptr dayitem = d->mAllDayAgenda->selectedIncidence();
    if (dayitem) {
        selected.append(d->mViewCalendar->item(dayitem));
    }

    return selected;
}

KCalendarCore::DateList AgendaView::selectedIncidenceDates() const
{
    KCalendarCore::DateList selected;
    QDate qd;

    qd = d->mAgenda->selectedIncidenceDate();
    if (qd.isValid()) {
        selected.append(qd);
    }

    qd = d->mAllDayAgenda->selectedIncidenceDate();
    if (qd.isValid()) {
        selected.append(qd);
    }

    return selected;
}

bool AgendaView::eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const
{
    if (selectionStart().isValid()) {
        QDateTime start = selectionStart();
        QDateTime end = selectionEnd();

        if (start.secsTo(end) == 15 * 60) {
            // One cell in the agenda view selected, e.g.
            // because of a double-click, => Use the default duration
            QTime defaultDuration(CalendarSupport::KCalPrefs::instance()->defaultDuration().time());
            int addSecs = (defaultDuration.hour() * 3600) + (defaultDuration.minute() * 60);
            end = start.addSecs(addSecs);
        }

        startDt = start;
        endDt = end;
        allDay = selectedIsAllDay();
        return true;
    }
    return false;
}

/** returns if only a single cell is selected, or a range of cells */
bool AgendaView::selectedIsSingleCell() const
{
    if (!selectionStart().isValid() || !selectionEnd().isValid()) {
        return false;
    }

    if (selectedIsAllDay()) {
        int days = selectionStart().daysTo(selectionEnd());
        return days < 1;
    } else {
        int secs = selectionStart().secsTo(selectionEnd());
        return secs <= 24 * 60 * 60 / d->mAgenda->rows();
    }
}

void AgendaView::updateView()
{
    fillAgenda();
}

/*
  Update configuration settings for the agenda view. This method is not
  complete.
*/
void AgendaView::updateConfig()
{
    // Agenda can be null if setPreferences() is called inside the ctor
    // We don't need to update anything in this case.
    if (d->mAgenda && d->mAllDayAgenda) {
        d->mAgenda->updateConfig();
        d->mAllDayAgenda->updateConfig();
        d->mTimeLabelsZone->setPreferences(preferences());
        d->mTimeLabelsZone->updateAll();
        updateTimeBarWidth();
        setHolidayMasks();
        createDayLabels(true);
        setChanges(changes() | ConfigChanged);
        updateView();
    }
}

void AgendaView::createTimeBarHeaders()
{
    qDeleteAll(d->mTimeBarHeaders);
    d->mTimeBarHeaders.clear();

    const QFont oldFont(font());
    QFont labelFont = d->mTimeLabelsZone->preferences()->agendaTimeLabelsFont();
    labelFont.setPointSize(labelFont.pointSize() - SHRINKDOWN);

    const auto lst = d->mTimeLabelsZone->timeLabels();
    for (QScrollArea *area : lst) {
        auto timeLabel = static_cast<TimeLabels *>(area->widget());
        auto label = new QLabel(timeLabel->header().replace(QLatin1Char('/'), QStringLiteral("/ ")), d->mTimeBarHeaderFrame);
        d->mTimeBarHeaderFrame->layout()->addWidget(label);
        label->setFont(labelFont);
        label->setAlignment(Qt::AlignBottom | Qt::AlignRight);
        label->setContentsMargins(0, 0, 0, 0);
        label->setWordWrap(true);
        label->setToolTip(timeLabel->headerToolTip());
        d->mTimeBarHeaders.append(label);
    }
    setFont(oldFont);
}

void AgendaView::updateTimeBarWidth()
{
    if (d->mIsSideBySide) {
        return;
    }

    createTimeBarHeaders();

    const QFont oldFont(font());
    QFont labelFont = d->mTimeLabelsZone->preferences()->agendaTimeLabelsFont();
    labelFont.setPointSize(labelFont.pointSize() - SHRINKDOWN);
    QFontMetrics fm(labelFont);

    int width = d->mTimeLabelsZone->preferedTimeLabelsWidth();
    for (QLabel *l : std::as_const(d->mTimeBarHeaders)) {
        const auto lst = l->text().split(QLatin1Char(' '));
        for (const QString &word : lst) {
            width = qMax(width, fm.boundingRect(word).width());
        }
    }
    setFont(oldFont);

    width = width + fm.boundingRect(QLatin1Char('/')).width();

    const int timeBarWidth = width * d->mTimeBarHeaders.count();

    d->mTimeBarHeaderFrame->setFixedWidth(timeBarWidth - SPACING);
    d->mTimeLabelsZone->setFixedWidth(timeBarWidth);
    if (d->mDummyAllDayLeft) {
        d->mDummyAllDayLeft->setFixedWidth(0);
    }

    d->mTopDayLabelsFrame->setWeekWidth(timeBarWidth);
    d->mBottomDayLabelsFrame->setWeekWidth(timeBarWidth);
}

void AgendaView::updateEventDates(AgendaItem *item, bool addIncidence, Akonadi::Collection::Id collectionId)
{
    qCDebug(CALENDARVIEW_LOG) << item->text() << "; item->cellXLeft(): " << item->cellXLeft() << "; item->cellYTop(): " << item->cellYTop()
                              << "; item->lastMultiItem(): " << item->lastMultiItem() << "; item->itemPos(): " << item->itemPos()
                              << "; item->itemCount(): " << item->itemCount();

    QDateTime startDt;
    QDateTime endDt;

    // Start date of this incidence, calculate the offset from it
    // (so recurring and non-recurring items can be treated exactly the same,
    // we never need to check for recurs(), because we only move the start day
    // by the number of days the agenda item was really moved. Smart, isn't it?)
    QDate thisDate;
    if (item->cellXLeft() < 0) {
        thisDate = (d->mSelectedDates.first()).addDays(item->cellXLeft());
    } else {
        thisDate = d->mSelectedDates[item->cellXLeft()];
    }
    int daysOffset = 0;

    // daysOffset should only be calculated if item->cellXLeft() is positive which doesn't happen
    // if the event's start isn't visible.
    if (item->cellXLeft() >= 0) {
        daysOffset = item->occurrenceDate().daysTo(thisDate);
    }

    int daysLength = 0;

    KCalendarCore::Incidence::Ptr incidence = item->incidence();
    Akonadi::Item aitem = d->mViewCalendar->item(incidence);
    if ((!aitem.isValid() && !addIncidence) || !incidence || !changer()) {
        qCWarning(CALENDARVIEW_LOG) << "changer is " << changer() << " and incidence is " << incidence.data();
        return;
    }

    QTime startTime(0, 0, 0);
    QTime endTime(0, 0, 0);
    if (incidence->allDay()) {
        daysLength = item->cellWidth() - 1;
    } else {
        startTime = d->mAgenda->gyToTime(item->cellYTop());
        if (item->lastMultiItem()) {
            endTime = d->mAgenda->gyToTime(item->lastMultiItem()->cellYBottom() + 1);
            daysLength = item->lastMultiItem()->cellXLeft() - item->cellXLeft();
        } else if (item->itemPos() == item->itemCount() && item->itemCount() > 1) {
            /* multiitem handling in agenda assumes two things:
               - The start (first KOAgendaItem) is always visible.
               - The first KOAgendaItem of the incidence has a non-null item->lastMultiItem()
                   pointing to the last KOagendaItem.

              But those aren't always met, for example when in day-view.
              kolab/issue4417
             */

            // Cornercase 1: - Resizing the end of the event but the start isn't visible
            endTime = d->mAgenda->gyToTime(item->cellYBottom() + 1);
            daysLength = item->itemCount() - 1;
            startTime = incidence->dtStart().time();
        } else if (item->itemPos() == 1 && item->itemCount() > 1) {
            // Cornercase 2: - Resizing the start of the event but the end isn't visible
            endTime = incidence->dateTime(KCalendarCore::Incidence::RoleEnd).time();
            daysLength = item->itemCount() - 1;
        } else {
            endTime = d->mAgenda->gyToTime(item->cellYBottom() + 1);
        }
    }

    // FIXME: use a visitor here
    if (const KCalendarCore::Event::Ptr ev = CalendarSupport::event(incidence)) {
        startDt = incidence->dtStart();
        // convert to calendar timespec because we then manipulate it
        // with time coming from the calendar
        startDt = startDt.toLocalTime();
        startDt = startDt.addDays(daysOffset);
        if (!incidence->allDay()) {
            startDt.setTime(startTime);
        }
        endDt = startDt.addDays(daysLength);
        if (!incidence->allDay()) {
            endDt.setTime(endTime);
        }
        if (incidence->dtStart().toLocalTime() == startDt && ev->dtEnd().toLocalTime() == endDt) {
            // No change
            QTimer::singleShot(0, this, &AgendaView::updateView);
            return;
        }
        /* setDtEnd() must be called before setDtStart(), otherwise, when moving
         * events, CalendarLocal::incidenceUpdated() will not remove the old hash
         * and that causes the event to be shown in the old date also (bug #179157).
         *
         * TODO: We need a better hashing mechanism for CalendarLocal.
         */
        ev->setDtEnd(endDt.toTimeSpec(incidence->dateTime(KCalendarCore::Incidence::RoleEnd).timeSpec()));
        incidence->setDtStart(startDt.toTimeSpec(incidence->dtStart().timeSpec()));
    } else if (const KCalendarCore::Todo::Ptr td = CalendarSupport::todo(incidence)) {
        endDt = td->dtDue(true).toLocalTime().addDays(daysOffset);
        endDt.setTime(td->allDay() ? QTime(00, 00, 00) : endTime);

        if (td->dtDue(true).toLocalTime() == endDt) {
            // No change
            QMetaObject::invokeMethod(this, &AgendaView::updateView, Qt::QueuedConnection);
            return;
        }

        const auto shift = td->dtDue(true).secsTo(endDt);
        startDt = td->dtStart(true).addSecs(shift);
        if (td->hasStartDate()) {
            td->setDtStart(startDt.toTimeSpec(incidence->dtStart().timeSpec()));
        }
        if (td->recurs()) {
            td->setDtRecurrence(td->dtRecurrence().addSecs(shift));
        }
        td->setDtDue(endDt.toTimeSpec(td->dtDue().timeSpec()), true);
    }

    if (!incidence->hasRecurrenceId()) {
        item->setOccurrenceDateTime(startDt);
    }

    bool result;
    if (addIncidence) {
        Akonadi::Collection collection = calendar()->collection(collectionId);
        result = changer()->createIncidence(incidence, collection, this) != -1;
    } else {
        KCalendarCore::Incidence::Ptr oldIncidence(Akonadi::CalendarUtils::incidence(aitem));
        aitem.setPayload<KCalendarCore::Incidence::Ptr>(incidence);
        result = changer()->modifyIncidence(aitem, oldIncidence, this) != -1;
    }

    // Update the view correctly if an agenda item move was aborted by
    // cancelling one of the subsequent dialogs.
    if (!result) {
        setChanges(changes() | IncidencesEdited);
        QMetaObject::invokeMethod(this, &AgendaView::updateView, Qt::QueuedConnection);
        return;
    }

    // don't update the agenda as the item already has the correct coordinates.
    // an update would delete the current item and recreate it, but we are still
    // using a pointer to that item! => CRASH
    enableAgendaUpdate(false);
    // We need to do this in a timer to make sure we are not deleting the item
    // we are currently working on, which would lead to crashes
    // Only the actually moved agenda item is already at the correct position and mustn't be
    // recreated. All others have to!!!
    if (incidence->recurs() || incidence->hasRecurrenceId()) {
        d->mUpdateItem = aitem;
        QMetaObject::invokeMethod(this, &AgendaView::updateView, Qt::QueuedConnection);
    }

    enableAgendaUpdate(true);
}

QDate AgendaView::startDate() const
{
    if (d->mSelectedDates.isEmpty()) {
        return {};
    }
    return d->mSelectedDates.first();
}

QDate AgendaView::endDate() const
{
    if (d->mSelectedDates.isEmpty()) {
        return {};
    }
    return d->mSelectedDates.last();
}

void AgendaView::showDates(const QDate &start, const QDate &end, const QDate &preferredMonth)
{
    Q_UNUSED(preferredMonth)
    if (!d->mSelectedDates.isEmpty() && d->mSelectedDates.first() == start && d->mSelectedDates.last() == end) {
        return;
    }

    if (!start.isValid() || !end.isValid() || start > end || start.daysTo(end) > MAX_DAY_COUNT) {
        qCWarning(CALENDARVIEW_LOG) << "got bizarre parameters: " << start << end << " - aborting here";
        return;
    }

    d->mSelectedDates = d->generateDateList(start, end);

    // and update the view
    setChanges(changes() | DatesChanged);
    fillAgenda();
    d->mTimeLabelsZone->update();
}

void AgendaView::showIncidences(const Akonadi::Item::List &incidences, const QDate &date)
{
    Q_UNUSED(date)

    if (!calendar()) {
        qCCritical(CALENDARVIEW_LOG) << "No Calendar set";
        return;
    }

    // we must check if they are not filtered; if they are, remove the filter
    KCalendarCore::CalFilter *filter = calendar()->filter();
    bool wehaveall = true;
    if (filter) {
        for (const Akonadi::Item &aitem : incidences) {
            if (!(wehaveall = filter->filterIncidence(Akonadi::CalendarUtils::incidence(aitem)))) {
                break;
            }
        }
    }

    if (!wehaveall) {
        calendar()->setFilter(nullptr);
    }

    QDateTime start = Akonadi::CalendarUtils::incidence(incidences.first())->dtStart().toLocalTime();
    QDateTime end = Akonadi::CalendarUtils::incidence(incidences.first())->dateTime(KCalendarCore::Incidence::RoleEnd).toLocalTime();
    Akonadi::Item first = incidences.first();
    for (const Akonadi::Item &aitem : incidences) {
        if (Akonadi::CalendarUtils::incidence(aitem)->dtStart().toLocalTime() < start) {
            first = aitem;
        }
        start = qMin(start, Akonadi::CalendarUtils::incidence(aitem)->dtStart().toLocalTime());
        end = qMax(start, Akonadi::CalendarUtils::incidence(aitem)->dateTime(KCalendarCore::Incidence::RoleEnd).toLocalTime());
    }

    end.toTimeZone(start.timeZone()); // allow direct comparison of dates
    if (start.date().daysTo(end.date()) + 1 <= currentDateCount()) {
        showDates(start.date(), end.date());
    } else {
        showDates(start.date(), start.date().addDays(currentDateCount() - 1));
    }

    d->mAgenda->selectItem(first);
}

void AgendaView::fillAgenda()
{
    if (changes() == NothingChanged) {
        return;
    }

    if (d->mViewCalendar->calendars() == 0) {
        qCWarning(CALENDARVIEW_LOG) << "No calendar is set";
        return;
    }

    /*
    qCDebug(CALENDARVIEW_LOG) << "changes = " << changes()
             << "; mUpdateAgenda = " << d->mUpdateAgenda
             << "; mUpdateAllDayAgenda = " << d->mUpdateAllDayAgenda; */

    /* Remember the item Ids of the selected items. In case one of the
     * items was deleted and re-added, we want to reselect it. */
    const QString selectedAgendaId = d->mAgenda->lastSelectedItemUid();
    const QString selectedAllDayAgendaId = d->mAllDayAgenda->lastSelectedItemUid();

    enableAgendaUpdate(true);
    d->clearView();

    if (changes().testFlag(DatesChanged)) {
        d->mAllDayAgenda->changeColumns(d->mSelectedDates.count());
        d->mAgenda->changeColumns(d->mSelectedDates.count());
        d->changeColumns(d->mSelectedDates.count());

        createDayLabels(false);
        setHolidayMasks();

        d->mAgenda->setDateList(d->mSelectedDates);
    }

    setChanges(NothingChanged);

    bool somethingReselected = false;
    const KCalendarCore::Incidence::List incidences = d->mViewCalendar->incidences();

    for (const KCalendarCore::Incidence::Ptr &incidence : incidences) {
        Q_ASSERT(incidence);
        const bool wasSelected = (incidence->uid() == selectedAgendaId) || (incidence->uid() == selectedAllDayAgendaId);

        if ((incidence->allDay() && d->mUpdateAllDayAgenda) || (!incidence->allDay() && d->mUpdateAgenda)) {
            displayIncidence(incidence, wasSelected);
        }

        if (wasSelected) {
            somethingReselected = true;
        }
    }

    d->mAgenda->checkScrollBoundaries();
    updateEventIndicators();

    //  mAgenda->viewport()->update();
    //  mAllDayAgenda->viewport()->update();

    // make invalid
    deleteSelectedDateTime();

    d->mUpdateAgenda = false;
    d->mUpdateAllDayAgenda = false;

    if (!somethingReselected) {
        Q_EMIT incidenceSelected(Akonadi::Item(), QDate());
    }
}

bool AgendaView::displayIncidence(const KCalendarCore::Incidence::Ptr &incidence, bool createSelected)
{
    if (!incidence) {
        return false;
    }

    if (incidence->hasRecurrenceId()) {
        // Normally a disassociated instance belongs to a recurring instance that
        // displays it.
        if (calendar2(incidence)->incidence(incidence->uid())) {
            return false;
        }
    }

    KCalendarCore::Todo::Ptr todo = CalendarSupport::todo(incidence);
    if (todo && (!preferences()->showTodosAgendaView() || !todo->hasDueDate())) {
        return false;
    }

    KCalendarCore::Event::Ptr event = CalendarSupport::event(incidence);
    const QDate today = QDate::currentDate();

    QDateTime firstVisibleDateTime(d->mSelectedDates.first(), QTime(0, 0, 0), Qt::LocalTime);
    QDateTime lastVisibleDateTime(d->mSelectedDates.last(), QTime(23, 59, 59, 999), Qt::LocalTime);

    // Optimization, very cheap operation that discards incidences that aren't in the timespan
    if (!d->mightBeVisible(incidence)) {
        return false;
    }

    std::vector<QDateTime> dateTimeList;

    const QDateTime incDtStart = incidence->dtStart().toLocalTime();
    const QDateTime incDtEnd = incidence->dateTime(KCalendarCore::Incidence::RoleEnd).toLocalTime();

    bool alreadyAddedToday = false;

    if (incidence->recurs()) {
        // timed incidences occur in [dtStart(), dtEnd()[
        // all-day incidences occur in [dtStart(), dtEnd()]
        // so we subtract 1 second in the timed case
        const int secsToAdd = incidence->allDay() ? 0 : -1;
        const int eventDuration = event ? incDtStart.daysTo(incDtEnd.addSecs(secsToAdd)) : 0;

        // if there's a multiday event that starts before firstVisibleDateTime but ends after
        // lets include it. timesInInterval() ignores incidences that aren't totaly inside
        // the range
        const QDateTime startDateTimeWithOffset = firstVisibleDateTime.addDays(-eventDuration);

        KCalendarCore::OccurrenceIterator rIt(*calendar(), incidence, startDateTimeWithOffset, lastVisibleDateTime);
        while (rIt.hasNext()) {
            rIt.next();
            auto occurrenceDate = rIt.occurrenceStartDate().toLocalTime();
            if (const auto todo = CalendarSupport::todo(rIt.incidence())) {
                // Recurrence exceptions may have durations different from the normal recurrences.
                occurrenceDate = occurrenceDate.addSecs(todo->dtStart().secsTo(todo->dtDue()));
            }
            const bool makesDayBusy = preferences()->colorAgendaBusyDays() && makesWholeDayBusy(rIt.incidence());
            if (makesDayBusy) {
                KCalendarCore::Event::List &busyEvents = d->mBusyDays[occurrenceDate.date()];
                busyEvents.append(event);
            }

            if (occurrenceDate.date() == today) {
                alreadyAddedToday = true;
            }
            d->insertIncidence(rIt.incidence(), rIt.recurrenceId(), occurrenceDate, createSelected);
        }
    } else {
        QDateTime dateToAdd; // date to add to our date list
        QDateTime incidenceEnd;

        if (todo && todo->hasDueDate() && !todo->isOverdue()) {
            // If it's not overdue it will be shown at the original date (not today)
            dateToAdd = todo->dtDue().toLocalTime();

            // To-dos due at a specific time are drawn with the bottom of the rectangle at dtDue.
            // If dtDue is at 00:00, then it should be displayed in the previous day, at 23:59.
            if (!todo->allDay() && dateToAdd.time() == QTime(0, 0)) {
                dateToAdd = dateToAdd.addSecs(-1);
            }

            incidenceEnd = dateToAdd;
        } else if (event) {
            dateToAdd = incDtStart;
            incidenceEnd = incDtEnd;
        }

        if (dateToAdd.isValid() && incidence->allDay()) {
            // so comparisons with < > actually work
            dateToAdd.setTime(QTime(0, 0));
            incidenceEnd.setTime(QTime(23, 59, 59, 999));
        }

        if (dateToAdd <= lastVisibleDateTime && incidenceEnd > firstVisibleDateTime) {
            dateTimeList.push_back(dateToAdd);
        }
    }

    // ToDo items shall be displayed today if they are overdue
    const QDateTime dateTimeToday = QDateTime(today, QTime(0, 0), Qt::LocalTime);
    if (todo && todo->isOverdue() && dateTimeToday >= firstVisibleDateTime && dateTimeToday <= lastVisibleDateTime) {
        /* If there's a recurring instance showing up today don't add "today" again
         * we don't want the event to appear duplicated */
        if (!alreadyAddedToday) {
            dateTimeList.push_back(dateTimeToday);
        }
    }

    const bool makesDayBusy = preferences()->colorAgendaBusyDays() && makesWholeDayBusy(incidence);
    for (auto t = dateTimeList.begin(); t != dateTimeList.end(); ++t) {
        if (makesDayBusy) {
            KCalendarCore::Event::List &busyEvents = d->mBusyDays[(*t).date()];
            busyEvents.append(event);
        }

        d->insertIncidence(incidence, t->toLocalTime(), t->toLocalTime(), createSelected);
    }

    // Can be multiday
    if (event && makesDayBusy && event->isMultiDay()) {
        const QDate lastVisibleDate = d->mSelectedDates.last();
        for (QDate date = event->dtStart().date(); date <= event->dtEnd().date() && date <= lastVisibleDate; date = date.addDays(1)) {
            KCalendarCore::Event::List &busyEvents = d->mBusyDays[date];
            busyEvents.append(event);
        }
    }

    return !dateTimeList.empty();
}

void AgendaView::updateEventIndicatorTop(int newY)
{
    for (int i = 0; i < d->mMinY.size(); ++i) {
        d->mEventIndicatorTop->enableColumn(i, newY > d->mMinY[i]);
    }
    d->mEventIndicatorTop->update();
}

void AgendaView::updateEventIndicatorBottom(int newY)
{
    for (int i = 0; i < d->mMaxY.size(); ++i) {
        d->mEventIndicatorBottom->enableColumn(i, newY <= d->mMaxY[i]);
    }
    d->mEventIndicatorBottom->update();
}

void AgendaView::slotIncidencesDropped(const QList<QUrl> &items, const QPoint &gpos, bool allDay)
{
    Q_UNUSED(items)
    Q_UNUSED(gpos)
    Q_UNUSED(allDay)

#ifdef AKONADI_PORT_DISABLED // one item -> multiple items, Incidence* -> akonadi item url
                             // (we might have to fetch the items here first!)
    if (gpos.x() < 0 || gpos.y() < 0) {
        return;
    }

    const QDate day = d->mSelectedDates[gpos.x()];
    const QTime time = d->mAgenda->gyToTime(gpos.y());
    KDateTime newTime(day, time, preferences()->timeSpec());
    newTime.setDateOnly(allDay);

    Todo::Ptr todo = Akonadi::CalendarUtils4::todo(todoItem);
    if (todo && dynamic_cast<Akonadi::ETMCalendar *>(calendar())) {
        const Akonadi::Item existingTodoItem = calendar()->itemForIncidence(calendar()->todo(todo->uid()));

        if (Todo::Ptr existingTodo = Akonadi::CalendarUtils::todo(existingTodoItem)) {
            qCDebug(CALENDARVIEW_LOG) << "Drop existing Todo";
            Todo::Ptr oldTodo(existingTodo->clone());
            if (changer()) {
                existingTodo->setDtDue(newTime);
                existingTodo->setAllDay(allDay);
                changer()->modifyIncidence(existingTodoItem, oldTodo, this);
            } else {
                KMessageBox::error(this,
                                   i18n("Unable to modify this to-do, "
                                        "because it cannot be locked."));
            }
        } else {
            qCDebug(CALENDARVIEW_LOG) << "Drop new Todo";
            todo->setDtDue(newTime);
            todo->setAllDay(allDay);
            if (!changer()->addIncidence(todo, this)) {
                KMessageBox::error(this, i18n("Unable to save %1 \"%2\".", i18n(todo->type()), todo->summary()));
            }
        }
    }
#else
    qCDebug(CALENDARVIEW_LOG) << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

static void setDateTime(KCalendarCore::Incidence::Ptr incidence, const QDateTime &dt, bool allDay)
{
    incidence->setAllDay(allDay);

    if (auto todo = CalendarSupport::todo(incidence)) {
        // To-dos are displayed on their due date and time.  Make sure the todo is displayed
        // where it was dropped.
        QDateTime dtStart = todo->dtStart();
        if (dtStart.isValid()) {
            auto duration = todo->dtStart().daysTo(todo->dtDue());
            dtStart = dt.addDays(-duration);
            dtStart.setTime({0, 0, 0});
        }
        // Set dtDue before dtStart;  see comment in updateEventDates().
        todo->setDtDue(dt, true);
        todo->setDtStart(dtStart);
    } else if (auto event = CalendarSupport::event(incidence)) {
        auto duration = event->dtStart().secsTo(event->dtEnd());
        if (duration == 0) {
            auto defaultDuration = CalendarSupport::KCalPrefs::instance()->defaultDuration().time();
            duration = (defaultDuration.hour() * 3600) + (defaultDuration.minute() * 60);
        }
        event->setDtEnd(dt.addSecs(duration));
        event->setDtStart(dt);
    } else { // Can't happen, but ...
        incidence->setDtStart(dt);
    }
}

void AgendaView::slotIncidencesDropped(const KCalendarCore::Incidence::List &incidences, const QPoint &gpos, bool allDay)
{
    if (gpos.x() < 0 || gpos.y() < 0) {
        return;
    }

    const QDate day = d->mSelectedDates[gpos.x()];
    const QTime time = d->mAgenda->gyToTime(gpos.y());
    QDateTime newTime(day, time, Qt::LocalTime);

    for (const KCalendarCore::Incidence::Ptr &incidence : incidences) {
        const Akonadi::Item existingItem = calendar()->item(incidence);
        const bool existsInSameCollection = existingItem.isValid() && (existingItem.storageCollectionId() == collectionId() || collectionId() == -1);

        if (existingItem.isValid() && existsInSameCollection) {
            auto newIncidence = existingItem.payload<KCalendarCore::Incidence::Ptr>();

            if (newIncidence->dtStart() == newTime && newIncidence->allDay() == allDay) {
                // Nothing changed
                continue;
            }

            KCalendarCore::Incidence::Ptr oldIncidence(newIncidence->clone());
            setDateTime(newIncidence, newTime, allDay);

            (void)changer()->modifyIncidence(existingItem, oldIncidence, this);
        } else { // Create a new one
            // The drop came from another application.  Create a new incidence.
            setDateTime(incidence, newTime, allDay);
            incidence->setUid(KCalendarCore::CalFormat::createUniqueId());
            Akonadi::Collection collection(collectionId());
            const bool added = -1 != changer()->createIncidence(incidence, collection, this);

            if (added) {
                // TODO: make async
                if (existingItem.isValid()) { // Dragged from one agenda to another, delete origin
                    (void)changer()->deleteIncidence(existingItem);
                }
            }
        }
    }
}

void AgendaView::startDrag(const KCalendarCore::Incidence::Ptr &incidence)
{
    if (!calendar()) {
        qCCritical(CALENDARVIEW_LOG) << "No Calendar set";
        return;
    }

    const Akonadi::Item item = d->mViewCalendar->item(incidence);
    if (item.isValid()) {
        startDrag(item);
    }
}

void AgendaView::startDrag(const Akonadi::Item &incidence)
{
    if (!calendar()) {
        qCCritical(CALENDARVIEW_LOG) << "No Calendar set";
        return;
    }
    if (QDrag *drag = CalendarSupport::createDrag(incidence, this)) {
        drag->exec();
    }
}

void AgendaView::readSettings()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    readSettings(config.data());
}

void AgendaView::readSettings(const KConfig *config)
{
    const KConfigGroup group = config->group("Views");

    const QList<int> sizes = group.readEntry("Separator AgendaView", QList<int>());

    // the size depends on the number of plugins used
    // we don't want to read invalid/corrupted settings or else agenda becomes invisible
    if (sizes.count() >= 2 && !sizes.contains(0)) {
        d->mSplitterAgenda->setSizes(sizes);
        updateConfig();
    }
}

void AgendaView::writeSettings(KConfig *config)
{
    KConfigGroup group = config->group("Views");

    QList<int> list = d->mSplitterAgenda->sizes();
    group.writeEntry("Separator AgendaView", list);
}

QList<bool> AgendaView::busyDayMask() const
{
    if (d->mSelectedDates.isEmpty() || !d->mSelectedDates[0].isValid()) {
        return {};
    }

    QList<bool> busyDayMask;
    busyDayMask.resize(d->mSelectedDates.count());

    for (int i = 0; i < d->mSelectedDates.count(); ++i) {
        busyDayMask[i] = !d->mBusyDays[d->mSelectedDates[i]].isEmpty();
    }

    return busyDayMask;
}

void AgendaView::setHolidayMasks()
{
    if (d->mSelectedDates.isEmpty() || !d->mSelectedDates[0].isValid()) {
        return;
    }

    d->mHolidayMask.resize(d->mSelectedDates.count() + 1);

    const QList<QDate> workDays = CalendarSupport::workDays(d->mSelectedDates.constFirst().addDays(-1), d->mSelectedDates.last());
    for (int i = 0; i < d->mSelectedDates.count(); ++i) {
        d->mHolidayMask[i] = !workDays.contains(d->mSelectedDates[i]);
    }

    // Store the information about the day before the visible area (needed for
    // overnight working hours) in the last bit of the mask:
    bool showDay = !workDays.contains(d->mSelectedDates[0].addDays(-1));
    d->mHolidayMask[d->mSelectedDates.count()] = showDay;

    d->mAgenda->setHolidayMask(&d->mHolidayMask);
    d->mAllDayAgenda->setHolidayMask(&d->mHolidayMask);
}

void AgendaView::clearSelection()
{
    d->mAgenda->deselectItem();
    d->mAllDayAgenda->deselectItem();
}

void AgendaView::newTimeSpanSelectedAllDay(const QPoint &start, const QPoint &end)
{
    newTimeSpanSelected(start, end);
    d->mTimeSpanInAllDay = true;
}

void AgendaView::newTimeSpanSelected(const QPoint &start, const QPoint &end)
{
    if (!d->mSelectedDates.count()) {
        return;
    }

    d->mTimeSpanInAllDay = false;

    const QDate dayStart = d->mSelectedDates[qBound(0, start.x(), (int)d->mSelectedDates.size() - 1)];
    const QDate dayEnd = d->mSelectedDates[qBound(0, end.x(), (int)d->mSelectedDates.size() - 1)];

    const QTime timeStart = d->mAgenda->gyToTime(start.y());
    const QTime timeEnd = d->mAgenda->gyToTime(end.y() + 1);

    d->mTimeSpanBegin = QDateTime(dayStart, timeStart);
    d->mTimeSpanEnd = QDateTime(dayEnd, timeEnd);
}

QDateTime AgendaView::selectionStart() const
{
    return d->mTimeSpanBegin;
}

QDateTime AgendaView::selectionEnd() const
{
    return d->mTimeSpanEnd;
}

bool AgendaView::selectedIsAllDay() const
{
    return d->mTimeSpanInAllDay;
}

void AgendaView::deleteSelectedDateTime()
{
    d->mTimeSpanBegin.setDate(QDate());
    d->mTimeSpanEnd.setDate(QDate());
    d->mTimeSpanInAllDay = false;
}

void AgendaView::removeIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    // Don't wrap this in a if (incidence->isAllDay) because all day
    // property might have changed
    d->mAllDayAgenda->removeIncidence(incidence);
    d->mAgenda->removeIncidence(incidence);

    if (!incidence->hasRecurrenceId() && d->mViewCalendar->isValid(incidence->uid())) {
        // Deleted incidence is an main incidence
        // Delete all exceptions as well
        const KCalendarCore::Incidence::List exceptions = calendar2(incidence->uid())->instances(incidence);
        for (const KCalendarCore::Incidence::Ptr &exception : exceptions) {
            if (exception->allDay()) {
                d->mAllDayAgenda->removeIncidence(exception);
            } else {
                d->mAgenda->removeIncidence(exception);
            }
        }
    }
}

void AgendaView::updateEventIndicators()
{
    d->mUpdateEventIndicatorsScheduled = false;
    d->mMinY = d->mAgenda->minContentsY();
    d->mMaxY = d->mAgenda->maxContentsY();

    d->mAgenda->checkScrollBoundaries();
    updateEventIndicatorTop(d->mAgenda->visibleContentsYMin());
    updateEventIndicatorBottom(d->mAgenda->visibleContentsYMax());
}

void AgendaView::setIncidenceChanger(Akonadi::IncidenceChanger *changer)
{
    EventView::setIncidenceChanger(changer);
    d->mAgenda->setIncidenceChanger(changer);
    d->mAllDayAgenda->setIncidenceChanger(changer);
}

void AgendaView::clearTimeSpanSelection()
{
    d->mAgenda->clearSelection();
    d->mAllDayAgenda->clearSelection();
    deleteSelectedDateTime();
}

Agenda *AgendaView::agenda() const
{
    return d->mAgenda;
}

Agenda *AgendaView::allDayAgenda() const
{
    return d->mAllDayAgenda;
}

QSplitter *AgendaView::splitter() const
{
    return d->mSplitterAgenda;
}

bool AgendaView::filterByCollectionSelection(const KCalendarCore::Incidence::Ptr &incidence)
{
    const Akonadi::Item item = d->mViewCalendar->item(incidence);

    if (!item.isValid()) {
        return true;
    }

    if (customCollectionSelection()) {
        return customCollectionSelection()->contains(item.parentCollection().id());
    }

    if (collectionId() < 0) {
        return true;
    } else {
        return collectionId() == item.storageCollectionId();
    }
}

void AgendaView::alignAgendas()
{
    // resize dummy widget so the allday agenda lines up with the hourly agenda.
    if (d->mDummyAllDayLeft) {
        d->mDummyAllDayLeft->setFixedWidth(d->mTimeLabelsZone->width() - d->mTimeBarHeaderFrame->width() - SPACING);
    }

    // Must be async, so they are centered
    createDayLabels(true);
}

void AgendaView::setChanges(EventView::Changes changes)
{
    d->setChanges(changes);
}

void AgendaView::scheduleUpdateEventIndicators()
{
    if (!d->mUpdateEventIndicatorsScheduled) {
        d->mUpdateEventIndicatorsScheduled = true;
        QTimer::singleShot(0, this, &AgendaView::updateEventIndicators);
    }
}

#include "agendaview.moc"

#include "moc_agendaview.cpp"
