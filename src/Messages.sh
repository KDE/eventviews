#! /bin/sh
# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

$EXTRACTRC `find . -name '*.ui' -or -name '*.kcfg'` >> rc.cpp || exit 11
$XGETTEXT `find . -name "*.cpp" | grep -v '/tests/'` -o $podir/libeventviews.pot
rm -f rc.cpp
