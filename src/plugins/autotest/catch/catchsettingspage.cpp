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

#include "../autotestconstants.h"
#include "catchconstants.h"
#include "catchsettingspage.h"
#include "catchsettings.h"

#include <coreplugin/icore.h>

namespace Autotest {
namespace Internal {

CatchSettingsWidget::CatchSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_ui.setupUi(this);
    connect(m_ui.repeatCatchsCB, &QCheckBox::toggled, m_ui.repetitionSpin, &QSpinBox::setEnabled);
    connect(m_ui.shuffleCatchsCB, &QCheckBox::toggled, m_ui.seedSpin, &QSpinBox::setEnabled);
}

void CatchSettingsWidget::setSettings(const CatchSettings &settings)
{
    m_ui.runDisabledCatchsCB->setChecked(settings.runDisabled);
    m_ui.repeatCatchsCB->setChecked(settings.repeat);
    m_ui.shuffleCatchsCB->setChecked(settings.shuffle);
    m_ui.repetitionSpin->setValue(settings.iterations);
    m_ui.seedSpin->setValue(settings.seed);
    m_ui.breakOnFailureCB->setChecked(settings.breakOnFailure);
    m_ui.throwOnFailureCB->setChecked(settings.throwOnFailure);
}

CatchSettings CatchSettingsWidget::settings() const
{
    CatchSettings result;
    result.runDisabled = m_ui.runDisabledCatchsCB->isChecked();
    result.repeat = m_ui.repeatCatchsCB->isChecked();
    result.shuffle = m_ui.shuffleCatchsCB->isChecked();
    result.iterations = m_ui.repetitionSpin->value();
    result.seed = m_ui.seedSpin->value();
    result.breakOnFailure = m_ui.breakOnFailureCB->isChecked();
    result.throwOnFailure = m_ui.throwOnFailureCB->isChecked();
    return result;
}

CatchSettingsPage::CatchSettingsPage(QSharedPointer<IFrameworkSettings> settings,
                                     const ITestFramework *framework)
    : ITestSettingsPage(framework),
      m_settings(qSharedPointerCast<CatchSettings>(settings)),
      m_widget(0)
{
    setDisplayName(QCoreApplication::translate("CatchFramework",
                                               Catch::Constants::FRAMEWORK_SETTINGS_CATEGORY));
}

QWidget *CatchSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new CatchSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void CatchSettingsPage::apply()
{
    if (!m_widget) // page was not shown at all
        return;
    *m_settings = m_widget->settings();
    m_settings->toSettings(Core::ICore::settings());
}

} // namespace Internal
} // namespace Autotest
