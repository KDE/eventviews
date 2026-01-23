/*
  SPDX-FileCopyrightText: 2000, 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#include "agendaitem.h"
#include "eventview.h"
#include "helper.h"

#include "prefs.h"
#include "prefs_base.h" // for enums

#include <CalendarSupport/KCalPrefs>
#include <CalendarSupport/Utils>

#include <Akonadi/TagCache>

#include <KContacts/VCardDrag>

#include <KCalUtils/ICalDrag>
#include <KCalUtils/IncidenceFormatter>
#include <KCalUtils/VCalDrag>

#include <KEmailAddress>

#include <KLocalizedString>
#include <KMessageBox>
#include <KWordWrap>

#include <QDragEnterEvent>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QToolTip>

using namespace KCalendarCore;
using namespace EventViews;
using namespace Qt::Literals::StringLiterals;
//-----------------------------------------------------------------------------

QPixmap *AgendaItem::alarmPxmp = nullptr;
QPixmap *AgendaItem::recurPxmp = nullptr;
QPixmap *AgendaItem::readonlyPxmp = nullptr;
QPixmap *AgendaItem::replyPxmp = nullptr;
QPixmap *AgendaItem::groupPxmp = nullptr;
QPixmap *AgendaItem::groupPxmpTent = nullptr;
QPixmap *AgendaItem::organizerPxmp = nullptr;
QPixmap *AgendaItem::eventPxmp = nullptr;

//-----------------------------------------------------------------------------

AgendaItem::AgendaItem(EventView *eventView,
                       const MultiViewCalendar::Ptr &calendar,
                       const KCalendarCore::Incidence::Ptr &incidence,
                       int itemPos,
                       int itemCount,
                       const QDateTime &qd,
                       bool isSelected,
                       QWidget *parent)
    : QWidget(parent)
    , mEventView(eventView)
    , mCalendar(calendar)
    , mIncidence(incidence)
    , mOccurrenceDateTime(qd)
    , mSelected(isSelected)
    , mSpecialEvent(false)
{
    if (!mIncidence) {
        mValid = false;
        return;
    }

    mIncidence = Incidence::Ptr(mIncidence->clone());
    if (mIncidence->customProperty("KABC", "BIRTHDAY") == "YES"_L1 || mIncidence->customProperty("KABC", "ANNIVERSARY") == "YES"_L1) {
        const int years = EventViews::yearDiff(mIncidence->dtStart().date(), qd.toLocalTime().date());
        if (years > 0) {
            mIncidence->setReadOnly(false);
            mIncidence->setSummary(i18np("%2 (1 year)", "%2 (%1 years)", years, mIncidence->summary()));
            mIncidence->setReadOnly(true);
            mCloned = true;
        }
    }

    mLabelText = mIncidence->summary();
    mIconAlarm = false;
    mIconRecur = false;
    mIconReadonly = false;
    mIconReply = false;
    mIconGroup = false;
    mIconGroupTent = false;
    mIconOrganizer = false;
    mMultiItemInfo = nullptr;
    mStartMoveInfo = nullptr;

    mItemPos = itemPos;
    mItemCount = itemCount;

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::transparent);
    setPalette(pal);

    setCellXY(0, 0, 1);
    setCellXRight(0);
    setMouseTracking(true);
    mResourceColor = QColor();
    updateIcons();

    setAcceptDrops(true);
}

AgendaItem::~AgendaItem() = default;

void AgendaItem::updateIcons()
{
    if (!mValid) {
        return;
    }
    mIconReadonly = mIncidence->isReadOnly();
    mIconRecur = mIncidence->recurs() || mIncidence->hasRecurrenceId();
    mIconAlarm = mIncidence->hasEnabledAlarms();
    if (mIncidence->attendeeCount() > 1) {
        if (mEventView->kcalPreferences()->thatIsMe(mIncidence->organizer().email())) {
            mIconReply = false;
            mIconGroup = false;
            mIconGroupTent = false;
            mIconOrganizer = true;
        } else {
            KCalendarCore::Attendee const me = mIncidence->attendeeByMails(mEventView->kcalPreferences()->allEmails());

            if (!me.isNull()) {
                if (me.status() == KCalendarCore::Attendee::NeedsAction && me.RSVP()) {
                    mIconReply = true;
                    mIconGroup = false;
                    mIconGroupTent = false;
                    mIconOrganizer = false;
                } else if (me.status() == KCalendarCore::Attendee::Tentative) {
                    mIconReply = false;
                    mIconGroup = false;
                    mIconGroupTent = true;
                    mIconOrganizer = false;
                } else {
                    mIconReply = false;
                    mIconGroup = true;
                    mIconGroupTent = false;
                    mIconOrganizer = false;
                }
            } else {
                mIconReply = false;
                mIconGroup = true;
                mIconGroupTent = false;
                mIconOrganizer = false;
            }
        }
    }
    update();
}

void AgendaItem::select(bool selected)
{
    if (mSelected != selected) {
        mSelected = selected;
        update();
    }
}

bool AgendaItem::dissociateFromMultiItem()
{
    if (!isMultiItem()) {
        return false;
    }

    AgendaItem::QPtr firstItem = firstMultiItem();
    if (firstItem == this) {
        firstItem = nextMultiItem();
    }

    AgendaItem::QPtr lastItem = lastMultiItem();
    if (lastItem == this) {
        lastItem = prevMultiItem();
    }

    AgendaItem::QPtr const prevItem = prevMultiItem();
    AgendaItem::QPtr const nextItem = nextMultiItem();

    if (prevItem) {
        prevItem->setMultiItem(firstItem, prevItem->prevMultiItem(), nextItem, lastItem);
    }
    if (nextItem) {
        nextItem->setMultiItem(firstItem, prevItem, nextItem->prevMultiItem(), lastItem);
    }
    delete mMultiItemInfo;
    mMultiItemInfo = nullptr;
    return true;
}

void AgendaItem::setIncidence(const KCalendarCore::Incidence::Ptr &incidence)
{
    mValid = false;
    if (incidence) {
        mValid = true;
        mIncidence = incidence;
        mLabelText = mIncidence->summary();
        updateIcons();
    }
}

/*
  Return height of item in units of agenda cells
*/
int AgendaItem::cellHeight() const
{
    return mCellYBottom - mCellYTop + 1;
}

/*
  Return height of item in units of agenda cells
*/
int AgendaItem::cellWidth() const
{
    return mCellXRight - mCellXLeft + 1;
}

void AgendaItem::setOccurrenceDateTime(const QDateTime &qd)
{
    mOccurrenceDateTime = qd;
}

QDate AgendaItem::occurrenceDate() const
{
    return mOccurrenceDateTime.toLocalTime().date();
}

void AgendaItem::setCellXY(int X, int YTop, int YBottom)
{
    mCellXLeft = X;
    mCellYTop = YTop;
    mCellYBottom = YBottom;
}

void AgendaItem::setCellXRight(int XRight)
{
    mCellXRight = XRight;
}

void AgendaItem::setCellX(int XLeft, int XRight)
{
    mCellXLeft = XLeft;
    mCellXRight = XRight;
}

void AgendaItem::setCellY(int YTop, int YBottom)
{
    mCellYTop = YTop;
    mCellYBottom = YBottom;
}

void AgendaItem::setMultiItem(const AgendaItem::QPtr &first, const AgendaItem::QPtr &prev, const AgendaItem::QPtr &next, const AgendaItem::QPtr &last)
{
    if (!mMultiItemInfo) {
        mMultiItemInfo = new MultiItemInfo;
    }
    mMultiItemInfo->mFirstMultiItem = first;
    mMultiItemInfo->mPrevMultiItem = prev;
    mMultiItemInfo->mNextMultiItem = next;
    mMultiItemInfo->mLastMultiItem = last;
}

bool AgendaItem::isMultiItem() const
{
    return mMultiItemInfo;
}

AgendaItem::QPtr AgendaItem::prependMoveItem(const AgendaItem::QPtr &e)
{
    if (!e) {
        return nullptr;
    }

    AgendaItem::QPtr first = nullptr;
    AgendaItem::QPtr last = nullptr;
    if (isMultiItem()) {
        first = mMultiItemInfo->mFirstMultiItem;
        last = mMultiItemInfo->mLastMultiItem;
    }
    if (!first) {
        first = this;
    }
    if (!last) {
        last = this;
    }

    e->setMultiItem(nullptr, nullptr, first, last);
    first->setMultiItem(e, e, first->nextMultiItem(), first->lastMultiItem());

    AgendaItem::QPtr tmp = first->nextMultiItem();
    while (tmp) {
        tmp->setMultiItem(e, tmp->prevMultiItem(), tmp->nextMultiItem(), tmp->lastMultiItem());
        tmp = tmp->nextMultiItem();
    }

    if (mStartMoveInfo && !e->moveInfo()) {
        e->mStartMoveInfo = new MultiItemInfo(*mStartMoveInfo);
        //    e->moveInfo()->mFirstMultiItem = moveInfo()->mFirstMultiItem;
        //    e->moveInfo()->mLastMultiItem = moveInfo()->mLastMultiItem;
        e->moveInfo()->mPrevMultiItem = nullptr;
        e->moveInfo()->mNextMultiItem = first;
    }

    if (first && first->moveInfo()) {
        first->moveInfo()->mPrevMultiItem = e;
    }
    return e;
}

AgendaItem::QPtr AgendaItem::appendMoveItem(const AgendaItem::QPtr &e)
{
    if (!e) {
        return nullptr;
    }

    AgendaItem::QPtr first = nullptr;
    AgendaItem::QPtr last = nullptr;
    if (isMultiItem()) {
        first = mMultiItemInfo->mFirstMultiItem;
        last = mMultiItemInfo->mLastMultiItem;
    }
    if (!first) {
        first = this;
    }
    if (!last) {
        last = this;
    }

    e->setMultiItem(first, last, nullptr, nullptr);
    AgendaItem::QPtr tmp = first;

    while (tmp) {
        tmp->setMultiItem(tmp->firstMultiItem(), tmp->prevMultiItem(), tmp->nextMultiItem(), e);
        tmp = tmp->nextMultiItem();
    }
    last->setMultiItem(last->firstMultiItem(), last->prevMultiItem(), e, e);

    if (mStartMoveInfo && !e->moveInfo()) {
        e->mStartMoveInfo = new MultiItemInfo(*mStartMoveInfo);
        //    e->moveInfo()->mFirstMultiItem = moveInfo()->mFirstMultiItem;
        //    e->moveInfo()->mLastMultiItem = moveInfo()->mLastMultiItem;
        e->moveInfo()->mPrevMultiItem = last;
        e->moveInfo()->mNextMultiItem = nullptr;
    }
    if (last && last->moveInfo()) {
        last->moveInfo()->mNextMultiItem = e;
    }
    return e;
}

AgendaItem::QPtr AgendaItem::removeMoveItem(const AgendaItem::QPtr &e)
{
    if (isMultiItem()) {
        AgendaItem::QPtr first = mMultiItemInfo->mFirstMultiItem;
        AgendaItem::QPtr next;
        AgendaItem::QPtr prev;
        AgendaItem::QPtr last = mMultiItemInfo->mLastMultiItem;
        if (!first) {
            first = this;
        }
        if (!last) {
            last = this;
        }
        if (first == e) {
            first = first->nextMultiItem();
            first->setMultiItem(nullptr, nullptr, first->nextMultiItem(), first->lastMultiItem());
        }
        if (last == e) {
            last = last->prevMultiItem();
            last->setMultiItem(last->firstMultiItem(), last->prevMultiItem(), nullptr, nullptr);
        }

        AgendaItem::QPtr tmp = first;
        if (first == last) {
            delete mMultiItemInfo;
            tmp = nullptr;
            mMultiItemInfo = nullptr;
        }
        while (tmp) {
            next = tmp->nextMultiItem();
            prev = tmp->prevMultiItem();
            if (e == next) {
                next = next->nextMultiItem();
            }
            if (e == prev) {
                prev = prev->prevMultiItem();
            }
            tmp->setMultiItem((tmp == first) ? nullptr : first, (tmp == prev) ? nullptr : prev, (tmp == next) ? nullptr : next, (tmp == last) ? nullptr : last);
            tmp = tmp->nextMultiItem();
        }
    }

    return e;
}

void AgendaItem::startMove()
{
    AgendaItem::QPtr first = this;
    if (isMultiItem() && mMultiItemInfo->mFirstMultiItem) {
        first = mMultiItemInfo->mFirstMultiItem;
    }
    first->startMovePrivate();
}

void AgendaItem::startMovePrivate()
{
    mStartMoveInfo = new MultiItemInfo;
    mStartMoveInfo->mStartCellXLeft = mCellXLeft;
    mStartMoveInfo->mStartCellXRight = mCellXRight;
    mStartMoveInfo->mStartCellYTop = mCellYTop;
    mStartMoveInfo->mStartCellYBottom = mCellYBottom;
    if (mMultiItemInfo) {
        mStartMoveInfo->mFirstMultiItem = mMultiItemInfo->mFirstMultiItem;
        mStartMoveInfo->mLastMultiItem = mMultiItemInfo->mLastMultiItem;
        mStartMoveInfo->mPrevMultiItem = mMultiItemInfo->mPrevMultiItem;
        mStartMoveInfo->mNextMultiItem = mMultiItemInfo->mNextMultiItem;
    } else {
        mStartMoveInfo->mFirstMultiItem = nullptr;
        mStartMoveInfo->mLastMultiItem = nullptr;
        mStartMoveInfo->mPrevMultiItem = nullptr;
        mStartMoveInfo->mNextMultiItem = nullptr;
    }
    if (isMultiItem() && mMultiItemInfo->mNextMultiItem) {
        mMultiItemInfo->mNextMultiItem->startMovePrivate();
    }
}

void AgendaItem::resetMove()
{
    if (mStartMoveInfo) {
        if (mStartMoveInfo->mFirstMultiItem) {
            mStartMoveInfo->mFirstMultiItem->resetMovePrivate();
        } else {
            resetMovePrivate();
        }
    }
}

void AgendaItem::resetMovePrivate()
{
    if (mStartMoveInfo) {
        mCellXLeft = mStartMoveInfo->mStartCellXLeft;
        mCellXRight = mStartMoveInfo->mStartCellXRight;
        mCellYTop = mStartMoveInfo->mStartCellYTop;
        mCellYBottom = mStartMoveInfo->mStartCellYBottom;

        // if we don't have mMultiItemInfo, the item didn't span two days before,
        // and wasn't moved over midnight, either, so we don't have to reset
        // anything. Otherwise, restore from mMoveItemInfo
        if (mMultiItemInfo) {
            // It was already a multi-day info
            mMultiItemInfo->mFirstMultiItem = mStartMoveInfo->mFirstMultiItem;
            mMultiItemInfo->mPrevMultiItem = mStartMoveInfo->mPrevMultiItem;
            mMultiItemInfo->mNextMultiItem = mStartMoveInfo->mNextMultiItem;
            mMultiItemInfo->mLastMultiItem = mStartMoveInfo->mLastMultiItem;

            if (!mStartMoveInfo->mFirstMultiItem) {
                // This was the first multi-item when the move started, delete all previous
                AgendaItem::QPtr toDel = mStartMoveInfo->mPrevMultiItem;
                AgendaItem::QPtr nowDel = nullptr;
                while (toDel) {
                    nowDel = toDel;
                    if (nowDel->moveInfo()) {
                        toDel = nowDel->moveInfo()->mPrevMultiItem;
                    }
                    Q_EMIT removeAgendaItem(nowDel);
                }
                mMultiItemInfo->mFirstMultiItem = nullptr;
                mMultiItemInfo->mPrevMultiItem = nullptr;
            }
            if (!mStartMoveInfo->mLastMultiItem) {
                // This was the last multi-item when the move started, delete all next
                AgendaItem::QPtr toDel = mStartMoveInfo->mNextMultiItem;
                AgendaItem::QPtr nowDel = nullptr;
                while (toDel) {
                    nowDel = toDel;
                    if (nowDel->moveInfo()) {
                        toDel = nowDel->moveInfo()->mNextMultiItem;
                    }
                    Q_EMIT removeAgendaItem(nowDel);
                }
                mMultiItemInfo->mLastMultiItem = nullptr;
                mMultiItemInfo->mNextMultiItem = nullptr;
            }

            if (mStartMoveInfo->mFirstMultiItem == nullptr && mStartMoveInfo->mLastMultiItem == nullptr) {
                // it was a single-day event before we started the move.
                delete mMultiItemInfo;
                mMultiItemInfo = nullptr;
            }
        }
        delete mStartMoveInfo;
        mStartMoveInfo = nullptr;
    }
    Q_EMIT showAgendaItem(this);
    if (nextMultiItem()) {
        nextMultiItem()->resetMovePrivate();
    }
}

void AgendaItem::endMove()
{
    AgendaItem::QPtr first = firstMultiItem();
    if (!first) {
        first = this;
    }
    first->endMovePrivate();
}

void AgendaItem::endMovePrivate()
{
    if (mStartMoveInfo) {
        // if first, delete all previous
        if (!firstMultiItem() || firstMultiItem() == this) {
            AgendaItem::QPtr toDel = mStartMoveInfo->mPrevMultiItem;
            AgendaItem::QPtr nowDel = nullptr;
            while (toDel) {
                nowDel = toDel;
                if (nowDel->moveInfo()) {
                    toDel = nowDel->moveInfo()->mPrevMultiItem;
                }
                Q_EMIT removeAgendaItem(nowDel);
            }
        }
        // if last, delete all next
        if (!lastMultiItem() || lastMultiItem() == this) {
            AgendaItem::QPtr toDel = mStartMoveInfo->mNextMultiItem;
            AgendaItem::QPtr nowDel = nullptr;
            while (toDel) {
                nowDel = toDel;
                if (nowDel->moveInfo()) {
                    toDel = nowDel->moveInfo()->mNextMultiItem;
                }
                Q_EMIT removeAgendaItem(nowDel);
            }
        }
        // also delete the moving info
        delete mStartMoveInfo;
        mStartMoveInfo = nullptr;
        if (nextMultiItem()) {
            nextMultiItem()->endMovePrivate();
        }
    }
}

void AgendaItem::moveRelative(int dx, int dy)
{
    int const newXLeft = cellXLeft() + dx;
    int const newXRight = cellXRight() + dx;
    int const newYTop = cellYTop() + dy;
    int const newYBottom = cellYBottom() + dy;
    setCellXY(newXLeft, newYTop, newYBottom);
    setCellXRight(newXRight);
}

void AgendaItem::expandTop(int dy, const bool allowOverLimit)
{
    int newYTop = cellYTop() + dy;
    int const newYBottom = cellYBottom();
    if (newYTop > newYBottom && !allowOverLimit) {
        newYTop = newYBottom;
    }
    setCellY(newYTop, newYBottom);
}

void AgendaItem::expandBottom(int dy)
{
    int const newYTop = cellYTop();
    int newYBottom = cellYBottom() + dy;
    if (newYBottom < newYTop) {
        newYBottom = newYTop;
    }
    setCellY(newYTop, newYBottom);
}

void AgendaItem::expandLeft(int dx)
{
    int newXLeft = cellXLeft() + dx;
    int const newXRight = cellXRight();
    if (newXLeft > newXRight) {
        newXLeft = newXRight;
    }
    setCellX(newXLeft, newXRight);
}

void AgendaItem::expandRight(int dx)
{
    int const newXLeft = cellXLeft();
    int newXRight = cellXRight() + dx;
    if (newXRight < newXLeft) {
        newXRight = newXLeft;
    }
    setCellX(newXLeft, newXRight);
}

void AgendaItem::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *md = e->mimeData();
    if (KCalUtils::ICalDrag::canDecode(md) || KCalUtils::VCalDrag::canDecode(md)) {
        // TODO: Allow dragging events/todos onto other events to create a relation
        e->ignore();
        return;
    }
    if (KContacts::VCardDrag::canDecode(md) || md->hasText()) {
        e->accept();
    } else {
        e->ignore();
    }
}

void AgendaItem::addAttendee(const QString &newAttendee)
{
    if (!mValid) {
        return;
    }

    QString name;
    QString email;
    KEmailAddress::extractEmailAddressAndName(newAttendee, email, name);
    if (!(name.isEmpty() && email.isEmpty())) {
        mIncidence->addAttendee(KCalendarCore::Attendee(name, email));
        KMessageBox::information(this,
                                 i18n("Attendee \"%1\" added to the calendar item \"%2\"", KEmailAddress::normalizedAddress(name, email, QString()), text()),
                                 i18nc("@title:window", "Attendee added"),
                                 QStringLiteral("AttendeeDroppedAdded"));
    }
}

void AgendaItem::dropEvent(QDropEvent *e)
{
    // TODO: Organize this better: First check for attachment
    // (not only file, also any other url!), then if it's a vcard,
    // otherwise check for attendees, then if the data is binary,
    // add a binary attachment.
    if (!mValid) {
        return;
    }

    const QMimeData *md = e->mimeData();

    bool const decoded = md->hasText();
    QString const mdText = md->text();
    if (decoded && mdText.startsWith("file:"_L1)) {
        mIncidence->addAttachment(KCalendarCore::Attachment(mdText));
        return;
    }

    KContacts::Addressee::List list;

    if (KContacts::VCardDrag::fromMimeData(md, list)) {
        for (const KContacts::Addressee &addressee : std::as_const(list)) {
            QString em(addressee.fullEmail());
            if (em.isEmpty()) {
                em = addressee.realName();
            }
            addAttendee(em);
        }
    }
}

QList<AgendaItem::QPtr> &AgendaItem::conflictItems()
{
    return mConflictItems;
}

void AgendaItem::setConflictItems(const QList<AgendaItem::QPtr> &ci)
{
    mConflictItems = ci;
    for (QList<AgendaItem::QPtr>::iterator it = mConflictItems.begin(), end(mConflictItems.end()); it != end; ++it) {
        (*it)->addConflictItem(this);
    }
}

void AgendaItem::addConflictItem(const AgendaItem::QPtr &ci)
{
    if (!mConflictItems.contains(ci)) {
        mConflictItems.append(ci);
    }
}

QString AgendaItem::label() const
{
    return mLabelText;
}

bool AgendaItem::overlaps(CellItem *o) const
{
    AgendaItem::QPtr const other = static_cast<AgendaItem *>(o);

    if (cellXLeft() <= other->cellXRight() && cellXRight() >= other->cellXLeft()) {
        if ((cellYTop() <= other->cellYBottom()) && (cellYBottom() >= other->cellYTop())) {
            return true;
        }
    }

    return false;
}

static void conditionalPaint(QPainter *p, bool condition, int &x, int y, int ft, const QPixmap &pxmp)
{
    if (condition) {
        p->drawPixmap(x, y, pxmp);
        x += pxmp.width() + ft;
    }
}

void AgendaItem::paintIcon(QPainter *p, int &x, int y, int ft)
{
    QString iconName;
    if (mIncidence->customProperty("KABC", "ANNIVERSARY") == "YES"_L1) {
        mSpecialEvent = true;
        iconName = QStringLiteral("view-calendar-wedding-anniversary");
    } else if (mIncidence->customProperty("KABC", "BIRTHDAY") == "YES"_L1) {
        mSpecialEvent = true;
        // We don't draw icon. The icon is drawn already, because it's the Akonadi::Collection's icon
    }

    conditionalPaint(p, !iconName.isEmpty(), x, y, ft, cachedSmallIcon(iconName));
}

void AgendaItem::paintIcons(QPainter *p, int &x, int y, int ft)
{
    if (!mEventView->preferences()->enableAgendaItemIcons()) {
        return;
    }

    paintIcon(p, x, y, ft);

    QSet<EventView::ItemIcon> const icons = mEventView->preferences()->agendaViewIcons();

    if (icons.contains(EventViews::EventView::CalendarCustomIcon)) {
        const QString iconName = mCalendar->iconForIncidence(mIncidence);
        if (!iconName.isEmpty() && iconName != "view-calendar"_L1 && iconName != "office-calendar"_L1) {
            conditionalPaint(p, true, x, y, ft, QIcon::fromTheme(iconName).pixmap(16, 16));
        }
    }

    const bool isTodo = mIncidence && mIncidence->type() == Incidence::TypeTodo;

    if (isTodo && icons.contains(EventViews::EventView::TaskIcon)) {
        const QString iconName = mIncidence->iconName(mOccurrenceDateTime.toLocalTime());
        conditionalPaint(p, !mSpecialEvent, x, y, ft, QIcon::fromTheme(iconName).pixmap(16, 16));
    }

    if (icons.contains(EventView::RecurringIcon)) {
        conditionalPaint(p, mIconRecur && !mSpecialEvent, x, y, ft, *recurPxmp);
    }

    if (icons.contains(EventView::ReminderIcon)) {
        conditionalPaint(p, mIconAlarm && !mSpecialEvent, x, y, ft, *alarmPxmp);
    }

    if (icons.contains(EventView::ReadOnlyIcon)) {
        conditionalPaint(p, mIconReadonly && !mSpecialEvent, x, y, ft, *readonlyPxmp);
    }

    if (icons.contains(EventView::ReplyIcon)) {
        conditionalPaint(p, mIconReply, x, y, ft, *replyPxmp);
    }

    if (icons.contains(EventView::AttendingIcon)) {
        conditionalPaint(p, mIconGroup, x, y, ft, *groupPxmp);
    }

    if (icons.contains(EventView::TentativeIcon)) {
        conditionalPaint(p, mIconGroupTent, x, y, ft, *groupPxmpTent);
    }

    if (icons.contains(EventView::OrganizerIcon)) {
        conditionalPaint(p, mIconOrganizer, x, y, ft, *organizerPxmp);
    }
}

void AgendaItem::paintEvent(QPaintEvent *ev)
{
    if (!mValid) {
        return;
    }

    QRect const visRect = visibleRegion().boundingRect();
    // when scrolling horizontally in the side-by-side view, the repainted area is clipped
    // to the newly visible area, which is a problem since the content changes when visRect
    // changes, so repaint the full item in that case
    if (ev->rect() != visRect && visRect.isValid() && ev->rect().isValid()) {
        update(visRect);
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const int fmargin = 0; // frame margin
    const int ft = 1; // frame thickness for layout, see drawRoundedRect(),
    // keep multiple of 2
    const int margin = 5 + ft + fmargin; // frame + space between frame and content

    // General idea is to always show the icons (even in the all-day events).
    // This creates a consistent feeling for the user when the view mode
    // changes and therefore the available width changes.
    // Also look at #17984

    if (!alarmPxmp) {
        alarmPxmp = new QPixmap(QIcon::fromTheme(QStringLiteral("task-reminder")).pixmap(16, 16));
        recurPxmp = new QPixmap(QIcon::fromTheme(QStringLiteral("appointment-recurring")).pixmap(16, 16));
        readonlyPxmp = new QPixmap(QIcon::fromTheme(QStringLiteral("object-locked")).pixmap(16, 16));
        replyPxmp = new QPixmap(QIcon::fromTheme(QStringLiteral("mail-reply-sender")).pixmap(16, 16));
        groupPxmp = new QPixmap(QIcon::fromTheme(QStringLiteral("meeting-attending")).pixmap(16, 16));
        groupPxmpTent = new QPixmap(QIcon::fromTheme(QStringLiteral("meeting-attending-tentative")).pixmap(16, 16));
        organizerPxmp = new QPixmap(QIcon::fromTheme(QStringLiteral("meeting-organizer")).pixmap(16, 16));
    }

    const auto categoryColor = getCategoryColor();
    const auto rcColor = mResourceColor.isValid() ? mResourceColor : categoryColor;
    const auto frameColor = getFrameColor(rcColor, categoryColor);
    const auto bgBaseColor = getBackgroundColor(rcColor, categoryColor);
    const auto bgColor = mSelected ? bgBaseColor.lighter(EventView::BRIGHTNESS_FACTOR) : bgBaseColor;
    const auto textColor = EventViews::getTextColor(bgColor);

    p.setPen(textColor);

    p.setFont(mEventView->preferences()->agendaViewFont());
    QFontMetrics fm = p.fontMetrics();

    const int singleLineHeight = fm.boundingRect(mLabelText).height();

    const bool roundTop = !prevMultiItem();
    const bool roundBottom = !nextMultiItem();

    drawRoundedRect(&p,
                    QRect(fmargin, fmargin, width() - fmargin * 2, height() - fmargin * 2),
                    mSelected,
                    bgColor,
                    frameColor,
                    true,
                    ft,
                    roundTop,
                    roundBottom);

    // calculate the height of the full version (case 4) to test whether it is
    // possible

    QString shortH;
    QString longH;
    if (!isMultiItem()) {
        shortH = QLocale().toString(mIncidence->dateTime(KCalendarCore::Incidence::RoleDisplayStart).toLocalTime().time(), QLocale::ShortFormat);

        if (CalendarSupport::hasEvent(mIncidence)) {
            longH =
                i18n("%1 - %2", shortH, QLocale().toString(mIncidence->dateTime(KCalendarCore::Incidence::RoleEnd).toLocalTime().time(), QLocale::ShortFormat));
        } else {
            longH = shortH;
        }
    } else if (!mMultiItemInfo->mFirstMultiItem) {
        shortH = QLocale().toString(mIncidence->dtStart().toLocalTime().time(), QLocale::ShortFormat);
        longH = shortH;
    } else {
        shortH = QLocale().toString(mIncidence->dateTime(KCalendarCore::Incidence::RoleEnd).toLocalTime().time(), QLocale::ShortFormat);
        longH = i18n("- %1", shortH);
    }

    KWordWrap ww = KWordWrap::formatText(fm, QRect(0, 0, width() - (2 * margin), -1), 0, mLabelText);
    int const th = ww.boundingRect().height();

    int const hlHeight =
        qMax(fm.boundingRect(longH).height(),
             qMax(alarmPxmp->height(),
                  qMax(recurPxmp->height(), qMax(readonlyPxmp->height(), qMax(replyPxmp->height(), qMax(groupPxmp->height(), organizerPxmp->height()))))));

    const bool completelyRenderable = th < (height() - 2 * ft - 2 - hlHeight);

    // case 1: do not draw text when not even a single line fits
    // Don't do this any more, always try to print out the text.
    // Even if it's just a few pixel, one can still guess the whole
    // text from just four pixels' height!
    if ( //( singleLineHeight > height() - 4 ) ||
        (width() < 16)) {
        int x = qRound((width() - 16) / 2.0);
        paintIcon(&p, x /*by-ref*/, margin, ft);
        return;
    }

    // case 2: draw a single line when no more space
    if ((2 * singleLineHeight) > (height() - 2 * margin)) {
        int x = margin;
        int txtWidth;

        if (mIncidence->allDay()) {
            x += visRect.left();
            const int y = qRound((height() - 16) / 2.0);
            paintIcons(&p, x, y, ft);
            txtWidth = visRect.right() - margin - x;
        } else {
            const int y = qRound((height() - 16) / 2.0);
            paintIcons(&p, x, y, ft);
            txtWidth = width() - margin - x;
        }

        const int y = ((height() - singleLineHeight) / 2) + fm.ascent();
        // show "start: summary"
        const auto startTime = QLocale().toString(mIncidence->dateTime(KCalendarCore::Incidence::RoleDisplayStart).toLocalTime().time(), QLocale::ShortFormat);
        KWordWrap::drawFadeoutText(&p, x, y, txtWidth, i18n("%1: %2", startTime, mLabelText));
        return;
    }

    // case 3: enough for 2-5 lines, but not for the header.
    //         Also used for the middle days in multi-events
    if (((!completelyRenderable) && ((height() - (2 * margin)) <= (5 * singleLineHeight)))
        || (isMultiItem() && mMultiItemInfo->mNextMultiItem && mMultiItemInfo->mFirstMultiItem)) {
        int x = margin;
        int txtWidth;

        if (mIncidence->allDay()) {
            x += visRect.left();
            paintIcons(&p, x, margin, ft);
            txtWidth = visRect.right() - margin - x;
        } else {
            paintIcons(&p, x, margin, ft);
            txtWidth = width() - margin - x;
        }

        // show "start: summary"
        const auto startTime = QLocale().toString(mIncidence->dateTime(KCalendarCore::Incidence::RoleDisplayStart).toLocalTime().time(), QLocale::ShortFormat);
        ww = KWordWrap::formatText(fm, QRect(0, 0, txtWidth, (height() - (2 * margin))), 0, i18n("%1: %2", startTime, mLabelText));
        ww.drawText(&p, x, margin, Qt::AlignHCenter | KWordWrap::FadeOut);
        return;
    }

    // case 4: paint everything, with header:
    // consists of (vertically) ft + headline&icons + ft + text + margin
    int y = 2 * ft + hlHeight;
    if (completelyRenderable) {
        y += (height() - (2 * ft) - margin - hlHeight - th) / 2;
    }

    int x = margin;
    int txtWidth;
    int hTxtWidth;
    int eventX;

    if (mIncidence->allDay()) {
        shortH.clear();
        longH.clear();

        if (const KCalendarCore::Event::Ptr alldayEvent = CalendarSupport::event(mIncidence)) {
            if (alldayEvent->isMultiDay(QTimeZone::systemTimeZone())) {
                // multi-day, all-day event
                shortH = i18n("%1 - %2",
                              QLocale().toString(mIncidence->dtStart().toLocalTime().date()),
                              QLocale().toString(mIncidence->dateTime(KCalendarCore::Incidence::RoleEnd).toLocalTime().date()));
                longH = shortH;

                // paint headline
                drawRoundedRect(&p,
                                QRect(fmargin, fmargin, width() - fmargin * 2, -fmargin * 2 + margin + hlHeight),
                                mSelected,
                                frameColor,
                                frameColor,
                                false,
                                ft,
                                roundTop,
                                false);
            } else {
                // single-day, all-day event

                // paint headline
                drawRoundedRect(&p,
                                QRect(fmargin, fmargin, width() - fmargin * 2, -fmargin * 2 + margin + hlHeight),
                                mSelected,
                                frameColor,
                                frameColor,
                                false,
                                ft,
                                roundTop,
                                false);
            }
        } else {
            // to-do

            // paint headline
            drawRoundedRect(&p,
                            QRect(fmargin, fmargin, width() - fmargin * 2, -fmargin * 2 + margin + hlHeight),
                            mSelected,
                            frameColor,
                            frameColor,
                            false,
                            ft,
                            roundTop,
                            false);
        }

        x += visRect.left();
        eventX = x;
        txtWidth = visRect.right() - margin - x;
        paintIcons(&p, x, margin / 2, ft);
        hTxtWidth = visRect.right() - margin - x;
    } else {
        // paint headline
        drawRoundedRect(&p,
                        QRect(fmargin, fmargin, width() - fmargin * 2, -fmargin * 2 + margin + hlHeight),
                        mSelected,
                        frameColor,
                        frameColor,
                        false,
                        ft,
                        roundTop,
                        false);

        txtWidth = width() - margin - x;
        eventX = x;
        paintIcons(&p, x, margin / 2, ft);
        hTxtWidth = width() - margin - x;
    }

    QString headline;
    int hw = fm.boundingRect(longH).width();
    if (hw > hTxtWidth) {
        headline = shortH;
        hw = fm.boundingRect(shortH).width();
        if (hw < txtWidth) {
            x += (hTxtWidth - hw) / 2;
        }
    } else {
        headline = longH;
        x += (hTxtWidth - hw) / 2;
    }
    p.setBackground(QBrush(frameColor));
    p.setPen(EventViews::getTextColor(frameColor));
    KWordWrap::drawFadeoutText(&p, x, (margin + hlHeight + fm.ascent()) / 2 - 2, hTxtWidth, headline);

    // draw event text
    ww = KWordWrap::formatText(fm, QRect(0, 0, txtWidth, height() - margin - y), 0, mLabelText);

    p.setBackground(QBrush(bgColor));
    p.setPen(textColor);
    QString const ws = ww.wrappedString();
    if (QStringView(ws).left(ws.length() - 1).indexOf(u'\n') >= 0) {
        ww.drawText(&p, eventX, y, Qt::AlignLeft | KWordWrap::FadeOut);
    } else {
        ww.drawText(&p, eventX + (txtWidth - ww.boundingRect().width() - 2 * margin) / 2, y, Qt::AlignHCenter | KWordWrap::FadeOut);
    }
}

void AgendaItem::drawRoundedRect(QPainter *p,
                                 QRect rect,
                                 bool selected,
                                 const QColor &bgColor,
                                 const QColor &frameColor,
                                 bool frame,
                                 int ft,
                                 bool roundTop,
                                 bool roundBottom)
{
    Q_UNUSED(ft)
    if (!mValid) {
        return;
    }

    QPainterPath path;

    const int RECT_MARGIN = 2;
    const int RADIUS = 2; // absolute radius

    const QRect rectWithMargin(rect.x() + RECT_MARGIN, rect.y() + RECT_MARGIN, rect.width() - 2 * RECT_MARGIN, rect.height() - 2 * RECT_MARGIN);

    const QPoint pointLeftTop(rectWithMargin.x(), rectWithMargin.y());
    const QPoint pointRightTop(rectWithMargin.x() + rectWithMargin.width(), rectWithMargin.y());
    const QPoint pointLeftBottom(rectWithMargin.x(), rectWithMargin.y() + rectWithMargin.height());
    const QPoint pointRightBottom(rectWithMargin.x() + rectWithMargin.width(), rectWithMargin.y() + rectWithMargin.height());

    if (!roundTop && !roundBottom) {
        path.addRect(rectWithMargin);
    } else if (roundTop && roundBottom) {
        path.addRoundedRect(rectWithMargin, RADIUS, RADIUS, Qt::AbsoluteSize);
    } else if (roundTop) {
        path.moveTo(pointRightBottom);
        path.lineTo(pointLeftBottom);
        path.lineTo(QPoint(pointLeftTop.x(), pointLeftTop.y() + RADIUS));
        path.quadTo(pointLeftTop, QPoint(pointLeftTop.x() + RADIUS, pointLeftTop.y()));
        path.lineTo(QPoint(pointRightTop.x() - RADIUS, pointRightTop.y()));
        path.quadTo(pointRightTop, QPoint(pointRightTop.x(), pointRightTop.y() + RADIUS));
        path.lineTo(pointRightBottom);
    } else if (roundBottom) {
        path.moveTo(pointRightTop);
        path.lineTo(QPoint(pointRightBottom.x(), pointRightBottom.y() - RADIUS));
        path.quadTo(pointRightBottom, QPoint(pointRightBottom.x() - RADIUS, pointRightBottom.y()));
        path.lineTo(QPoint(pointLeftBottom.x() + RADIUS, pointLeftBottom.y()));
        path.quadTo(pointLeftBottom, QPoint(pointLeftBottom.x(), pointLeftBottom.y() - RADIUS));
        path.lineTo(pointLeftTop);
        path.lineTo(pointRightTop);
    }

    path.closeSubpath();
    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    const QPen border(frameColor, 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p->setPen(border);

    // header
    if (!frame) {
        QBrush brushSolid(Qt::SolidPattern);

        QColor top = bgColor.darker(250);
        top.setAlpha(selected ? 40 : 60);
        brushSolid.setColor(top);

        p->setBrush(bgColor);
        p->drawPath(path);

        p->setBrush(brushSolid);
        p->drawPath(path);
        p->restore();

        return;
    }

    p->setBrush(bgColor);
    p->drawPath(path);
    p->restore();
}

QColor AgendaItem::getCategoryColor() const
{
    const QStringList &categories = mIncidence->categories();
    if (categories.isEmpty() || !Akonadi::TagCache::instance()->tagColor(categories.first()).isValid()) {
        const auto colorPreference = mEventView->preferences()->agendaViewColors();
        if (colorPreference == PrefsBase::CategoryOnly || !mResourceColor.isValid()) {
            return CalendarSupport::KCalPrefs::instance()->unsetCategoryColor();
        }
        return mResourceColor;
    }
    return Akonadi::TagCache::instance()->tagColor(categories.first());
}

QColor AgendaItem::getFrameColor(const QColor &resourceColor, const QColor &categoryColor) const
{
    const auto colorPreference = mEventView->preferences()->agendaViewColors();
    const bool frameDisplaysCategory = (colorPreference == PrefsBase::CategoryOnly || colorPreference == PrefsBase::ResourceInsideCategoryOutside);
    return frameDisplaysCategory ? categoryColor : resourceColor;
}

QColor AgendaItem::getBackgroundColor(const QColor &resourceColor, const QColor &categoryColor) const
{
    if (CalendarSupport::hasTodo(mIncidence) && !mEventView->preferences()->todosUseCategoryColors()) {
        Todo::Ptr const todo = CalendarSupport::todo(mIncidence);
        Q_ASSERT(todo);
        const QDate dueDate = todo->dtDue().toLocalTime().date();
        const QDate today = QDate::currentDate();
        const QDate occurDate = this->occurrenceDate();
        if (todo->isOverdue() && today >= occurDate) {
            return mEventView->preferences()->todoOverdueColor();
        } else if (dueDate == today && dueDate == occurDate && !todo->isCompleted()) {
            return mEventView->preferences()->todoDueTodayColor();
        }
    }
    const auto colorPreference = mEventView->preferences()->agendaViewColors();
    const bool bgDisplaysCategory = (colorPreference == PrefsBase::CategoryOnly || colorPreference == PrefsBase::CategoryInsideResourceOutside);
    return bgDisplaysCategory ? categoryColor : resourceColor;
}

bool AgendaItem::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Paint) {
        return mValid;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

bool AgendaItem::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        if (!mEventView->preferences()->enableToolTips()) {
            return true;
        } else if (mValid) {
            auto helpEvent = static_cast<QHelpEvent *>(event);
            QToolTip::showText(helpEvent->globalPos(),
                               KCalUtils::IncidenceFormatter::toolTipStr(mCalendar->displayName(mIncidence), mIncidence, occurrenceDate(), true),
                               this);
        }
    }
    return QWidget::event(event);
}

#include "moc_agendaitem.cpp"
