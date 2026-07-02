/*
  SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.net>
  SPDX-FileContributor: Sergio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include <QString>
class KCheckableProxyModel;

namespace EventViews
{
class ConfigDialogInterface
{
public:
    virtual ~ConfigDialogInterface() = default;

    virtual int numberOfColumns() const = 0;
    virtual bool useCustomColumns() const = 0;
    virtual QString columnTitle(int column) const = 0;

    virtual KCheckableProxyModel *takeSelectionModel(int column) = 0;
};
}
