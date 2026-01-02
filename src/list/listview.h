/*
  SPDX-FileCopyrightText: 1999 Preston Brown <pbrown@kde.org>
  SPDX-FileCopyrightText: 2000, 2001 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#pragma once

#include "eventview.h"

#include <memory>

class KConfig;

class QModelIndex;

namespace EventViews
{
class ListViewPrivate;

/*!
  This class provides a multi-column list view of events.  It can
  display events from one particular day or several days, it doesn't
  matter.

  \brief multi-column list view of various events.
  @author Preston Brown <pbrown@kde.org>
  \sa EventView
*/
class EVENTVIEWS_EXPORT ListView : public EventView
{
    Q_OBJECT
public:
    explicit ListView(QWidget *parent = nullptr, bool nonInteractive = false);
    ~ListView() override;

    [[nodiscard]] int currentDateCount() const override;
    [[nodiscard]] Akonadi::Item::List selectedIncidences() const override;
    [[nodiscard]] KCalendarCore::DateList selectedIncidenceDates() const override;

    // Shows all incidences of the calendar
    void showAll();

    /*!
     * Read settings from the "ListView Layout" group of the configuration.
     * \deprecated Use readSettings with a specific KConfigGroup.
     */
    void readSettings(KConfig *config);

    /*!
     * Read settings from the given configuration group.
     * \since 5.18.1
     */
    void readSettings(const KConfigGroup &cfgGroup);

    /*!
     * Write settings to the "ListView Layout" group of the configuration.
     * \deprecated Use writeSettings with a specific KConfigGroup.
     */
    void writeSettings(KConfig *config);

    /*!
     * Write settings to the given configuration group.
     * \since 5.18.1
     */
    void writeSettings(KConfigGroup &cfgGroup);

    void clear();
    QSize sizeHint() const override;

public Q_SLOTS:
    void updateView() override;

    void showDates(const QDate &start, const QDate &end, const QDate &preferredMonth = QDate()) override;

    void showIncidences(const Akonadi::Item::List &items, const QDate &date) override;

    void clearSelection() override;

    void changeIncidenceDisplay(const Akonadi::Item &, int);

    void defaultItemAction(const QModelIndex &);
    void defaultItemAction(const Akonadi::Item::Id id);

    void popupMenu(const QPoint &);

Q_SIGNALS:
    void showNewEventPopupSignal();
    void showIncidencePopupSignal(const Akonadi::CollectionCalendar::Ptr &, const Akonadi::Item &, const QDate &);

protected Q_SLOTS:
    void processSelectionChange();

private:
    EVENTVIEWS_NO_EXPORT void slotSortIndicatorChanged(int logicalIndex, Qt::SortOrder order);

    std::unique_ptr<ListViewPrivate> const d;
};
}
