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

#include "catchresult.h"
#include "catchconstants.h"
#include "../testframeworkmanager.h"
#include "../testtreeitem.h"

#include <coreplugin/id.h>

namespace Autotest {
namespace Internal {

CatchResult::CatchResult(const QString &projectFile, const QString &name)
    : TestResult(name), m_projectFile(projectFile)
{
}

CatchResult::CatchResult(const QString &executable, const QString &projectFile,
                         const QString &name)
    : TestResult(executable, name), m_projectFile(projectFile)
{
}

const QString CatchResult::outputString(bool selected) const
{
    const QString &desc = description();
    QString output;
    switch (result()) {
    case Result::Pass:
    case Result::Fail:
        output = m_testSetName;
        if (selected && !desc.isEmpty())
            output.append('\n').append(desc);
        break;
    default:
        output = desc;
        if (!selected)
            output = output.split('\n').first();
    }
    return output;
}

bool CatchResult::isDirectParentOf(const TestResult *other, bool *needsIntermediate) const
{
    if (!TestResult::isDirectParentOf(other, needsIntermediate))
        return false;

    const CatchResult *gtOther = static_cast<const CatchResult *>(other);
    if (m_iteration != gtOther->m_iteration)
        return false;
    return isTest() && gtOther->isTestSet();
}

static QString normalizeName(const QString &name)
{
    static QRegExp parameterIndex("/\\d+");

    QString nameWithoutParameterIndices = name;
    nameWithoutParameterIndices.remove(parameterIndex);

    return nameWithoutParameterIndices.split('/').last();
}

static QString normalizeTestName(const QString &testname)
{
    QString nameWithoutTypeParam = testname.split(',').first();

    return normalizeName(nameWithoutTypeParam);
}

const TestTreeItem *CatchResult::findTestTreeItem() const
{
    auto id = Core::Id(Constants::FRAMEWORK_PREFIX).withSuffix(Catch::Constants::FRAMEWORK_NAME);
    const TestTreeItem *rootNode = TestFrameworkManager::instance()->rootNodeForTestFramework(id);
    if (!rootNode)
        return nullptr;

    const auto item = rootNode->findAnyChild([this](const Utils::TreeItem *item) {
        const auto treeItem = static_cast<const TestTreeItem *>(item);
        return treeItem && matches(treeItem);
    });
    return static_cast<const TestTreeItem *>(item);
}

bool CatchResult::matches(const TestTreeItem *treeItem) const
{
    if (treeItem->proFile() != m_projectFile)
        return false;

    if (isTest())
        return matchesTestCase(treeItem);

    return matchesTestFunctionOrSet(treeItem);
}

bool CatchResult::matchesTestFunctionOrSet(const TestTreeItem *treeItem) const
{
    if (treeItem->type() != TestTreeItem::TestFunctionOrSet)
        return false;

    const QString testItemTestSet = treeItem->parentItem()->name() + '.' + treeItem->name();
    return testItemTestSet == normalizeName(m_testSetName);
}

bool CatchResult::matchesTestCase(const TestTreeItem *treeItem) const
{
    if (treeItem->type() != TestTreeItem::TestCase)
        return false;

    return treeItem->name() == normalizeTestName(name());
}

} // namespace Internal
} // namespace Autotest
