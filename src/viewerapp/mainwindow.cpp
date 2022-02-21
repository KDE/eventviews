/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
    SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mainwindow.h"

#include "settings.h"

#include "agenda/agenda.h"
#include "agenda/agendaview.h"
#include "calendarview_debug.h"
#include "month/monthview.h"
#include "multiagenda/multiagendaview.h"
#include "prefs.h"
#include "timeline/timelineview.h"
#include <CalendarSupport/CollectionSelection>

#include <Akonadi/IncidenceChanger>
#include <KCalendarCore/Event>

#include <Akonadi/Collection>
#include <Akonadi/ControlGui>

#include <KCheckableProxyModel>

using namespace Akonadi;
using namespace CalendarSupport;
using namespace EventViews;

MainWindow::MainWindow(const QStringList &viewNames)
    : QMainWindow()
    , mViewNames(viewNames)
    , mIncidenceChanger(0)
    , mSettings(0)
    , mViewPreferences(0)
{
    mUi.setupUi(this);
    mUi.tabWidget->clear();

    connect(mUi.addViewMenu, &QMenu::triggered, this, &MainWindow::addViewTriggered);

    Akonadi::ControlGui::widgetNeedsAkonadi(this);

    setGeometry(0, 0, 800, 600);
    QMetaObject::invokeMethod(this, "delayedInit", Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    delete mViewPreferences;
    delete mSettings;
}

void MainWindow::addView(const QString &viewName)
{
    EventView *eventView = 0;

    const auto start = QDateTime::currentDateTime().addDays(-1);
    const auto end = QDateTime::currentDateTime().addDays(1);

    if (viewName == QLatin1String("agenda")) {
        eventView = new AgendaView(start.date(), end.date(), true, false, this);
    } else if (viewName == QLatin1String("multiagenda")) {
        eventView = new MultiAgendaView(this);
    } else if (viewName == QLatin1String("month")) {
        eventView = new MonthView(MonthView::Visible, this);
    } else if (viewName == QLatin1String("timeline")) {
        eventView = new TimelineView(this);
    }

    if (eventView) {
        eventView->setPreferences(*mViewPreferences);
        eventView->setCalendar(mCalendar);
        eventView->setIncidenceChanger(mIncidenceChanger);
        eventView->setDateRange(start, end);
        eventView->updateConfig();
        mUi.tabWidget->addTab(eventView, viewName);
    } else {
        qCCritical(CALENDARVIEW_LOG) << "Cannot create view" << viewName;
    }
}

void MainWindow::delayedInit()
{
    // create our application settings
    mSettings = new Settings;

    // create view preferences so that matching values are retrieved from
    // application settings
    mViewPreferences = new PrefsPtr(new Prefs(mSettings));

    mCalendar = Akonadi::ETMCalendar::Ptr(new Akonadi::ETMCalendar());
    KCheckableProxyModel *checkableProxy = mCalendar->checkableProxyModel();
    QItemSelectionModel *selectionModel = checkableProxy->selectionModel();

    CalendarSupport::CollectionSelection *collectionSelection = new CalendarSupport::CollectionSelection(selectionModel);
    EventViews::EventView::setGlobalCollectionSelection(collectionSelection);

    mIncidenceChanger = new IncidenceChanger(this);
    mCalendar->setCollectionFilteringEnabled(false);

    for (const QString &viewName : std::as_const(mViewNames)) {
        addView(viewName);
    }
}

void MainWindow::addViewTriggered(QAction *action)
{
    QString viewName = action->text().toLower();
    viewName.remove(QLatin1Char('&'));
    addView(viewName);
}
