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

#include <Akonadi/Collection>
#include <Akonadi/CollectionFilterProxyModel>
#include <Akonadi/ControlGui>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/IncidenceChanger>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>

#include <CalendarSupport/CollectionSelection>

#include <KCalendarCore/Event>

#include <KCheckableProxyModel>

using namespace Akonadi;
using namespace CalendarSupport;
using namespace EventViews;

MainWindow::MainWindow(const QStringList &viewNames)
    : QMainWindow()
    , mViewNames(viewNames)
    , mIncidenceChanger(nullptr)
    , mSettings(nullptr)
    , mViewPreferences(nullptr)
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
    EventView *eventView = nullptr;

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
        eventView->setIncidenceChanger(mIncidenceChanger);
        eventView->updateConfig();

        for (const auto &calendar : mCalendars) {
            eventView->addCalendar(calendar);
        }

        eventView->setDateRange(start, end);

        mUi.tabWidget->addTab(eventView, viewName);
        mEventViews.push_back(eventView);
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

    mMonitor = new Akonadi::Monitor(this);
    for (const auto &mt : KCalendarCore::Incidence::mimeTypes()) {
        mMonitor->setMimeTypeMonitored(mt);
    }
    mMonitor->itemFetchScope().fetchFullPayload();
    mMonitor->itemFetchScope().setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
    mEtm = new Akonadi::EntityTreeModel(mMonitor, this);

    auto collectionProxy = new Akonadi::CollectionFilterProxyModel(mEtm);
    collectionProxy->addMimeTypeFilters(KCalendarCore::Incidence::mimeTypes());
    collectionProxy->setSourceModel(mEtm);

    auto selectionModel = new QItemSelectionModel(collectionProxy, mEtm);

    auto checkableProxy = new KCheckableProxyModel(mEtm);
    checkableProxy->setSourceModel(collectionProxy);
    checkableProxy->setSelectionModel(selectionModel);

    mUi.calendarView->setModel(checkableProxy);

    CalendarSupport::CollectionSelection *collectionSelection = new CalendarSupport::CollectionSelection(selectionModel, this);
    EventViews::EventView::setGlobalCollectionSelection(collectionSelection);

    connect(collectionSelection, &CalendarSupport::CollectionSelection::collectionSelected, this, &MainWindow::collectionSelected);
    connect(collectionSelection, &CalendarSupport::CollectionSelection::collectionDeselected, this, &MainWindow::collectionDeselected);

    for (const auto &collection : collectionSelection->selectedCollections()) {
        collectionSelected(collection);
    }

    mIncidenceChanger = new IncidenceChanger(this);

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

void MainWindow::collectionSelected(const Akonadi::Collection &col)
{
    qDebug() << "Collection selected id=" << col.id() << "name=" << col.displayName();
    auto calendar = Akonadi::CollectionCalendar::Ptr::create(mEtm, col);
    mCalendars.push_back(calendar);

    for (auto view : mEventViews) {
        view->addCalendar(calendar);
        view->updateView();
    }
}

void MainWindow::collectionDeselected(const Akonadi::Collection &col)
{
    qDebug() << "Collection deselected id=" << col.id() << "name=" << col.displayName();
    auto calendar = std::find_if(mCalendars.begin(), mCalendars.end(), [col](const auto &cal) {
        return cal->collection() == col;
    });
    if (calendar == mCalendars.cend()) {
        return;
    }

    const auto start = QDateTime::currentDateTime().addDays(-1);
    const auto end = QDateTime::currentDateTime().addDays(1);

    for (auto view : mEventViews) {
        view->removeCalendar(*calendar);
        view->updateView();
    }

    mCalendars.erase(calendar);
}
#include "moc_mainwindow.cpp"
