/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace WinRt {
namespace Internal {

class WinRtRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT

public:
    explicit WinRtRunConfiguration(ProjectExplorer::Target *target);

    QWidget *createConfigurationWidget() override;
    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &map) override;

    const QString &proFilePath() const { return m_proFilePath; }
    QString arguments() const;
    bool uninstallAfterStop() const { return m_uninstallAfterStop; }
    void setUninstallAfterStop(bool b);

    QString buildSystemTarget() const final;

    ProjectExplorer::Runnable runnable() const override;

signals:
    void argumentsChanged(QString);
    void uninstallAfterStopChanged(bool);

private:
    QString extraId() const final;

    QString m_proFilePath;
    bool m_uninstallAfterStop = false;

    QString executable() const;
};

} // namespace Internal
} // namespace WinRt
