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

#include "catchparser.h"
#include "catchtreeitem.h"
#include "catchvisitors.h"
#include "catch_utils.h"

#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectpart.h>

namespace Autotest {
namespace Internal {

TestTreeItem *CatchParseResult::createTestTreeItem() const
{
    if (itemType != TestTreeItem::TestCase && itemType != TestTreeItem::TestFunctionOrSet)
        return nullptr;
    CatchTreeItem *item = new CatchTreeItem(name, fileName, itemType);
    item->setProFile(proFile);
    item->setLine(line);
    item->setColumn(column);

    if (parameterized)
        item->setState(CatchTreeItem::Parameterized);
    if (typed)
        item->setState(CatchTreeItem::Typed);
    if (disabled)
        item->setState(CatchTreeItem::Disabled);
    for (const TestParseResult *testSet : children)
        item->appendChild(testSet->createTestTreeItem());
    return item;
}

static bool includesCatch(const CPlusPlus::Document::Ptr &doc,
                          const CPlusPlus::Snapshot &snapshot)
{
    static const QString catchH("catch.hpp");
    for (const CPlusPlus::Document::Include &inc : doc->resolvedIncludes()) {
        if (inc.resolvedFileName().endsWith(catchH))
            return true;
    }

    for (const QString &include : snapshot.allIncludesForDocument(doc->fileName())) {
        if (include.endsWith(catchH))
            return true;
    }

    return false;
}

static bool hasCatchNames(const CPlusPlus::Document::Ptr &document)
{
    for (const CPlusPlus::Document::MacroUse &macro : document->macroUses()) {
        if (!macro.isFunctionLike())
            continue;
        if (CatchUtils::isCatchMacro(QLatin1String(macro.macro().name()))) {
            const QVector<CPlusPlus::Document::Block> args = macro.arguments();
            if (args.size() != 2)
                continue;
            return true;
        }
    }
    return false;
}

static bool handleCatch(QFutureInterface<TestParseResultPtr> futureInterface,
                        const CPlusPlus::Document::Ptr &doc,
                        const CPlusPlus::Snapshot &snapshot,
                        const Core::Id &id)
{
    const CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();
    const QString &filePath = doc->fileName();
    const QByteArray &fileContent = CppParser::getFileContent(filePath);
    CPlusPlus::Document::Ptr document = snapshot.preprocessedDocument(fileContent, filePath);
    document->check();
    CPlusPlus::AST *ast = document->translationUnit()->ast();
    CatchVisitor visitor(document);
    visitor.accept(ast);

    QMap<CatchCaseSpec, CatchCodeLocationList> result = visitor.catchFunctions();
    QString proFile;
    const QList<CppTools::ProjectPart::Ptr> &ppList = modelManager->projectPart(filePath);
    if (ppList.size())
        proFile = ppList.first()->projectFile;
    else
        return false; // happens if shutting down while parsing

    for (const CatchCaseSpec &testSpec : result.keys()) {
        CatchParseResult *parseResult = new CatchParseResult(id);
        parseResult->itemType = TestTreeItem::TestCase;
        parseResult->fileName = filePath;
        parseResult->name = testSpec.testCaseName;
        parseResult->parameterized = testSpec.parameterized;
        parseResult->typed = testSpec.typed;
        parseResult->disabled = testSpec.disabled;
        parseResult->proFile = proFile;

        for (const CatchCodeLocationAndType &location : result.value(testSpec)) {
            CatchParseResult *testSet = new CatchParseResult(id);
            testSet->name = location.m_name;
            testSet->fileName = filePath;
            testSet->line = location.m_line;
            testSet->column = location.m_column;
            testSet->disabled = location.m_state & CatchTreeItem::Disabled;
            testSet->itemType = location.m_type;
            testSet->proFile = proFile;

            parseResult->children.append(testSet);
        }

        futureInterface.reportResult(TestParseResultPtr(parseResult));
    }
    return !result.keys().isEmpty();
}

bool CatchParser::processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                  const QString &fileName)
{
    if (!m_cppSnapshot.contains(fileName) || !selectedForBuilding(fileName))
        return false;
    CPlusPlus::Document::Ptr document = m_cppSnapshot.find(fileName).value();
    if (!includesCatch(document, m_cppSnapshot) || !hasCatchNames(document))
        return false;
    return handleCatch(futureInterface, document, m_cppSnapshot, id());
}

} // namespace Internal
} // namespace Autotest
