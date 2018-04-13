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

#include "catchconfiguration.h"
#include "catchconstants.h"
#include "catchoutputreader.h"
#include "catchsettings.h"
#include "../autotestplugin.h"
#include "../testframeworkmanager.h"
#include "../testsettings.h"

#include <utils/algorithm.h>

namespace Autotest {
namespace Internal {

TestOutputReader *CatchConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi,
                                                   QProcess *app) const
{
    return new CatchOutputReader(fi, app, buildDirectory(), projectFile());
}

static QStringList filterInterfering(const QStringList &provided, QStringList *omitted)
{
    static const QSet<QString> knownInterferingOptions { "--catch_list_tests",
                                                         "--catch_filter=",
                                                         "--catch_also_run_disabled_tests",
                                                         "--catch_repeat=",
                                                         "--catch_shuffle",
                                                         "--catch_random_seed=",
                                                         "--catch_output=",
                                                         "--catch_stream_result_to=",
                                                         "--catch_break_on_failure",
                                                         "--catch_throw_on_failure",
                                                         "--catch_color="
                                                         };

    QSet<QString> allowed = Utils::filtered(provided.toSet(), [] (const QString &arg) {
        return Utils::allOf(knownInterferingOptions, [&arg] (const QString &interfering) {
            return !arg.startsWith(interfering);
        });
    });

    if (omitted) {
        QSet<QString> providedSet = provided.toSet();
        providedSet.subtract(allowed);
        omitted->append(providedSet.toList());
    }
    return allowed.toList();
}

QStringList CatchConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    static const Core::Id id
            = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(Catch::Constants::FRAMEWORK_NAME);

    QStringList arguments;
    if (AutotestPlugin::settings()->processArgs) {
        arguments << filterInterfering(runnable().commandLineArguments.split(
                                           ' ', QString::SkipEmptyParts), omitted);
    }

    const QStringList &testSets = testCases();
    if (testSets.size())
        arguments << "--catch_filter=" + testSets.join(':');

    TestFrameworkManager *manager = TestFrameworkManager::instance();
    auto gSettings = qSharedPointerCast<CatchSettings>(manager->settingsForTestFramework(id));
    if (gSettings.isNull())
        return arguments;

    if (gSettings->runDisabled)
        arguments << "--catch_also_run_disabled_tests";
    if (gSettings->repeat)
        arguments << QString("--catch_repeat=%1").arg(gSettings->iterations);
    if (gSettings->shuffle)
        arguments << "--catch_shuffle" << QString("--catch_random_seed=%1").arg(gSettings->seed);
    if (gSettings->throwOnFailure)
        arguments << "--catch_throw_on_failure";

    if (isDebugRunMode()) {
        if (gSettings->breakOnFailure)
            arguments << "--catch_break_on_failure";
    }
    return arguments;
}

} // namespace Internal
} // namespace Autotest
