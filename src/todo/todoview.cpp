/*
  This file is part of KOrganizer.

  SPDX-FileCopyrightText: 2000, 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2005 Rafal Rzepecki <divide@users.sourceforge.net>
  SPDX-FileCopyrightText: 2008 Thomas Thrainer <tom_t@gmx.at>
  SPDX-FileCopyrightText: 2013 Sérgio Martins <iamsergio@gmail.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#include "todoview.h"
using namespace Qt::Literals::StringLiterals;

#include "calendarview_debug.h"
#include "coloredtodoproxymodel.h"
#include "tododelegates.h"
#include "todoviewquickaddline.h"
#include "todoviewquicksearch.h"
#include "todoviewsortfilterproxymodel.h"
#include "todoviewview.h"

#include <Akonadi/CalendarUtils>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeModel>
#include <Akonadi/TagFetchJob>

#include <Akonadi/ETMViewStateSaver>
#include <Akonadi/IncidenceTreeModel>
#include <Akonadi/TodoModel>

#include <CalendarSupport/KCalPrefs>

#include <KCalendarCore/CalFormat>

#include <KConfig>
#include <KDatePickerPopup>
#include <KDescendantsProxyModel>
#include <KJob>
#include <KMessageBox>

#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QToolButton>

#include <chrono>

using namespace std::chrono_literals;

Q_DECLARE_METATYPE(QPointer<QMenu>)

using namespace EventViews;
using namespace KCalendarCore;

namespace EventViews
{

class CalendarFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit CalendarFilterModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
        mDescendantsProxy.setDisplayAncestorData(false);
        QSortFilterProxyModel::setSourceModel(&mDescendantsProxy);
    }

    void setSourceModel(QAbstractItemModel *model) override
    {
        mDescendantsProxy.setSourceModel(model);
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        const auto source_index = sourceModel()->index(source_row, 0, source_parent);
        const auto item = sourceModel()->data(source_index, Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();

        if (!item.isValid()) {
            return false;
        }
        return mEnabledCalendars.contains(item.parentCollection().id());
    }

    void addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
    {
        mEnabledCalendars.insert(calendar->collection().id());
        invalidateFilter();
    }

    void removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
    {
        mEnabledCalendars.remove(calendar->collection().id());
        invalidateFilter();
    }

private:
    KDescendantsProxyModel mDescendantsProxy;
    QSet<Akonadi::Collection::Id> mEnabledCalendars;
};

// We share this struct between all views, for performance and memory purposes
class ModelStack
{
public:
    ModelStack(const EventViews::PrefsPtr &preferences, QObject *parent_)
        : todoModel(new Akonadi::TodoModel())
        , coloredTodoModel(new ColoredTodoProxyModel(preferences))
        , parent(parent_)
        , prefs(preferences)
    {
        coloredTodoModel->setSourceModel(todoModel);
    }

    ~ModelStack()
    {
        delete coloredTodoModel;
        delete todoModel;
        delete todoTreeModel;
        delete todoFlatModel;
    }

    void registerView(TodoView *view)
    {
        views << view;
    }

    void unregisterView(TodoView *view)
    {
        views.removeAll(view);
    }

    void setFlatView(bool flat)
    {
        const QString todoMimeType = QStringLiteral("application/x-vnd.akonadi.calendar.todo");
        if (flat) {
            for (TodoView *view : std::as_const(views)) {
                // In flatview dropping confuses users and it's very easy to drop into a child item
                view->mView->setDragDropMode(QAbstractItemView::DragOnly);
                view->setFlatView(flat, /**propagate=*/false); // So other views update their toggle icon

                if (todoTreeModel) {
                    view->saveViewState(); // Save the tree state before it's gone
                }
            }

            delete todoFlatModel;
            todoFlatModel = new Akonadi::EntityMimeTypeFilterModel(parent);
            todoFlatModel->addMimeTypeInclusionFilter(todoMimeType);
            todoFlatModel->setSourceModel(model);
            todoModel->setSourceModel(todoFlatModel);

            delete todoTreeModel;
            todoTreeModel = nullptr;
        } else {
            delete todoTreeModel;
            todoTreeModel = new Akonadi::IncidenceTreeModel(QStringList() << todoMimeType, parent);
            for (TodoView *view : std::as_const(views)) {
                QObject::connect(todoTreeModel, &Akonadi::IncidenceTreeModel::indexChangedParent, view, &TodoView::expandIndex);
                QObject::connect(todoTreeModel, &Akonadi::IncidenceTreeModel::batchInsertionFinished, view, &TodoView::restoreViewState);
                view->mView->setDragDropMode(QAbstractItemView::DragDrop);
                view->setFlatView(flat, /**propagate=*/false); // So other views update their toggle icon
            }
            todoTreeModel->setSourceModel(model);
            todoModel->setSourceModel(todoTreeModel);
            delete todoFlatModel;
            todoFlatModel = nullptr;
        }

        for (TodoView *view : std::as_const(views)) {
            view->mFlatViewButton->blockSignals(true);
            // We block signals to avoid recursion, we have two TodoViews and mFlatViewButton is synchronized
            view->mFlatViewButton->setChecked(flat);
            view->mFlatViewButton->blockSignals(false);
            view->mView->setRootIsDecorated(!flat);
            view->restoreViewState();
        }

        prefs->setFlatListTodo(flat);
        prefs->writeConfig();
    }

    void setModel(QAbstractItemModel *model)
    {
        this->model = model;
        if (todoTreeModel) {
            todoTreeModel->setSourceModel(this->model);
        }
    }

    bool isFlatView() const
    {
        return todoFlatModel != nullptr;
    }

    Akonadi::TodoModel *const todoModel;
    ColoredTodoProxyModel *const coloredTodoModel;
    QList<TodoView *> views;
    QObject *parent = nullptr;

    QAbstractItemModel *model = nullptr;
    Akonadi::IncidenceTreeModel *todoTreeModel = nullptr;
    Akonadi::EntityMimeTypeFilterModel *todoFlatModel = nullptr;
    EventViews::PrefsPtr prefs;
};
}

// Don't use K_GLOBAL_STATIC, see QTBUG-22667
static ModelStack *sModels = nullptr;

TodoView::TodoView(const EventViews::PrefsPtr &prefs, bool sidebarView, QWidget *parent)
    : EventView(parent)
    , mCalendarFilterModel(std::make_unique<CalendarFilterModel>())
    , mQuickSearch(nullptr)
    , mQuickAdd(nullptr)
    , mTreeStateRestorer(nullptr)
    , mSidebarView(sidebarView)
    , mResizeColumnsScheduled(false)
{
    mResizeColumnsTimer = new QTimer(this);
    connect(mResizeColumnsTimer, &QTimer::timeout, this, &TodoView::resizeColumns);
    mResizeColumnsTimer->setInterval(100ms); // so we don't overdue it when user resizes window manually
    mResizeColumnsTimer->setSingleShot(true);

    setPreferences(prefs);
    if (!sModels) {
        sModels = new ModelStack(prefs, parent);
        connect(sModels->todoModel, &Akonadi::TodoModel::dropOnSelfRejected, this, []() {
            KMessageBox::information(nullptr,
                                     i18nc("@info", "Cannot move to-do to itself or a child of itself."),
                                     i18nc("@title:window", "Drop To-do"),
                                     QStringLiteral("NoDropTodoOntoItself"));
        });
    }
    sModels->registerView(this);
    sModels->setModel(mCalendarFilterModel.get());

    mProxyModel = new TodoViewSortFilterProxyModel(preferences(), this);
    mProxyModel->setSourceModel(sModels->coloredTodoModel);
    mProxyModel->setFilterKeyColumn(Akonadi::TodoModel::SummaryColumn);
    mProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    mProxyModel->setSortRole(Qt::EditRole);
    connect(mProxyModel, &TodoViewSortFilterProxyModel::rowsInserted, this, &TodoView::onRowsInserted);

    if (!mSidebarView) {
        mQuickSearch = new TodoViewQuickSearch(this);
        mQuickSearch->setVisible(prefs->enableTodoQuickSearch());
        connect(mQuickSearch,
                &TodoViewQuickSearch::searchTextChanged,
                mProxyModel,
                qOverload<const QString &>(&QSortFilterProxyModel::setFilterRegularExpression));
        connect(mQuickSearch, &TodoViewQuickSearch::searchTextChanged, this, &TodoView::restoreViewState);
        connect(mQuickSearch, &TodoViewQuickSearch::filterCategoryChanged, mProxyModel, &TodoViewSortFilterProxyModel::setCategoryFilter);
        connect(mQuickSearch, &TodoViewQuickSearch::filterCategoryChanged, this, &TodoView::restoreViewState);
        connect(mQuickSearch, &TodoViewQuickSearch::filterPriorityChanged, mProxyModel, &TodoViewSortFilterProxyModel::setPriorityFilter);
        connect(mQuickSearch, &TodoViewQuickSearch::filterPriorityChanged, this, &TodoView::restoreViewState);
    }

    mView = new TodoViewView(this);
    mView->setModel(mProxyModel);

    mView->setContextMenuPolicy(Qt::CustomContextMenu);

    mView->setSortingEnabled(true);

    mView->setAutoExpandDelay(250);
    mView->setDragDropMode(QAbstractItemView::DragDrop);

    mView->setExpandsOnDoubleClick(false);
    mView->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

    connect(mView->header(), &QHeaderView::geometriesChanged, this, &TodoView::scheduleResizeColumns);
    connect(mView, &TodoViewView::visibleColumnCountChanged, this, &TodoView::resizeColumns);

    auto richTextDelegate = new TodoRichTextDelegate(mView);
    mView->setItemDelegateForColumn(Akonadi::TodoModel::SummaryColumn, richTextDelegate);
    mView->setItemDelegateForColumn(Akonadi::TodoModel::DescriptionColumn, richTextDelegate);

    auto priorityDelegate = new TodoPriorityDelegate(mView);
    mView->setItemDelegateForColumn(Akonadi::TodoModel::PriorityColumn, priorityDelegate);

    auto startDateDelegate = new TodoDueDateDelegate(mView);
    mView->setItemDelegateForColumn(Akonadi::TodoModel::StartDateColumn, startDateDelegate);

    auto dueDateDelegate = new TodoDueDateDelegate(mView);
    mView->setItemDelegateForColumn(Akonadi::TodoModel::DueDateColumn, dueDateDelegate);

    auto completeDelegate = new TodoCompleteDelegate(mView);
    mView->setItemDelegateForColumn(Akonadi::TodoModel::PercentColumn, completeDelegate);

    mCategoriesDelegate = new TodoCategoriesDelegate(mView);
    mView->setItemDelegateForColumn(Akonadi::TodoModel::CategoriesColumn, mCategoriesDelegate);

    connect(mView, &TodoViewView::customContextMenuRequested, this, &TodoView::contextMenu);
    connect(mView, &TodoViewView::doubleClicked, this, &TodoView::itemDoubleClicked);

    connect(mView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TodoView::currentChanged);

    mQuickAdd = new TodoViewQuickAddLine(this);
    mQuickAdd->setClearButtonEnabled(true);
    mQuickAdd->setVisible(preferences()->enableQuickTodo());
    connect(mQuickAdd, &TodoViewQuickAddLine::returnPressed, this, &TodoView::addQuickTodo);

    mFullViewButton = nullptr;
    if (!mSidebarView) {
        mFullViewButton = new QToolButton(this);
        mFullViewButton->setAutoRaise(true);
        mFullViewButton->setCheckable(true);

        mFullViewButton->setToolTip(i18nc("@info:tooltip", "Display to-do list in a full window"));
        mFullViewButton->setWhatsThis(i18nc("@info:whatsthis", "Checking this option will cause the to-do view to use the full window."));
    }
    mFlatViewButton = new QToolButton(this);
    mFlatViewButton->setAutoRaise(true);
    mFlatViewButton->setCheckable(true);
    mFlatViewButton->setToolTip(i18nc("@info:tooltip", "Display to-dos in a flat list or a tree"));
    mFlatViewButton->setWhatsThis(i18nc("@info:whatsthis",
                                        "Checking this button will cause the to-dos to be displayed either as a "
                                        "flat list or a hierarchical tree where the parental "
                                        "relationships are removed."));

    connect(mFlatViewButton, &QToolButton::toggled, this, [this](bool flatView) {
        setFlatView(flatView, true);
    });
    if (mFullViewButton) {
        connect(mFullViewButton, &QToolButton::toggled, this, &TodoView::setFullView);
    }

    auto layout = new QGridLayout(this);
    layout->setContentsMargins({});
    if (!mSidebarView) {
        layout->addWidget(mQuickSearch, 0, 0, 1, 2);
    }
    layout->addWidget(mView, 1, 0, 1, 2);
    layout->setRowStretch(1, 1);
    layout->addWidget(mQuickAdd, 2, 0);

    // Dummy layout just to add a few px of right margin so the checkbox is aligned
    // with the QAbstractItemView's viewport.
    auto dummyLayout = new QHBoxLayout();
    dummyLayout->setContentsMargins(0, 0, mView->frameWidth() /*right*/, 0);
    if (!mSidebarView) {
        auto f = new QFrame(this);
        f->setFrameShape(QFrame::VLine);
        f->setFrameShadow(QFrame::Sunken);
        dummyLayout->addWidget(f);
        dummyLayout->addWidget(mFullViewButton);
    }
    dummyLayout->addWidget(mFlatViewButton);

    layout->addLayout(dummyLayout, 2, 1);

    // ---------------- POPUP-MENUS -----------------------
    mItemPopupMenu = new QMenu(this);

    mItemPopupMenuItemOnlyEntries << mItemPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("document-preview")),
                                                               i18nc("@action:inmenu show the to-do", "&Show"),
                                                               this,
                                                               &TodoView::showTodo);

    QAction *a = mItemPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("document-edit")),
                                           i18nc("@action:inmenu edit the to-do", "&Edit…"),
                                           this,
                                           &TodoView::editTodo);
    mItemPopupMenuReadWriteEntries << a;
    mItemPopupMenuItemOnlyEntries << a;

    a = mItemPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("edit-delete")),
                                  i18nc("@action:inmenu delete the to-do", "&Delete"),
                                  this,
                                  &TodoView::deleteTodo);
    mItemPopupMenuReadWriteEntries << a;
    mItemPopupMenuItemOnlyEntries << a;

    mItemPopupMenu->addSeparator();

    mItemPopupMenuItemOnlyEntries << mItemPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("document-print")),
                                                               i18nc("@action:inmenu print the to-do", "&Print…"),
                                                               this,
                                                               &TodoView::printTodo);

    mItemPopupMenuItemOnlyEntries << mItemPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("document-print-preview")),
                                                               i18nc("@action:inmenu print preview the to-do", "Print Previe&w…"),
                                                               this,
                                                               &TodoView::printPreviewTodo);

    mItemPopupMenu->addSeparator();

    mItemPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("view-calendar-tasks")),
                              i18nc("@action:inmenu create a new to-do", "New &To-do…"),
                              this,
                              &TodoView::newTodo);

    a = mItemPopupMenu->addAction(i18nc("@action:inmenu create a new sub-to-do", "New Su&b-to-do…"), this, &TodoView::newSubTodo);
    mItemPopupMenuReadWriteEntries << a;
    mItemPopupMenuItemOnlyEntries << a;

    mMakeTodoIndependent = mItemPopupMenu->addAction(i18nc("@action:inmenu", "&Make this To-do Independent"), this, &TodoView::unSubTodoSignal);

    mMakeSubtodosIndependent = mItemPopupMenu->addAction(i18nc("@action:inmenu", "Make all Sub-to-dos &Independent"), this, &TodoView::unAllSubTodoSignal);

    mItemPopupMenuItemOnlyEntries << mMakeTodoIndependent;
    mItemPopupMenuItemOnlyEntries << mMakeSubtodosIndependent;

    mItemPopupMenuReadWriteEntries << mMakeTodoIndependent;
    mItemPopupMenuReadWriteEntries << mMakeSubtodosIndependent;

    mItemPopupMenu->addSeparator();

    a = mItemPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("appointment-new")),
                                  i18nc("@action:inmenu", "Create Event from To-do"),
                                  this,
                                  qOverload<>(&TodoView::createEvent));
    a->setObjectName("createevent"_L1);
    mItemPopupMenuReadWriteEntries << a;
    mItemPopupMenuItemOnlyEntries << a;

    mItemPopupMenu->addSeparator();

    mCopyPopupMenu = new KDatePickerPopup(KDatePickerPopup::NoDate | KDatePickerPopup::DatePicker | KDatePickerPopup::Words, QDate::currentDate(), this);
    mCopyPopupMenu->setTitle(i18nc("@title:menu", "&Copy To"));

    connect(mCopyPopupMenu, &KDatePickerPopup::dateChanged, this, &TodoView::copyTodoToDate);

    connect(mCopyPopupMenu, &KDatePickerPopup::dateChanged, mItemPopupMenu, &QMenu::hide);

    mMovePopupMenu = new KDatePickerPopup(KDatePickerPopup::NoDate | KDatePickerPopup::DatePicker | KDatePickerPopup::Words, QDate::currentDate(), this);
    mMovePopupMenu->setTitle(i18nc("@title:menu", "&Move To"));

    connect(mMovePopupMenu, &KDatePickerPopup::dateChanged, this, &TodoView::setNewDate);
    connect(mView->startPopupMenu(), &KDatePickerPopup::dateChanged, this, &TodoView::setStartDate);

    connect(mMovePopupMenu, &KDatePickerPopup::dateChanged, mItemPopupMenu, &QMenu::hide);

    mItemPopupMenu->insertMenu(nullptr, mCopyPopupMenu);
    mItemPopupMenu->insertMenu(nullptr, mMovePopupMenu);

    mItemPopupMenu->addSeparator();
    mItemPopupMenu->addAction(i18nc("@action:inmenu delete completed to-dos", "Pur&ge Completed"), this, &TodoView::purgeCompletedSignal);

    mPriorityPopupMenu = new QMenu(this);
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu unspecified priority", "unspecified"))] = 0;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu highest priority", "1 (highest)"))] = 1;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu priority value=2", "2"))] = 2;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu priority value=3", "3"))] = 3;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu priority value=4", "4"))] = 4;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu medium priority", "5 (medium)"))] = 5;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu priority value=6", "6"))] = 6;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu priority value=7", "7"))] = 7;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu priority value=8", "8"))] = 8;
    mPriority[mPriorityPopupMenu->addAction(i18nc("@action:inmenu lowest priority", "9 (lowest)"))] = 9;
    connect(mPriorityPopupMenu, &QMenu::triggered, this, &TodoView::setNewPriority);

    mPercentageCompletedPopupMenu = new QMenu(this);
    for (int i = 0; i <= 100; i += 10) {
        const QString label = QStringLiteral("%1 %").arg(i);
        mPercentage[mPercentageCompletedPopupMenu->addAction(label)] = i;
    }
    connect(mPercentageCompletedPopupMenu, &QMenu::triggered, this, &TodoView::setNewPercentage);

    setMinimumHeight(50);

    // Initialize our proxy models
    setFlatView(preferences()->flatListTodo());
    setFullView(preferences()->fullViewTodo());

    updateConfig();
}

TodoView::~TodoView()
{
    saveViewState();

    sModels->unregisterView(this);
    if (sModels->views.isEmpty()) {
        delete sModels;
        sModels = nullptr;
    }
}

void TodoView::expandIndex(const QModelIndex &index)
{
    QModelIndex todoModelIndex = sModels->todoModel->mapFromSource(index);
    Q_ASSERT(todoModelIndex.isValid());
    const auto coloredIndex = sModels->coloredTodoModel->mapFromSource(todoModelIndex);
    Q_ASSERT(coloredIndex.isValid());
    QModelIndex realIndex = mProxyModel->mapFromSource(coloredIndex);
    Q_ASSERT(realIndex.isValid());
    while (realIndex.isValid()) {
        mView->expand(realIndex);
        realIndex = mProxyModel->parent(realIndex);
    }
}

void TodoView::setModel(QAbstractItemModel *model)
{
    EventView::setModel(model);

    mCalendarFilterModel->setSourceModel(model);
    restoreViewState();
}

void TodoView::addCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    EventView::addCalendar(calendar);
    mCalendarFilterModel->addCalendar(calendar);
    if (calendars().size() == 1) {
        mProxyModel->setCalFilter(calendar->filter());
    }
}

void TodoView::removeCalendar(const Akonadi::CollectionCalendar::Ptr &calendar)
{
    mCalendarFilterModel->removeCalendar(calendar);
    EventView::removeCalendar(calendar);
}

Akonadi::Item::List TodoView::selectedIncidences() const
{
    Akonadi::Item::List ret;
    const QModelIndexList selection = mView->selectionModel()->selectedRows();
    ret.reserve(selection.count());
    for (const QModelIndex &mi : selection) {
        ret << mi.data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();
    }
    return ret;
}

DateList TodoView::selectedIncidenceDates() const
{
    // The todo view only lists todo's. It's probably not a good idea to
    // return something about the selected todo here, because it has got
    // a couple of dates (creation, due date, completion date), and the
    // caller could not figure out what he gets. So just return an empty list.
    return {};
}

void TodoView::saveLayout(KConfig *config, const QString &group) const
{
    KConfigGroup cfgGroup = config->group(group);
    QHeaderView *header = mView->header();

    QVariantList columnVisibility;
    QVariantList columnOrder;
    QVariantList columnWidths;
    const int headerCount = header->count();
    columnVisibility.reserve(headerCount);
    columnWidths.reserve(headerCount);
    columnOrder.reserve(headerCount);
    for (int i = 0; i < headerCount; ++i) {
        columnVisibility << QVariant(!mView->isColumnHidden(i));
        columnWidths << QVariant(header->sectionSize(i));
        columnOrder << QVariant(header->visualIndex(i));
    }
    cfgGroup.writeEntry("ColumnVisibility", columnVisibility);
    cfgGroup.writeEntry("ColumnOrder", columnOrder);
    cfgGroup.writeEntry("ColumnWidths", columnWidths);

    cfgGroup.writeEntry("SortAscending", (int)header->sortIndicatorOrder());
    if (header->isSortIndicatorShown()) {
        cfgGroup.writeEntry("SortColumn", header->sortIndicatorSection());
    } else {
        cfgGroup.writeEntry("SortColumn", -1);
    }

    if (!mSidebarView) {
        preferences()->setFullViewTodo(mFullViewButton->isChecked());
    }
    preferences()->setFlatListTodo(mFlatViewButton->isChecked());
}

void TodoView::restoreLayout(KConfig *config, const QString &group, bool minimalDefaults)
{
    KConfigGroup cfgGroup = config->group(group);
    QHeaderView *header = mView->header();

    QVariantList columnVisibility = cfgGroup.readEntry("ColumnVisibility", QVariantList());
    QVariantList columnOrder = cfgGroup.readEntry("ColumnOrder", QVariantList());
    QVariantList columnWidths = cfgGroup.readEntry("ColumnWidths", QVariantList());

    if (columnVisibility.isEmpty()) {
        // if config is empty then use default settings
        mView->hideColumn(Akonadi::TodoModel::RecurColumn);
        mView->hideColumn(Akonadi::TodoModel::DescriptionColumn);
        mView->hideColumn(Akonadi::TodoModel::CalendarColumn);
        mView->hideColumn(Akonadi::TodoModel::CompletedDateColumn);

        if (minimalDefaults) {
            mView->hideColumn(Akonadi::TodoModel::PriorityColumn);
            mView->hideColumn(Akonadi::TodoModel::PercentColumn);
            mView->hideColumn(Akonadi::TodoModel::DescriptionColumn);
            mView->hideColumn(Akonadi::TodoModel::CategoriesColumn);
        }

        // We don't have any incidences (content) yet, so we delay resizing
        QTimer::singleShot(0, this, &TodoView::resizeColumns);
    } else {
        for (int i = 0; i < header->count() && i < columnOrder.size() && i < columnWidths.size() && i < columnVisibility.size(); i++) {
            bool visible = columnVisibility[i].toBool();
            int width = columnWidths[i].toInt();
            int order = columnOrder[i].toInt();

            header->resizeSection(i, width);
            header->moveSection(header->visualIndex(i), order);
            if (i != 0 && !visible) {
                mView->hideColumn(i);
            }
        }
    }

    int sortOrder = cfgGroup.readEntry("SortAscending", (int)Qt::AscendingOrder);
    int sortColumn = cfgGroup.readEntry("SortColumn", -1);
    if (sortColumn >= 0) {
        mView->sortByColumn(sortColumn, (Qt::SortOrder)sortOrder);
    }

    mFlatViewButton->setChecked(cfgGroup.readEntry("FlatView", false));
}

void TodoView::setIncidenceChanger(Akonadi::IncidenceChanger *changer)
{
    EventView::setIncidenceChanger(changer);
    sModels->todoModel->setIncidenceChanger(changer);
}

void TodoView::showDates(const QDate &start, const QDate &end, const QDate &)
{
    // There is nothing to do here for the Todo View
    Q_UNUSED(start)
    Q_UNUSED(end)
}

void TodoView::showIncidences(const Akonadi::Item::List &incidenceList, const QDate &date)
{
    Q_UNUSED(incidenceList)
    Q_UNUSED(date)
}

void TodoView::updateView()
{
    if (calendars().empty()) {
        return;
    }

    auto calendar = calendars().first();
    mProxyModel->setCalFilter(calendar->filter());
}

void TodoView::changeIncidenceDisplay(const Akonadi::Item &, Akonadi::IncidenceChanger::ChangeType)
{
    // Don't do anything, model is connected to ETM, it's up to date
}

void TodoView::updateConfig()
{
    Q_ASSERT(preferences());
    if (!mSidebarView && mQuickSearch) {
        mQuickSearch->setVisible(preferences()->enableTodoQuickSearch());
    }

    if (mQuickAdd) {
        mQuickAdd->setVisible(preferences()->enableQuickTodo());
    }

    if (mProxyModel) {
        mProxyModel->invalidate();
    }

    updateView();
}

void TodoView::clearSelection()
{
    mView->selectionModel()->clearSelection();
}

void TodoView::addTodo(const QString &summary, const Akonadi::Item &parentItem, const QStringList &categories)
{
    const QString summaryTrimmed = summary.trimmed();
    if (!changer() || summaryTrimmed.isEmpty()) {
        return;
    }

    KCalendarCore::Todo::Ptr parent = Akonadi::CalendarUtils::todo(parentItem);

    KCalendarCore::Todo::Ptr todo(new KCalendarCore::Todo);
    todo->setSummary(summaryTrimmed);
    todo->setOrganizer(Person(CalendarSupport::KCalPrefs::instance()->fullName(), CalendarSupport::KCalPrefs::instance()->email()));

    todo->setCategories(categories);

    if (parent && !parent->hasRecurrenceId()) {
        todo->setRelatedTo(parent->uid());
    }

    Akonadi::Collection collection;

    // Use the same collection of the parent.
    if (parentItem.isValid()) {
        // Don't use parentColection() since it might be a virtual collection
        collection = Akonadi::EntityTreeModel::updatedCollection(model(), parentItem.storageCollectionId());
    }

    changer()->createIncidence(todo, collection, this);
}

void TodoView::addQuickTodo(Qt::KeyboardModifiers modifiers)
{
    if (modifiers == Qt::NoModifier) {
        /*const QModelIndex index = */
        addTodo(mQuickAdd->text(), Akonadi::Item(), mProxyModel->categories());
    } else if (modifiers == Qt::ControlModifier) {
        QModelIndexList selection = mView->selectionModel()->selectedRows();
        if (selection.count() != 1) {
            qCWarning(CALENDARVIEW_LOG) << "No to-do selected" << selection;
            return;
        }
        const QModelIndex idx = mProxyModel->mapToSource(selection[0]);
        mView->expand(selection[0]);
        const auto parent = sModels->coloredTodoModel->data(idx, Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
        addTodo(mQuickAdd->text(), parent, mProxyModel->categories());
    } else {
        return;
    }
    mQuickAdd->setText(QString());
}

void TodoView::contextMenu(QPoint pos)
{
    const bool hasItem = mView->indexAt(pos).isValid();
    Incidence::Ptr incidencePtr;

    for (QAction *entry : std::as_const(mItemPopupMenuItemOnlyEntries)) {
        bool enable;

        if (hasItem) {
            const Akonadi::Item::List incidences = selectedIncidences();

            if (incidences.isEmpty()) {
                enable = false;
            } else {
                Akonadi::Item item = incidences.first();
                incidencePtr = Akonadi::CalendarUtils::incidence(item);

                // Action isn't RO, it can change the incidence, "Edit" for example.
                const bool actionIsRw = mItemPopupMenuReadWriteEntries.contains(entry);

                const auto collection = Akonadi::EntityTreeModel::updatedCollection(model(), item.storageCollectionId());
                const bool incidenceIsRO = (collection.rights() & Akonadi::Collection::CanChangeItem) == 0;

                enable = hasItem && (!actionIsRw || !incidenceIsRO);
            }
        } else {
            enable = false;
        }

        entry->setEnabled(enable);
    }
    mCopyPopupMenu->setEnabled(hasItem);
    mMovePopupMenu->setEnabled(hasItem);

    if (hasItem) {
        if (incidencePtr) {
            const bool hasRecId = incidencePtr->hasRecurrenceId();
            const bool hasSubtodos = mView->model()->hasChildren(mView->indexAt(pos));

            mMakeSubtodosIndependent->setEnabled(!hasRecId && hasSubtodos);
            mMakeTodoIndependent->setEnabled(!hasRecId && !incidencePtr->relatedTo().isEmpty());
        }

        switch (mView->indexAt(pos).column()) {
        case Akonadi::TodoModel::PriorityColumn:
            mPriorityPopupMenu->popup(mView->viewport()->mapToGlobal(pos));
            break;
        case Akonadi::TodoModel::PercentColumn:
            mPercentageCompletedPopupMenu->popup(mView->viewport()->mapToGlobal(pos));
            break;
        case Akonadi::TodoModel::StartDateColumn:
            mView->startPopupMenu()->popup(mView->viewport()->mapToGlobal(pos));
            break;
        case Akonadi::TodoModel::DueDateColumn:
            mMovePopupMenu->popup(mView->viewport()->mapToGlobal(pos));
            break;
        case Akonadi::TodoModel::CategoriesColumn:
            createCategoryPopupMenu()->popup(mView->viewport()->mapToGlobal(pos));
            break;
        default:
            mItemPopupMenu->popup(mView->viewport()->mapToGlobal(pos));
            break;
        }
    } else {
        mItemPopupMenu->popup(mView->viewport()->mapToGlobal(pos));
    }
}

void TodoView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);
    if (!current.isValid()) {
        Q_EMIT incidenceSelected(Akonadi::Item(), QDate());
        return;
    }

    const auto todoItem = current.data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();

    if (selectedIncidenceDates().isEmpty()) {
        Q_EMIT incidenceSelected(todoItem, QDate());
    } else {
        Q_EMIT incidenceSelected(todoItem, selectedIncidenceDates().at(0));
    }
}

void TodoView::showTodo()
{
    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }

    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();

    Q_EMIT showIncidenceSignal(todoItem);
}

void TodoView::editTodo()
{
    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }

    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();
    Q_EMIT editIncidenceSignal(todoItem);
}

void TodoView::deleteTodo()
{
    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() == 1) {
        const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();

        if (!changer()->deletedRecently(todoItem.id())) {
            Q_EMIT deleteIncidenceSignal(todoItem);
        }
    }
}

void TodoView::newTodo()
{
    Q_EMIT newTodoSignal(QDate::currentDate().addDays(7));
}

void TodoView::newSubTodo()
{
    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() == 1) {
        const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();

        Q_EMIT newSubTodoSignal(todoItem);
    } else {
        // This never happens
        qCWarning(CALENDARVIEW_LOG) << "Selection size isn't 1";
    }
}

void TodoView::copyTodoToDate(QDate date)
{
    if (!changer()) {
        return;
    }

    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }

    const QModelIndex origIndex = mProxyModel->mapToSource(selection[0]);
    Q_ASSERT(origIndex.isValid());

    const auto origItem = sModels->coloredTodoModel->data(origIndex, Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();

    const KCalendarCore::Todo::Ptr orig = Akonadi::CalendarUtils::todo(origItem);
    if (!orig) {
        return;
    }

    KCalendarCore::Todo::Ptr todo(orig->clone());

    todo->setUid(KCalendarCore::CalFormat::createUniqueId());

    QDateTime due = todo->dtDue();
    due.setDate(date);
    todo->setDtDue(due);

    changer()->createIncidence(todo, Akonadi::Collection(), this);
}

void TodoView::scheduleResizeColumns()
{
    mResizeColumnsScheduled = true;
    mResizeColumnsTimer->start(); // restarts the timer if already active
}

void TodoView::itemDoubleClicked(const QModelIndex &index)
{
    if (index.isValid()) {
        QModelIndex summary = index.sibling(index.row(), Akonadi::TodoModel::SummaryColumn);
        if (summary.flags() & Qt::ItemIsEditable) {
            editTodo();
        } else {
            showTodo();
        }
    }
}

QMenu *TodoView::createCategoryPopupMenu()
{
    auto tempMenu = new QMenu(this);

    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return tempMenu;
    }

    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();
    KCalendarCore::Todo::Ptr todo = Akonadi::CalendarUtils::todo(todoItem);
    Q_ASSERT(todo);

    const QStringList checkedCategories = todo->categories();

    auto tagFetchJob = new Akonadi::TagFetchJob(this);
    connect(tagFetchJob, &Akonadi::TagFetchJob::result, this, &TodoView::onTagsFetched);
    tagFetchJob->setProperty("menu", QVariant::fromValue(QPointer<QMenu>(tempMenu)));
    tagFetchJob->setProperty("checkedCategories", checkedCategories);

    connect(tempMenu, &QMenu::triggered, this, &TodoView::changedCategories);
    connect(tempMenu, &QMenu::aboutToHide, tempMenu, &QMenu::deleteLater);
    return tempMenu;
}

void TodoView::onTagsFetched(KJob *job)
{
    if (job->error()) {
        qCWarning(CALENDARVIEW_LOG) << "Failed to fetch tags " << job->errorString();
        return;
    }
    auto fetchJob = static_cast<Akonadi::TagFetchJob *>(job);
    const QStringList checkedCategories = job->property("checkedCategories").toStringList();
    auto menu = job->property("menu").value<QPointer<QMenu>>();
    if (menu) {
        const auto lst = fetchJob->tags();
        for (const Akonadi::Tag &tag : lst) {
            const QString name = tag.name();
            QAction *action = menu->addAction(name);
            action->setCheckable(true);
            action->setData(name);
            if (checkedCategories.contains(name)) {
                action->setChecked(true);
            }
        }
    }
}

void TodoView::setNewDate(QDate date)
{
    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }

    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();
    KCalendarCore::Todo::Ptr todo = Akonadi::CalendarUtils::todo(todoItem);
    Q_ASSERT(todo);

    const auto collection = Akonadi::EntityTreeModel::updatedCollection(model(), todoItem.storageCollectionId());
    if (collection.rights() & Akonadi::Collection::CanChangeItem) {
        KCalendarCore::Todo::Ptr oldTodo(todo->clone());
        QDateTime dt(date.startOfDay());

        if (!todo->allDay()) {
            dt.setTime(todo->dtDue().time());
        }

        if (todo->hasStartDate() && dt < todo->dtStart()) {
            todo->setDtStart(dt);
        }
        todo->setDtDue(dt);

        changer()->modifyIncidence(todoItem, oldTodo, this);
    } else {
        qCDebug(CALENDARVIEW_LOG) << "Item is readOnly";
    }
}

void TodoView::setStartDate(QDate date)
{
    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }

    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();
    KCalendarCore::Todo::Ptr todo = Akonadi::CalendarUtils::todo(todoItem);
    Q_ASSERT(todo);

    const auto collection = Akonadi::EntityTreeModel::updatedCollection(model(), todoItem.storageCollectionId());
    if (collection.rights() & Akonadi::Collection::CanChangeItem) {
        KCalendarCore::Todo::Ptr oldTodo(todo->clone());
        QDateTime dt(date.startOfDay());

        if (!todo->allDay()) {
            dt.setTime(todo->dtStart().time());
        }

        if (todo->hasDueDate() && dt > todo->dtDue()) {
            todo->setDtDue(dt);
        }
        todo->setDtStart(dt);

        changer()->modifyIncidence(todoItem, oldTodo, this);
    } else {
        qCDebug(CALENDARVIEW_LOG) << "Item is readOnly";
    }
}

void TodoView::setNewPercentage(QAction *action)
{
    QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }

    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();
    KCalendarCore::Todo::Ptr todo = Akonadi::CalendarUtils::todo(todoItem);
    Q_ASSERT(todo);

    const auto collection = Akonadi::EntityTreeModel::updatedCollection(model(), todoItem.storageCollectionId());
    if (collection.rights() & Akonadi::Collection::CanChangeItem) {
        KCalendarCore::Todo::Ptr oldTodo(todo->clone());

        int percentage = mPercentage.value(action);
        if (percentage == 100) {
            todo->setCompleted(QDateTime::currentDateTime());
            todo->setPercentComplete(100);
        } else {
            todo->setPercentComplete(percentage);
        }
        changer()->modifyIncidence(todoItem, oldTodo, this);
    } else {
        qCDebug(CALENDARVIEW_LOG) << "Item is read only";
    }
}

void TodoView::setNewPriority(QAction *action)
{
    const QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }
    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();
    KCalendarCore::Todo::Ptr todo = Akonadi::CalendarUtils::todo(todoItem);
    const auto collection = Akonadi::EntityTreeModel::updatedCollection(model(), todoItem.storageCollectionId());
    if (collection.rights() & Akonadi::Collection::CanChangeItem) {
        KCalendarCore::Todo::Ptr oldTodo(todo->clone());
        todo->setPriority(mPriority[action]);

        changer()->modifyIncidence(todoItem, oldTodo, this);
    }
}

void TodoView::changedCategories(QAction *action)
{
    const QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }

    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();
    KCalendarCore::Todo::Ptr todo = Akonadi::CalendarUtils::todo(todoItem);
    Q_ASSERT(todo);
    const auto collection = Akonadi::EntityTreeModel::updatedCollection(model(), todoItem.storageCollectionId());
    if (collection.rights() & Akonadi::Collection::CanChangeItem) {
        KCalendarCore::Todo::Ptr oldTodo(todo->clone());

        const QString cat = action->data().toString();
        QStringList categories = todo->categories();
        if (categories.contains(cat)) {
            categories.removeAll(cat);
        } else {
            categories.append(cat);
        }
        categories.sort();
        todo->setCategories(categories);
        changer()->modifyIncidence(todoItem, oldTodo, this);
    } else {
        qCDebug(CALENDARVIEW_LOG) << "No active item, active item is read-only, or locking failed";
    }
}

void TodoView::setFullView(bool fullView)
{
    if (!mFullViewButton) {
        return;
    }

    mFullViewButton->setChecked(fullView);
    if (fullView) {
        mFullViewButton->setIcon(QIcon::fromTheme(QStringLiteral("view-restore")));
    } else {
        mFullViewButton->setIcon(QIcon::fromTheme(QStringLiteral("view-fullscreen")));
    }

    mFullViewButton->blockSignals(true);
    // We block signals to avoid recursion; there are two TodoViews and
    // also mFullViewButton is synchronized.
    mFullViewButton->setChecked(fullView);
    mFullViewButton->blockSignals(false);

    preferences()->setFullViewTodo(fullView);
    preferences()->writeConfig();

    Q_EMIT fullViewChanged(fullView);
}

void TodoView::setFlatView(bool flatView, bool notifyOtherViews)
{
    if (flatView) {
        mFlatViewButton->setIcon(QIcon::fromTheme(QStringLiteral("view-list-tree")));
    } else {
        mFlatViewButton->setIcon(QIcon::fromTheme(QStringLiteral("view-list-details")));
    }

    if (notifyOtherViews) {
        sModels->setFlatView(flatView);
    }
}

void TodoView::onRowsInserted(const QModelIndex &parent, int start, int end)
{
    if (start != end || !entityTreeModel()) {
        return;
    }

    QModelIndex idx = mView->model()->index(start, 0);

    // If the collection is currently being populated, we don't do anything
    QVariant v = idx.data(Akonadi::EntityTreeModel::ItemRole);
    if (!v.isValid()) {
        return;
    }

    auto item = v.value<Akonadi::Item>();
    if (!item.isValid()) {
        return;
    }

    const bool isPopulated = entityTreeModel()->isCollectionPopulated(item.storageCollectionId());
    if (!isPopulated) {
        return;
    }

    // Case #1, adding an item that doesn't have parent: We select it
    if (!parent.isValid()) {
        QModelIndexList selection = mView->selectionModel()->selectedRows();
        if (selection.size() <= 1) {
            // don't destroy complex selections, not applicable now (only single
            // selection allowed), but for the future...
            int colCount = static_cast<int>(Akonadi::TodoModel::ColumnCount);
            mView->selectionModel()->select(QItemSelection(idx, mView->model()->index(start, colCount - 1)),
                                            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        }
        return;
    }

    // Case 2: Adding an item that has a parent: we expand the parent
    if (sModels->isFlatView()) {
        return;
    }

    QModelIndex index = parent;
    mView->expand(index);
    while (index.parent().isValid()) {
        mView->expand(index.parent());
        index = index.parent();
    }
}

void TodoView::getHighlightMode(bool &highlightEvents, bool &highlightTodos, bool &highlightJournals)
{
    highlightTodos = preferences()->highlightTodos();
    highlightEvents = !highlightTodos;
    highlightJournals = false;
}

bool TodoView::usesFullWindow()
{
    return preferences()->fullViewTodo();
}

void TodoView::resizeColumns()
{
    mResizeColumnsScheduled = false;

    mView->resizeColumnToContents(Akonadi::TodoModel::StartDateColumn);
    mView->resizeColumnToContents(Akonadi::TodoModel::DueDateColumn);
    mView->resizeColumnToContents(Akonadi::TodoModel::CompletedDateColumn);
    mView->resizeColumnToContents(Akonadi::TodoModel::PriorityColumn);
    mView->resizeColumnToContents(Akonadi::TodoModel::CalendarColumn);
    mView->resizeColumnToContents(Akonadi::TodoModel::RecurColumn);
    mView->resizeColumnToContents(Akonadi::TodoModel::PercentColumn);

    // We have 3 columns that should stretch: summary, description and categories.
    // Summary is always visible.
    const bool descriptionVisible = !mView->isColumnHidden(Akonadi::TodoModel::DescriptionColumn);
    const bool categoriesVisible = !mView->isColumnHidden(Akonadi::TodoModel::CategoriesColumn);

    // Calculate size of non-stretchable columns:
    int size = 0;
    for (int i = 0; i < Akonadi::TodoModel::ColumnCount; ++i) {
        if (!mView->isColumnHidden(i) && i != Akonadi::TodoModel::SummaryColumn && i != Akonadi::TodoModel::DescriptionColumn
            && i != Akonadi::TodoModel::CategoriesColumn) {
            size += mView->columnWidth(i);
        }
    }

    // Calculate the remaining space that we have for the stretchable columns
    int remainingSize = mView->header()->width() - size;

    // 100 for summary, 100 for description
    const int requiredSize = descriptionVisible ? 200 : 100;

    if (categoriesVisible) {
        const int categorySize = 100;
        mView->setColumnWidth(Akonadi::TodoModel::CategoriesColumn, categorySize);
        remainingSize -= categorySize;
    }

    if (remainingSize < requiredSize) {
        // Too little size, so let's use a horizontal scrollbar
        mView->resizeColumnToContents(Akonadi::TodoModel::SummaryColumn);
        mView->resizeColumnToContents(Akonadi::TodoModel::DescriptionColumn);
    } else if (descriptionVisible) {
        mView->setColumnWidth(Akonadi::TodoModel::SummaryColumn, remainingSize / 2);
        mView->setColumnWidth(Akonadi::TodoModel::DescriptionColumn, remainingSize / 2);
    } else {
        mView->setColumnWidth(Akonadi::TodoModel::SummaryColumn, remainingSize);
    }
}

void TodoView::restoreViewState()
{
    if (sModels->isFlatView()) {
        return;
    }

    if (sModels->todoTreeModel && !sModels->todoTreeModel->sourceModel()) {
        return;
    }

    // QElapsedTimer timer;
    // timer.start();
    delete mTreeStateRestorer;
    mTreeStateRestorer = new Akonadi::ETMViewStateSaver();
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup group(config, stateSaverGroup());
    mTreeStateRestorer->setView(mView);
    mTreeStateRestorer->restoreState(group);
    // qCDebug(CALENDARVIEW_LOG) << "Took " << timer.elapsed();
}

QString TodoView::stateSaverGroup() const
{
    QString str = QStringLiteral("TodoTreeViewState");
    if (mSidebarView) {
        str += QLatin1Char('S');
    }

    return str;
}

void TodoView::saveViewState()
{
    Akonadi::ETMViewStateSaver treeStateSaver;
    KConfigGroup group(preferences()->config(), stateSaverGroup());
    treeStateSaver.setView(mView);
    treeStateSaver.saveState(group);
}

void TodoView::resizeEvent(QResizeEvent *event)
{
    EventViews::EventView::resizeEvent(event);
    scheduleResizeColumns();
}

void TodoView::createEvent()
{
    const QModelIndexList selection = mView->selectionModel()->selectedRows();
    if (selection.size() != 1) {
        return;
    }

    const auto todoItem = selection[0].data(Akonadi::TodoModel::TodoRole).value<Akonadi::Item>();

    Q_EMIT createEvent(todoItem);
}

#include "todoview.moc"

#include "moc_todoview.cpp"
