/*
  SPDX-FileCopyrightText: 2023 Kevin Ottens <ervin@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

#include "eventviews_export.h"
#include "prefs.h"

#include <QIdentityProxyModel>

#include <memory>

class ColoredTodoProxyModelPrivate;

/**
 * ProxyModel adding background color for overdue and due today items in a TodoModel
 */
class ColoredTodoProxyModel : public QIdentityProxyModel
{
public:
    explicit ColoredTodoProxyModel(const EventViews::PrefsPtr &preferences, QObject *parent = nullptr);
    ~ColoredTodoProxyModel() override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

private:
    std::unique_ptr<ColoredTodoProxyModelPrivate> const d;
};
