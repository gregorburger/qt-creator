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

#include "catchtreeitem.h"
#include "catchconfiguration.h"
#include "catchparser.h"
#include "../testframeworkmanager.h"

#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/session.h>
#include <utils/algorithm.h>
#include <utils/asconst.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

static QString catchFilter(CatchTreeItem::TestStates states)
{
    if ((states & CatchTreeItem::Parameterized) && (states & CatchTreeItem::Typed))
        return QString("*/%1/*.%2");
    if (states & CatchTreeItem::Parameterized)
        return QString("*/%1.%2/*");
    if (states & CatchTreeItem::Typed)
        return QString("%1/*.%2");
    return QString("%1.%2");
}

QVariant CatchTreeItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole: {
        if (type() == TestTreeItem::Root)
            break;

        const QString &displayName = (m_state & Disabled) ? name().mid(9) : name();
        return QVariant(displayName + nameSuffix());
    }
    case Qt::CheckStateRole:
        switch (type()) {
        case Root:
        case GroupNode:
        case TestCase:
        case TestFunctionOrSet:
            return checked();
        default:
            return QVariant();
        }
    case ItalicRole:
        return false;
    case EnabledRole:
        return !(m_state & Disabled);
    default:
        break;
    }
    return TestTreeItem::data(column, role);
}

TestConfiguration *CatchTreeItem::testConfiguration() const
{
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    QTC_ASSERT(project, return nullptr);

    CatchConfiguration *config = nullptr;
    switch (type()) {
    case TestCase: {
        const QString &testSpecifier = catchFilter(state()).arg(name()).arg('*');
        if (int count = childCount()) {
            config = new CatchConfiguration;
            config->setTestCases(QStringList(testSpecifier));
            config->setTestCaseCount(count);
            config->setProjectFile(proFile());
            config->setProject(project);
        }
        break;
    }
    case TestFunctionOrSet: {
        CatchTreeItem *parent = static_cast<CatchTreeItem *>(parentItem());
        if (!parent)
            return nullptr;
        const QString &testSpecifier = catchFilter(parent->state()).arg(parent->name()).arg(name());
        config = new CatchConfiguration;
        config->setTestCases(QStringList(testSpecifier));
        config->setProjectFile(proFile());
        config->setProject(project);
        break;
    }
    default:
        return nullptr;
    }
    if (config)
        config->setInternalTargets(internalTargets());
    return config;
}

TestConfiguration *CatchTreeItem::debugConfiguration() const
{
    CatchConfiguration *config = static_cast<CatchConfiguration *>(testConfiguration());
    if (config)
        config->setRunMode(TestRunMode::Debug);
    return config;
}

struct TestCases
{
    QStringList filters;
    int testSetCount = 0;
    QSet<QString> internalTargets;
};

static void collectTestInfo(const CatchTreeItem *item,
                            QHash<QString, TestCases> &testCasesForProFile,
                            bool ignoreCheckState)
{
    QTC_ASSERT(item, return);
    if (item->type() == TestTreeItem::GroupNode) {
        for (int row = 0, count = item->childCount(); row < count; ++row) {
            auto child = static_cast<const CatchTreeItem *>(item->childItem(row));
            collectTestInfo(child, testCasesForProFile, ignoreCheckState);
        }
        return;
    }
    const int childCount = item->childCount();
    QTC_ASSERT(childCount != 0, return);
    QTC_ASSERT(item->type() == TestTreeItem::TestCase, return);
    if (ignoreCheckState || item->checked() == Qt::Checked) {
        const QString &projectFile = item->childItem(0)->proFile();
        testCasesForProFile[projectFile].filters.append(
                    catchFilter(item->state()).arg(item->name()).arg('*'));
        testCasesForProFile[projectFile].testSetCount += childCount - 1;
        testCasesForProFile[projectFile].internalTargets.unite(item->internalTargets());
    } else if (item->checked() == Qt::PartiallyChecked) {
        for (int childRow = 0; childRow < childCount; ++childRow) {
            const TestTreeItem *child = item->childItem(childRow);
            QTC_ASSERT(child->type() == TestTreeItem::TestFunctionOrSet, continue);
            if (child->checked() == Qt::Checked) {
                testCasesForProFile[child->proFile()].filters.append(
                            catchFilter(item->state()).arg(item->name()).arg(child->name()));
                testCasesForProFile[child->proFile()].internalTargets.unite(
                            child->internalTargets());
            }
        }
    }
}

QList<TestConfiguration *> CatchTreeItem::getTestConfigurations(bool ignoreCheckState) const
{
    QList<TestConfiguration *> result;
    ProjectExplorer::Project *project = ProjectExplorer::SessionManager::startupProject();
    if (!project || type() != Root)
        return result;

    QHash<QString, TestCases> testCasesForProFile;
    for (int row = 0, count = childCount(); row < count; ++row) {
        auto child = static_cast<const CatchTreeItem *>(childItem(row));
        collectTestInfo(child, testCasesForProFile, ignoreCheckState);
    }

    for (auto it = testCasesForProFile.begin(), end = testCasesForProFile.end(); it != end; ++it) {
        for (const QString &target : Utils::asConst(it.value().internalTargets)) {
            CatchConfiguration *tc = new CatchConfiguration;
            if (!ignoreCheckState)
                tc->setTestCases(it.value().filters);
            tc->setTestCaseCount(tc->testCaseCount() + it.value().testSetCount);
            tc->setProjectFile(it.key());
            tc->setProject(project);
            tc->setInternalTarget(target);
            result << tc;
        }
    }

    return result;
}

QList<TestConfiguration *> CatchTreeItem::getAllTestConfigurations() const
{
    return getTestConfigurations(true);
}

QList<TestConfiguration *> CatchTreeItem::getSelectedTestConfigurations() const
{
    return getTestConfigurations(false);
}

TestTreeItem *CatchTreeItem::find(const TestParseResult *result)
{
    QTC_ASSERT(result, return nullptr);

    const CatchParseResult *parseResult = static_cast<const CatchParseResult *>(result);
    CatchTreeItem::TestStates states = parseResult->disabled ? CatchTreeItem::Disabled
                                                             : CatchTreeItem::Enabled;
    if (parseResult->parameterized)
        states |= CatchTreeItem::Parameterized;
    if (parseResult->typed)
        states |= CatchTreeItem::Typed;
    switch (type()) {
    case Root:
        if (TestFrameworkManager::instance()->groupingEnabled(result->frameworkId)) {
            const QFileInfo fileInfo(parseResult->fileName);
            const QFileInfo base(fileInfo.absolutePath());
            for (int row = 0; row < childCount(); ++row) {
                CatchTreeItem *group = static_cast<CatchTreeItem *>(childAt(row));
                if (group->filePath() != base.absoluteFilePath())
                    continue;
                if (auto groupChild = group->findChildByNameStateAndFile(parseResult->name, states,
                                                                         parseResult->proFile)) {
                    return groupChild;
                }
            }
            return nullptr;
        }
        return findChildByNameStateAndFile(parseResult->name, states, parseResult->proFile);
    case GroupNode:
        return findChildByNameStateAndFile(parseResult->name, states, parseResult->proFile);
    case TestCase:
        return findChildByNameAndFile(result->name, result->fileName);
    default:
        return nullptr;
    }
}

bool CatchTreeItem::modify(const TestParseResult *result)
{
    QTC_ASSERT(result, return false);

    switch (type()) {
    case TestFunctionOrSet:
        return modifyTestSetContent(static_cast<const CatchParseResult *>(result));
    default:
        return false;
    }
}

TestTreeItem *CatchTreeItem::createParentGroupNode() const
{
    if (type() != TestCase)
        return nullptr;
    const QFileInfo fileInfo(filePath());
    const QFileInfo base(fileInfo.absolutePath());
    return new CatchTreeItem(base.baseName(), fileInfo.absolutePath(), TestTreeItem::GroupNode);
}

bool CatchTreeItem::modifyTestSetContent(const CatchParseResult *result)
{
    bool hasBeenModified = modifyLineAndColumn(result);
    CatchTreeItem::TestStates states = result->disabled ? CatchTreeItem::Disabled
                                                        : CatchTreeItem::Enabled;
    if (m_state != states) {
        m_state = states;
        hasBeenModified = true;
    }
    return hasBeenModified;
}

TestTreeItem *CatchTreeItem::findChildByNameStateAndFile(const QString &name,
                                                         CatchTreeItem::TestStates state,
                                                         const QString &proFile) const
{
    return findChildBy([name, state, proFile](const TestTreeItem *other) -> bool {
        const CatchTreeItem *catchItem = static_cast<const CatchTreeItem *>(other);
        return other->proFile() == proFile
                && other->name() == name
                && catchItem->state() == state;
    });
}

QString CatchTreeItem::nameSuffix() const
{
    static QString markups[] = {QCoreApplication::translate("CatchTreeItem", "parameterized"),
                                QCoreApplication::translate("CatchTreeItem", "typed")};
    QString suffix;
    if (m_state & Parameterized)
        suffix =  QString(" [") + markups[0];
    if (m_state & Typed)
        suffix += (suffix.isEmpty() ? QString(" [") : QString(", ")) + markups[1];
    if (!suffix.isEmpty())
        suffix += ']';
    return suffix;
}

QSet<QString> CatchTreeItem::internalTargets() const
{
    QSet<QString> result;
    const auto cppMM = CppTools::CppModelManager::instance();
    const auto projectInfo = cppMM->projectInfo(ProjectExplorer::SessionManager::startupProject());
    const QString file = filePath();
    const QVector<CppTools::ProjectPart::Ptr> projectParts = projectInfo.projectParts();
    if (projectParts.isEmpty())
        return TestTreeItem::dependingInternalTargets(cppMM, file);
    for (const CppTools::ProjectPart::Ptr projectPart : projectParts) {
        if (projectPart->projectFile == proFile()
                && Utils::anyOf(projectPart->files, [&file] (const CppTools::ProjectFile &pf) {
                                return pf.path == file;
        })) {
            result.insert(projectPart->buildSystemTarget + '|' + projectPart->projectFile);
            if (projectPart->buildTargetType != CppTools::ProjectPart::Executable)
                result.unite(TestTreeItem::dependingInternalTargets(cppMM, file));
        }
    }
    return result;
}

} // namespace Internal
} // namespace Autotest
