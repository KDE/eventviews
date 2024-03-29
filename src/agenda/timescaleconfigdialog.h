/*
  SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno@virlet.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/
#pragma once

#include "ui_timescaleedit_base.h"

#include <QDialog>

#include <memory>

namespace EventViews
{
class Prefs;
using PrefsPtr = QSharedPointer<Prefs>;

class TimeScaleConfigDialogPrivate;

class TimeScaleConfigDialog : public QDialog, private Ui::TimeScaleEditWidget
{
    Q_OBJECT

public:
    TimeScaleConfigDialog(const PrefsPtr &preferences, QWidget *parent);
    ~TimeScaleConfigDialog() override;

private:
    void add();
    void remove();
    void up();
    void down();
    void okClicked();

    void slotUpdateButton();
    QStringList zones() const;

private:
    std::unique_ptr<TimeScaleConfigDialogPrivate> const d;
};
}
