/*
  SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Sergio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "eventview.h"

#include <QDateTime>

#include <memory>

namespace EventViews
{
class ConfigDialogInterface;
class MultiAgendaViewPrivate;

;

/**
  Shows one agenda for every resource side-by-side.
*/
class EVENTVIEWS_EXPORT MultiAgendaView : public EventView
{
    Q_OBJECT
public:
    class CalendarFactory
    {
    public:
        using Ptr = QSharedPointer<CalendarFactory>;

        explicit CalendarFactory() = default;
        virtual ~CalendarFactory() = default;

        virtual Akonadi::CollectionCalendar::Ptr calendarForCollection(const Akonadi::Collection &collection) = 0;
    };

    explicit MultiAgendaView(QWidget *parent = nullptr);
    explicit MultiAgendaView(const CalendarFactory::Ptr &calendarFactory, QWidget *parent = nullptr);

    ~MultiAgendaView() override;

    void setModel(QAbstractItemModel *model) override;

    Q_REQUIRED_RESULT Akonadi::Item::List selectedIncidences() const override;
    Q_REQUIRED_RESULT KCalendarCore::DateList selectedIncidenceDates() const override;
    Q_REQUIRED_RESULT int currentDateCount() const override;
    Q_REQUIRED_RESULT int maxDatesHint() const;

    Q_REQUIRED_RESULT bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const override;

    void addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar) override;
    void removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar) override;

    Q_REQUIRED_RESULT bool hasConfigurationDialog() const override;

    void setChanges(Changes changes) override;

    Q_REQUIRED_RESULT bool customColumnSetupUsed() const;
    Q_REQUIRED_RESULT int customNumberOfColumns() const;
    Q_REQUIRED_RESULT QStringList customColumnTitles() const;
    Q_REQUIRED_RESULT QVector<KCheckableProxyModel *> collectionSelectionModels() const;

    void setPreferences(const PrefsPtr &prefs) override;

Q_SIGNALS:
    void showNewEventPopupSignal();
    void showIncidencePopupSignal(const Akonadi::CollectionCalendar::Ptr &calendar, const Akonadi::Item &, const QDate &);

public Q_SLOTS:

    void customCollectionsChanged(ConfigDialogInterface *dlg);

    void showDates(const QDate &start, const QDate &end, const QDate &preferredMonth = QDate()) override;
    void showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date) override;
    void updateView() override;
    void updateConfig() override;

    void setIncidenceChanger(Akonadi::IncidenceChanger *changer) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

    void doRestoreConfig(const KConfigGroup &configGroup) override;
    void doSaveConfig(KConfigGroup &configGroup) override;

protected Q_SLOTS:
    /**
     * Reimplemented from KOrg::BaseView
     */
    void collectionSelectionChanged();

private Q_SLOTS:
    EVENTVIEWS_NO_EXPORT void slotSelectionChanged();
    EVENTVIEWS_NO_EXPORT void slotClearTimeSpanSelection();
    EVENTVIEWS_NO_EXPORT void resizeSplitters();
    EVENTVIEWS_NO_EXPORT void setupScrollBar();
    EVENTVIEWS_NO_EXPORT void zoomView(const int delta, QPoint pos, const Qt::Orientation ori);
    EVENTVIEWS_NO_EXPORT void slotResizeScrollView();
    EVENTVIEWS_NO_EXPORT void recreateViews();
    EVENTVIEWS_NO_EXPORT void forceRecreateViews();

private:
    friend class MultiAgendaViewPrivate;
    std::unique_ptr<MultiAgendaViewPrivate> const d;
};
}
