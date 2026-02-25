/*
  SPDX-FileCopyrightText: 2001, 2003 Cornelius Schumacher <schumacher@kde.org>
  SPDX-FileCopyrightText: 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Kevin Krammer <krake@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0
*/
#include "prefs.h"
#include "prefs_base.h"

#include "calendarview_debug.h"

#include <Akonadi/AttributeFactory>
#include <Akonadi/CollectionColorAttribute>

#include <QFontDatabase>
#include <QRandomGenerator>
using namespace EventViews;

static QSet<EventViews::EventView::ItemIcon> iconArrayToSet(const QByteArray &array)
{
    QSet<EventViews::EventView::ItemIcon> set;
    for (int i = 0; i < array.size(); ++i) {
        if (i >= EventViews::EventView::IconCount) {
            qCWarning(CALENDARVIEW_LOG) << "Icon array is too big: " << array.size();
            return set;
        }
        if (array[i] != 0) {
            set.insert(static_cast<EventViews::EventView::ItemIcon>(i));
        }
    }
    return set;
}

static QByteArray iconSetToArray(const QSet<EventViews::EventView::ItemIcon> &set)
{
    QByteArray array;
    for (int i = 0; i < EventViews::EventView::IconCount; ++i) {
        const bool contains = set.contains(static_cast<EventViews::EventView::ItemIcon>(i));
        array.append(contains ? 1 : 0);
    }

    return array;
}

static QByteArray agendaViewIconDefaults()
{
    QByteArray iconDefaults;
    iconDefaults.resize(7);

    iconDefaults[EventViews::EventView::CalendarCustomIcon] = 1;
    iconDefaults[EventViews::EventView::TaskIcon] = 1;
    iconDefaults[EventViews::EventView::JournalIcon] = 1;
    iconDefaults[EventViews::EventView::RecurringIcon] = 1;
    iconDefaults[EventViews::EventView::ReminderIcon] = 1;
    iconDefaults[EventViews::EventView::ReadOnlyIcon] = 1;
    iconDefaults[EventViews::EventView::ReplyIcon] = 0;

    return iconDefaults;
}

static QByteArray monthViewIconDefaults()
{
    QByteArray iconDefaults;
    iconDefaults.resize(7);

    iconDefaults[EventViews::EventView::CalendarCustomIcon] = 1;
    iconDefaults[EventViews::EventView::TaskIcon] = 1;
    iconDefaults[EventViews::EventView::JournalIcon] = 1;
    iconDefaults[EventViews::EventView::RecurringIcon] = 0;
    iconDefaults[EventViews::EventView::ReminderIcon] = 0;
    iconDefaults[EventViews::EventView::ReadOnlyIcon] = 1;
    iconDefaults[EventViews::EventView::ReplyIcon] = 0;

    return iconDefaults;
}

class BaseConfig : public PrefsBase
{
public:
    BaseConfig();

    void setResourceColor(const QString &resource, const QColor &color);

    void setTimeScaleTimezones(const QStringList &timeZones);
    QStringList timeScaleTimezones() const;
    void setUse24HourClock(bool enable);
    bool use24HourClock() const;
    void setUseDualLabels(bool enable);
    bool useDualLabels() const;

    QHash<QString, QColor> mResourceColors;
    QColor mDefaultResourceColor;

    QFont mDefaultMonthViewFont;
    QFont mDefaultAgendaTimeLabelsFont;

    QStringList mTimeScaleTimeZones;
    bool mUse24HourClock = false;
    bool mUseDualLabels = false;

    QSet<EventViews::EventView::ItemIcon> mAgendaViewIcons;
    QSet<EventViews::EventView::ItemIcon> mMonthViewIcons;

protected:
    void usrSetDefaults() override;
    void usrRead() override;
    bool usrSave() override;
};

BaseConfig::BaseConfig()
    : mDefaultMonthViewFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont))
    , mDefaultAgendaTimeLabelsFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont))
{
    // make a large default time bar font, at least 16 points.
    mDefaultAgendaTimeLabelsFont.setPointSize(qMax(mDefaultAgendaTimeLabelsFont.pointSize() + 4, 16));

    // make it a bit smaller
    mDefaultMonthViewFont.setPointSize(qMax(mDefaultMonthViewFont.pointSize() - 2, 6));

    agendaTimeLabelsFontItem()->setDefaultValue(mDefaultAgendaTimeLabelsFont);
    agendaTimeLabelsFontItem()->setDefault();
    monthViewFontItem()->setDefaultValue(mDefaultMonthViewFont);
    monthViewFontItem()->setDefault();
}

void BaseConfig::setResourceColor(const QString &resource, const QColor &color)
{
    mResourceColors.insert(resource, color);
}

void BaseConfig::setTimeScaleTimezones(const QStringList &timeZones)
{
    mTimeScaleTimeZones = timeZones;
}

QStringList BaseConfig::timeScaleTimezones() const
{
    return mTimeScaleTimeZones;
}

void BaseConfig::setUse24HourClock(bool enable)
{
    mUse24HourClock = enable;
}

bool BaseConfig::use24HourClock() const
{
    return mUse24HourClock;
}

void BaseConfig::setUseDualLabels(bool enable)
{
    mUseDualLabels = enable;
}

bool BaseConfig::useDualLabels() const
{
    return mUseDualLabels;
}

void BaseConfig::usrSetDefaults()
{
    setAgendaTimeLabelsFont(mDefaultAgendaTimeLabelsFont);
    setMonthViewFont(mDefaultMonthViewFont);

    PrefsBase::usrSetDefaults();
}

void BaseConfig::usrRead()
{
    KConfigGroup const rColorsConfig(config(), QStringLiteral("Resources Colors"));
    const QStringList colorKeyList = rColorsConfig.keyList();

    for (const QString &key : colorKeyList) {
        QColor const color = rColorsConfig.readEntry(key, mDefaultResourceColor);
        // qCDebug(CALENDARVIEW_LOG) << "key:" << key << "value:" << color;
        setResourceColor(key, color);
    }

    KConfigGroup const timeScaleConfig(config(), QStringLiteral("Timescale"));
    setTimeScaleTimezones(timeScaleConfig.readEntry("Timescale Timezones", QStringList()));
    const QString str = QLocale().timeFormat();
    // 'A' or 'a' means am/pm is shown (and then 'h' uses 12-hour format)
    // but 'H' forces a 24-hour format anyway, even with am/pm shown.
    if (str.contains(QLatin1Char('a'), Qt::CaseInsensitive) && !str.contains(u'H')) {
        setUse24HourClock(timeScaleConfig.readEntry("24 Hour Clock", false));
    } else {
        setUse24HourClock(timeScaleConfig.readEntry("24 Hour Clock", true));
    }
    setUseDualLabels(timeScaleConfig.readEntry("Dual Labels", false));

    KConfigGroup const monthViewConfig(config(), QStringLiteral("Month View"));
    KConfigGroup const agendaViewConfig(config(), QStringLiteral("Agenda View"));
    const auto agendaIconArray = agendaViewConfig.readEntry<QByteArray>("agendaViewItemIcons", agendaViewIconDefaults());
    const auto monthIconArray = monthViewConfig.readEntry<QByteArray>("monthViewItemIcons", monthViewIconDefaults());

    mAgendaViewIcons = iconArrayToSet(agendaIconArray);
    mMonthViewIcons = iconArrayToSet(monthIconArray);

    KConfigSkeleton::usrRead();
}

bool BaseConfig::usrSave()
{
    KConfigGroup rColorsConfig(config(), QStringLiteral("Resources Colors"));
    for (auto it = mResourceColors.constBegin(); it != mResourceColors.constEnd(); ++it) {
        rColorsConfig.writeEntry(it.key(), it.value());
    }

    KConfigGroup timeScaleConfig(config(), QStringLiteral("Timescale"));
    timeScaleConfig.writeEntry("Timescale Timezones", timeScaleTimezones());
    timeScaleConfig.writeEntry("24 Hour Clock", use24HourClock());
    timeScaleConfig.writeEntry("Dual Labels", useDualLabels());

    KConfigGroup monthViewConfig(config(), QStringLiteral("Month View"));
    KConfigGroup agendaViewConfig(config(), QStringLiteral("Agenda View"));

    const QByteArray agendaIconArray = iconSetToArray(mAgendaViewIcons);
    const QByteArray monthIconArray = iconSetToArray(mMonthViewIcons);

    agendaViewConfig.writeEntry<QByteArray>("agendaViewItemIcons", agendaIconArray);
    monthViewConfig.writeEntry<QByteArray>("monthViewItemIcons", monthIconArray);

    return KConfigSkeleton::usrSave();
}

class EventViews::PrefsPrivate
{
public:
    explicit PrefsPrivate()
    {
    }

    explicit PrefsPrivate(KCoreConfigSkeleton *appConfig)
        : mAppConfig(appConfig)
    {
    }

    KConfigSkeletonItem *appConfigItem(const KConfigSkeletonItem *baseConfigItem) const;

    void setBool(KCoreConfigSkeleton::ItemBool *baseConfigItem, bool value);
    bool getBool(const KCoreConfigSkeleton::ItemBool *baseConfigItem) const;

    void setInt(KCoreConfigSkeleton::ItemInt *baseConfigItem, int value);
    int getInt(const KCoreConfigSkeleton::ItemInt *baseConfigItem) const;

    void setString(KCoreConfigSkeleton::ItemString *baseConfigItem, const QString &value);
    QString getString(const KCoreConfigSkeleton::ItemString *baseConfigItem) const;

    void setDateTime(KCoreConfigSkeleton::ItemDateTime *baseConfigItem, const QDateTime &value);
    QDateTime getDateTime(const KCoreConfigSkeleton::ItemDateTime *baseConfigItem) const;

    void setStringList(KCoreConfigSkeleton::ItemStringList *baseConfigItem, const QStringList &value);
    QStringList getStringList(const KCoreConfigSkeleton::ItemStringList *baseConfigItem) const;

    void setColor(KConfigSkeleton::ItemColor *baseConfigItem, const QColor &value);
    QColor getColor(const KConfigSkeleton::ItemColor *baseConfigItem) const;

    void setFont(KConfigSkeleton::ItemFont *baseConfigItem, const QFont &value);
    QFont getFont(const KConfigSkeleton::ItemFont *baseConfigItem) const;

    BaseConfig mBaseConfig;
    KCoreConfigSkeleton *mAppConfig = nullptr;
};

KConfigSkeletonItem *PrefsPrivate::appConfigItem(const KConfigSkeletonItem *baseConfigItem) const
{
    Q_ASSERT(baseConfigItem);

    if (mAppConfig) {
        return mAppConfig->findItem(baseConfigItem->name());
    }

    return nullptr;
}

void PrefsPrivate::setBool(KCoreConfigSkeleton::ItemBool *baseConfigItem, bool value)
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemBool *>(appItem);
        if (item) {
            item->setValue(value);
        } else {
            qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type Bool";
        }
    } else {
        baseConfigItem->setValue(value);
    }
}

bool PrefsPrivate::getBool(const KCoreConfigSkeleton::ItemBool *baseConfigItem) const
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemBool *>(appItem);
        if (item) {
            return item->value();
        }
        qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type Bool";
    }
    return baseConfigItem->value();
}

void PrefsPrivate::setInt(KCoreConfigSkeleton::ItemInt *baseConfigItem, int value)
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemInt *>(appItem);
        if (item) {
            item->setValue(value);
        } else {
            qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type Int";
        }
    } else {
        baseConfigItem->setValue(value);
    }
}

int PrefsPrivate::getInt(const KCoreConfigSkeleton::ItemInt *baseConfigItem) const
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemInt *>(appItem);
        if (item) {
            return item->value();
        }
        qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type Int";
    }
    return baseConfigItem->value();
}

void PrefsPrivate::setString(KCoreConfigSkeleton::ItemString *baseConfigItem, const QString &value)
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemString *>(appItem);

        if (item) {
            item->setValue(value);
        } else {
            qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type String";
        }
    } else {
        baseConfigItem->setValue(value);
    }
}

QString PrefsPrivate::getString(const KCoreConfigSkeleton::ItemString *baseConfigItem) const
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemString *>(appItem);

        if (item) {
            return item->value();
        }
        qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type String";
    }
    return baseConfigItem->value();
}

void PrefsPrivate::setDateTime(KCoreConfigSkeleton::ItemDateTime *baseConfigItem, const QDateTime &value)
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemDateTime *>(appItem);

        if (item) {
            item->setValue(value);
        } else {
            qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type DateTime";
        }
    } else {
        baseConfigItem->setValue(value);
    }
}

QDateTime PrefsPrivate::getDateTime(const KCoreConfigSkeleton::ItemDateTime *baseConfigItem) const
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemDateTime *>(appItem);

        if (item) {
            return item->value();
        }
        qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type DateTime";
    }
    return baseConfigItem->value();
}

void PrefsPrivate::setStringList(KCoreConfigSkeleton::ItemStringList *baseConfigItem, const QStringList &value)
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemStringList *>(appItem);

        if (item) {
            item->setValue(value);
        } else {
            qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type StringList";
        }
    } else {
        baseConfigItem->setValue(value);
    }
}

QStringList PrefsPrivate::getStringList(const KCoreConfigSkeleton::ItemStringList *baseConfigItem) const
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KCoreConfigSkeleton::ItemStringList *>(appItem);

        if (item) {
            return item->value();
        }
        qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type StringList";
    }
    return baseConfigItem->value();
}

void PrefsPrivate::setColor(KConfigSkeleton::ItemColor *baseConfigItem, const QColor &value)
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KConfigSkeleton::ItemColor *>(appItem);
        if (item) {
            item->setValue(value);
        } else {
            qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type Color";
        }
    } else {
        baseConfigItem->setValue(value);
    }
}

QColor PrefsPrivate::getColor(const KConfigSkeleton::ItemColor *baseConfigItem) const
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KConfigSkeleton::ItemColor *>(appItem);
        if (item) {
            return item->value();
        }
        qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type Color";
    }
    return baseConfigItem->value();
}

void PrefsPrivate::setFont(KConfigSkeleton::ItemFont *baseConfigItem, const QFont &value)
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KConfigSkeleton::ItemFont *>(appItem);
        if (item) {
            item->setValue(value);
        } else {
            qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type Font";
        }
    } else {
        baseConfigItem->setValue(value);
    }
}

QFont PrefsPrivate::getFont(const KConfigSkeleton::ItemFont *baseConfigItem) const
{
    KConfigSkeletonItem *appItem = appConfigItem(baseConfigItem);
    if (appItem) {
        auto item = dynamic_cast<KConfigSkeleton::ItemFont *>(appItem);
        if (item) {
            return item->value();
        }
        qCCritical(CALENDARVIEW_LOG) << "Application config item" << appItem->name() << "is not of type Font";
    }
    return baseConfigItem->value();
}

Prefs::Prefs()
    : d(new PrefsPrivate())
{
    // necessary to use CollectionColorAttribute in the EventViews::resourceColor and EventViews::setResourceColor
    Akonadi::AttributeFactory::registerAttribute<Akonadi::CollectionColorAttribute>();
}

Prefs::Prefs(KCoreConfigSkeleton *appConfig)
    : d(new PrefsPrivate(appConfig))
{
    // necessary to use CollectionColorAttribute in the EventViews::resourceColor and EventViews::setResourceColor
    Akonadi::AttributeFactory::registerAttribute<Akonadi::CollectionColorAttribute>();
}

Prefs::~Prefs() = default;

void Prefs::readConfig()
{
    d->mBaseConfig.load();
    if (d->mAppConfig) {
        d->mAppConfig->load();
    }
}

void Prefs::writeConfig()
{
    d->mBaseConfig.save();
    if (d->mAppConfig) {
        d->mAppConfig->save();
    }
}

void Prefs::setUseSystemColor(bool useSystemColor)
{
    d->setBool(d->mBaseConfig.useSystemColorItem(), useSystemColor);
}

bool Prefs::useSystemColor() const
{
    return d->getBool(d->mBaseConfig.useSystemColorItem());
}

void Prefs::setMarcusBainsShowSeconds(bool showSeconds)
{
    d->setBool(d->mBaseConfig.marcusBainsShowSecondsItem(), showSeconds);
}

bool Prefs::marcusBainsShowSeconds() const
{
    return d->getBool(d->mBaseConfig.marcusBainsShowSecondsItem());
}

void Prefs::setAgendaMarcusBainsLineLineColor(const QColor &color)
{
    d->setColor(d->mBaseConfig.agendaMarcusBainsLineLineColorItem(), color);
}

QColor Prefs::agendaMarcusBainsLineLineColor() const
{
    return d->getColor(d->mBaseConfig.agendaMarcusBainsLineLineColorItem());
}

void Prefs::setMarcusBainsEnabled(bool enabled)
{
    d->setBool(d->mBaseConfig.marcusBainsEnabledItem(), enabled);
}

bool Prefs::marcusBainsEnabled() const
{
    return d->getBool(d->mBaseConfig.marcusBainsEnabledItem());
}

void Prefs::setAgendaMarcusBainsLineFont(const QFont &font)
{
    d->setFont(d->mBaseConfig.agendaMarcusBainsLineFontItem(), font);
}

QFont Prefs::agendaMarcusBainsLineFont() const
{
    return d->getFont(d->mBaseConfig.agendaMarcusBainsLineFontItem());
}

void Prefs::setHourSize(int size)
{
    d->setInt(d->mBaseConfig.hourSizeItem(), size);
}

int Prefs::hourSize() const
{
    return d->getInt(d->mBaseConfig.hourSizeItem());
}

void Prefs::setDayBegins(const QDateTime &dateTime)
{
    d->setDateTime(d->mBaseConfig.dayBeginsItem(), dateTime);
}

QDateTime Prefs::dayBegins() const
{
    return d->getDateTime(d->mBaseConfig.dayBeginsItem());
}

void Prefs::setFirstDayOfWeek(const int day)
{
    d->setInt(d->mBaseConfig.weekStartDayItem(), day - 1);
}

int Prefs::firstDayOfWeek() const
{
    return d->getInt(d->mBaseConfig.weekStartDayItem()) + 1;
}

void Prefs::setWorkingHoursStart(const QDateTime &dateTime)
{
    d->setDateTime(d->mBaseConfig.workingHoursStartItem(), dateTime);
}

QDateTime Prefs::workingHoursStart() const
{
    return d->getDateTime(d->mBaseConfig.workingHoursStartItem());
}

void Prefs::setWorkingHoursEnd(const QDateTime &dateTime)
{
    d->setDateTime(d->mBaseConfig.workingHoursEndItem(), dateTime);
}

QDateTime Prefs::workingHoursEnd() const
{
    return d->getDateTime(d->mBaseConfig.workingHoursEndItem());
}

void Prefs::setSelectionStartsEditor(bool startEditor)
{
    d->setBool(d->mBaseConfig.selectionStartsEditorItem(), startEditor);
}

bool Prefs::selectionStartsEditor() const
{
    return d->getBool(d->mBaseConfig.selectionStartsEditorItem());
}

void Prefs::setAgendaGridWorkHoursBackgroundColor(const QColor &color)
{
    d->setColor(d->mBaseConfig.agendaGridWorkHoursBackgroundColorItem(), color);
}

QColor Prefs::agendaGridWorkHoursBackgroundColor() const
{
    return d->getColor(d->mBaseConfig.agendaGridWorkHoursBackgroundColorItem());
}

void Prefs::setAgendaGridHighlightColor(const QColor &color)
{
    d->setColor(d->mBaseConfig.agendaGridHighlightColorItem(), color);
}

QColor Prefs::agendaGridHighlightColor() const
{
    return d->getColor(d->mBaseConfig.agendaGridHighlightColorItem());
}

void Prefs::setAgendaGridBackgroundColor(const QColor &color)
{
    d->setColor(d->mBaseConfig.agendaGridBackgroundColorItem(), color);
}

QColor Prefs::agendaGridBackgroundColor() const
{
    return d->getColor(d->mBaseConfig.agendaGridBackgroundColorItem());
}

void Prefs::setEnableAgendaItemIcons(bool enable)
{
    d->setBool(d->mBaseConfig.enableAgendaItemIconsItem(), enable);
}

bool Prefs::enableAgendaItemIcons() const
{
    return d->getBool(d->mBaseConfig.enableAgendaItemIconsItem());
}

void Prefs::setEnableAgendaItemDesc(bool enable)
{
    d->setBool(d->mBaseConfig.enableAgendaItemDescItem(), enable);
}

bool Prefs::enableAgendaItemDesc() const
{
    return d->getBool(d->mBaseConfig.enableAgendaItemDescItem());
}

void Prefs::setTodosUseCategoryColors(bool useColors)
{
    d->setBool(d->mBaseConfig.todosUseCategoryColorsItem(), useColors);
}

bool Prefs::todosUseCategoryColors() const
{
    return d->getBool(d->mBaseConfig.todosUseCategoryColorsItem());
}

void Prefs::setAgendaHolidaysBackgroundColor(const QColor &color) const
{
    d->setColor(d->mBaseConfig.agendaHolidaysBackgroundColorItem(), color);
}

QColor Prefs::agendaHolidaysBackgroundColor() const
{
    return d->getColor(d->mBaseConfig.agendaHolidaysBackgroundColorItem());
}

void Prefs::setAgendaViewColors(int colors)
{
    d->setInt(d->mBaseConfig.agendaViewColorsItem(), colors);
}

int Prefs::agendaViewColors() const
{
    return d->getInt(d->mBaseConfig.agendaViewColorsItem());
}

void Prefs::setAgendaViewFont(const QFont &font)
{
    d->setFont(d->mBaseConfig.agendaViewFontItem(), font);
}

QFont Prefs::agendaViewFont() const
{
    return d->getFont(d->mBaseConfig.agendaViewFontItem());
}

void Prefs::setMonthViewFont(const QFont &font)
{
    d->setFont(d->mBaseConfig.monthViewFontItem(), font);
}

QFont Prefs::monthViewFont() const
{
    return d->getFont(d->mBaseConfig.monthViewFontItem());
}

QColor Prefs::monthGridBackgroundColor() const
{
    return d->getColor(d->mBaseConfig.monthGridBackgroundColorItem());
}

void Prefs::setMonthGridBackgroundColor(const QColor &color)
{
    d->setColor(d->mBaseConfig.monthGridBackgroundColorItem(), color);
}

QColor Prefs::monthGridWorkHoursBackgroundColor() const
{
    return d->getColor(d->mBaseConfig.monthGridWorkHoursBackgroundColorItem());
}

void Prefs::monthGridWorkHoursBackgroundColor(const QColor &color)
{
    d->setColor(d->mBaseConfig.monthGridWorkHoursBackgroundColorItem(), color);
}

int Prefs::monthViewColors() const
{
    return d->getInt(d->mBaseConfig.monthViewColorsItem());
}

void Prefs::setMonthViewColors(int colors) const
{
    d->setInt(d->mBaseConfig.monthViewColorsItem(), colors);
}

void Prefs::setEnableMonthItemIcons(bool enable)
{
    d->setBool(d->mBaseConfig.enableMonthItemIconsItem(), enable);
}

bool Prefs::enableMonthItemIcons() const
{
    return d->getBool(d->mBaseConfig.enableMonthItemIconsItem());
}

bool Prefs::showTimeInMonthView() const
{
    return d->getBool(d->mBaseConfig.showTimeInMonthViewItem());
}

void Prefs::setShowTimeInMonthView(bool show)
{
    d->setBool(d->mBaseConfig.showTimeInMonthViewItem(), show);
}

bool Prefs::showTodosMonthView() const
{
    return d->getBool(d->mBaseConfig.showTodosMonthViewItem());
}

void Prefs::setShowTodosMonthView(bool show)
{
    d->setBool(d->mBaseConfig.showTodosMonthViewItem(), show);
}

bool Prefs::showJournalsMonthView() const
{
    return d->getBool(d->mBaseConfig.showJournalsMonthViewItem());
}

void Prefs::setShowJournalsMonthView(bool show)
{
    d->setBool(d->mBaseConfig.showJournalsMonthViewItem(), show);
}

bool Prefs::fullViewMonth() const
{
    return d->getBool(d->mBaseConfig.fullViewMonthItem());
}

void Prefs::setFullViewMonth(bool fullView)
{
    d->setBool(d->mBaseConfig.fullViewMonthItem(), fullView);
}

bool Prefs::sortCompletedTodosSeparately() const
{
    return d->getBool(d->mBaseConfig.sortCompletedTodosSeparatelyItem());
}

void Prefs::setSortCompletedTodosSeparately(bool enable)
{
    d->setBool(d->mBaseConfig.sortCompletedTodosSeparatelyItem(), enable);
}

void Prefs::setEnableToolTips(bool enable)
{
    d->setBool(d->mBaseConfig.enableToolTipsItem(), enable);
}

bool Prefs::enableToolTips() const
{
    return d->getBool(d->mBaseConfig.enableToolTipsItem());
}

void Prefs::setShowTodosAgendaView(bool show)
{
    d->setBool(d->mBaseConfig.showTodosAgendaViewItem(), show);
}

bool Prefs::showTodosAgendaView() const
{
    return d->getBool(d->mBaseConfig.showTodosAgendaViewItem());
}

void Prefs::setAgendaTimeLabelsFont(const QFont &font)
{
    d->setFont(d->mBaseConfig.agendaTimeLabelsFontItem(), font);
}

QFont Prefs::agendaTimeLabelsFont() const
{
    return d->getFont(d->mBaseConfig.agendaTimeLabelsFontItem());
}

/* cppcheck-suppress functionStatic */
QTimeZone Prefs::timeZone() const
{
    return QTimeZone::systemTimeZone();
}

bool Prefs::colorAgendaBusyDays() const
{
    return d->getBool(d->mBaseConfig.colorBusyDaysEnabledItem());
}

bool Prefs::colorMonthBusyDays() const
{
    return d->getBool(d->mBaseConfig.colorMonthBusyDaysEnabledItem());
}

QColor Prefs::viewBgBusyColor() const
{
    return d->getColor(d->mBaseConfig.viewBgBusyColorItem());
}

void Prefs::setViewBgBusyColor(const QColor &color)
{
    d->mBaseConfig.mViewBgBusyColor = color;
}

QColor Prefs::holidayColor() const
{
    return d->getColor(d->mBaseConfig.holidayColorItem());
}

void Prefs::setHolidayColor(const QColor &color)
{
    d->mBaseConfig.mHolidayColor = color;
}

QColor Prefs::agendaViewBackgroundColor() const
{
    return d->getColor(d->mBaseConfig.agendaBgColorItem());
}

void Prefs::setAgendaViewBackgroundColor(const QColor &color)
{
    d->mBaseConfig.mAgendaBgColor = color;
}

QColor Prefs::workingHoursColor() const
{
    return d->getColor(d->mBaseConfig.workingHoursColorItem());
}

void Prefs::setWorkingHoursColor(const QColor &color)
{
    d->mBaseConfig.mWorkingHoursColor = color;
}

QColor Prefs::todoDueTodayColor() const
{
    return d->getColor(d->mBaseConfig.todoDueTodayColorItem());
}

void Prefs::setTodoDueTodayColor(const QColor &color)
{
    d->mBaseConfig.mTodoDueTodayColor = color;
}

QColor Prefs::todoOverdueColor() const
{
    return d->getColor(d->mBaseConfig.todoOverdueColorItem());
}

void Prefs::setTodoOverdueColor(const QColor &color)
{
    d->mBaseConfig.mTodoOverdueColor = color;
}

void Prefs::setColorAgendaBusyDays(bool enable)
{
    d->mBaseConfig.mColorBusyDaysEnabled = enable;
}

void Prefs::setColorMonthBusyDays(bool enable)
{
    d->mBaseConfig.mColorMonthBusyDaysEnabled = enable;
}

QColor Prefs::monthTodayColor() const
{
    return d->getColor(d->mBaseConfig.monthTodayColorItem());
}

void Prefs::setMonthTodayColor(const QColor &color)
{
    d->mBaseConfig.mMonthTodayColor = color;
}

void Prefs::setResourceColor(const QString &cal, const QColor &color)
{
    d->mBaseConfig.setResourceColor(cal, color);
}

QColor Prefs::resourceColorKnown(const QString &cal) const
{
    QColor color;
    if (!cal.isEmpty()) {
        color = d->mBaseConfig.mResourceColors.value(cal);
    }
    return color;
}

QColor Prefs::resourceColor(const QString &cal)
{
    if (cal.isEmpty()) {
        return d->mBaseConfig.mDefaultResourceColor;
    }

    QColor color = resourceColorKnown(cal);

    // assign default color if enabled
    if (!color.isValid() && d->getBool(d->mBaseConfig.assignDefaultResourceColorsItem())) {
        color.setRgb(0x37, 0x7A, 0xBC); // blueish
        const int seed = d->getInt(d->mBaseConfig.defaultResourceColorSeedItem());
        const QStringList colors = d->getStringList(d->mBaseConfig.defaultResourceColorsItem());
        if (seed > 0 && seed - 1 < colors.size()) {
            color = QColor::fromString(colors[seed - 1]);
        } else {
            color.setRgb(QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256), QRandomGenerator::global()->bounded(256));
        }
        d->setInt(d->mBaseConfig.defaultResourceColorSeedItem(), (seed + 1));
        d->mBaseConfig.setResourceColor(cal, color);
    }
    if (color.isValid()) {
        return color;
    } else {
        return d->mBaseConfig.mDefaultResourceColor;
    }
}

QStringList Prefs::timeScaleTimezones() const
{
    return d->mBaseConfig.timeScaleTimezones();
}

void Prefs::setTimeScaleTimezones(const QStringList &timeZones)
{
    d->mBaseConfig.setTimeScaleTimezones(timeZones);
}

bool Prefs::use24HourClock() const
{
    return d->mBaseConfig.use24HourClock();
}

void Prefs::setUse24HourClock(bool enable)
{
    d->mBaseConfig.setUse24HourClock(enable);
}

bool Prefs::useDualLabels() const
{
    return d->mBaseConfig.useDualLabels();
}

void Prefs::setUseDualLabels(bool enable)
{
    d->mBaseConfig.setUseDualLabels(enable);
}

KConfigSkeleton::ItemFont *Prefs::fontItem(const QString &name) const
{
    KConfigSkeletonItem *item = d->mAppConfig ? d->mAppConfig->findItem(name) : nullptr;

    if (!item) {
        item = d->mBaseConfig.findItem(name);
    }

    return dynamic_cast<KConfigSkeleton::ItemFont *>(item);
}

QStringList Prefs::selectedPlugins() const
{
    return d->mBaseConfig.mSelectedPlugins;
}

QStringList Prefs::decorationsAtAgendaViewTop() const
{
    return d->mBaseConfig.decorationsAtAgendaViewTop();
}

QStringList Prefs::decorationsAtAgendaViewBottom() const
{
    return d->mBaseConfig.decorationsAtAgendaViewBottom();
}

void Prefs::setSelectedPlugins(const QStringList &plugins)
{
    d->mBaseConfig.setSelectedPlugins(plugins);
}

void Prefs::setDecorationsAtAgendaViewTop(const QStringList &decorations)
{
    d->mBaseConfig.setDecorationsAtAgendaViewTop(decorations);
}

void Prefs::setDecorationsAtAgendaViewBottom(const QStringList &decorations)
{
    d->mBaseConfig.setDecorationsAtAgendaViewBottom(decorations);
}

QSet<EventViews::EventView::ItemIcon> Prefs::agendaViewIcons() const
{
    return d->mBaseConfig.mAgendaViewIcons;
}

void Prefs::setAgendaViewIcons(const QSet<EventViews::EventView::ItemIcon> &icons)
{
    d->mBaseConfig.mAgendaViewIcons = icons;
}

QSet<EventViews::EventView::ItemIcon> Prefs::monthViewIcons() const
{
    return d->mBaseConfig.mMonthViewIcons;
}

void Prefs::setMonthViewIcons(const QSet<EventViews::EventView::ItemIcon> &icons)
{
    d->mBaseConfig.mMonthViewIcons = icons;
}

void Prefs::setFlatListTodo(bool enable)
{
    d->mBaseConfig.mFlatListTodo = enable;
}

bool Prefs::flatListTodo() const
{
    return d->mBaseConfig.mFlatListTodo;
}

void Prefs::setFullViewTodo(bool enable)
{
    d->mBaseConfig.mFullViewTodo = enable;
}

bool Prefs::fullViewTodo() const
{
    return d->mBaseConfig.mFullViewTodo;
}

bool Prefs::enableTodoQuickSearch() const
{
    return d->mBaseConfig.mEnableTodoQuickSearch;
}

void Prefs::setEnableTodoQuickSearch(bool enable)
{
    d->mBaseConfig.mEnableTodoQuickSearch = enable;
}

bool Prefs::enableQuickTodo() const
{
    return d->mBaseConfig.mEnableQuickTodo;
}

void Prefs::setEnableQuickTodo(bool enable)
{
    d->mBaseConfig.mEnableQuickTodo = enable;
}

bool Prefs::highlightTodos() const
{
    return d->mBaseConfig.mHighlightTodos;
}

void Prefs::setHighlightTodos(bool highlight)
{
    d->mBaseConfig.mHighlightTodos = highlight;
}

KConfig *Prefs::config() const
{
    return d->mAppConfig ? d->mAppConfig->config() : d->mBaseConfig.config();
}
