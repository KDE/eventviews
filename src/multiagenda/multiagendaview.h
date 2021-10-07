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

/**
  Shows one agenda for every resource side-by-side.
*/
class EVENTVIEWS_EXPORT MultiAgendaView : public EventView
{
    Q_OBJECT
public:
    explicit MultiAgendaView(QWidget *parent = nullptr);
    ~MultiAgendaView() override;

    Q_REQUIRED_RESULT Akonadi::Item::List selectedIncidences() const override;
    Q_REQUIRED_RESULT KCalendarCore::DateList selectedIncidenceDates() const override;
    Q_REQUIRED_RESULT int currentDateCount() const override;
    Q_REQUIRED_RESULT int maxDatesHint() const;

    Q_REQUIRED_RESULT bool eventDurationHint(QDateTime &startDt, QDateTime &endDt, bool &allDay) const override;

    void setCalendar(const Akonadi::ETMCalendar::Ptr &cal) override;

    Q_REQUIRED_RESULT bool hasConfigurationDialog() const override;

    void setChanges(Changes changes) override;

    Q_REQUIRED_RESULT bool customColumnSetupUsed() const;
    Q_REQUIRED_RESULT int customNumberOfColumns() const;
    Q_REQUIRED_RESULT QStringList customColumnTitles() const;
    Q_REQUIRED_RESULT QVector<KCheckableProxyModel *> collectionSelectionModels() const;

    void setPreferences(const PrefsPtr &prefs) override;

Q_SIGNALS:
    void showNewEventPopupSignal();
    void showIncidencePopupSignal(const Akonadi::Item &, const QDate &);

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
    void slotSelectionChanged();
    void slotClearTimeSpanSelection();
    void resizeSplitters();
    void setupScrollBar();
    void zoomView(const int delta, QPoint pos, const Qt::Orientation ori);
    void slotResizeScrollView();
    void recreateViews();
    void forceRecreateViews();

private:
    class Private;
    std::unique_ptr<Private> const d;
};
}

