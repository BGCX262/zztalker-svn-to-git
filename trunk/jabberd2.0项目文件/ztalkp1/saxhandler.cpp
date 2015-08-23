#include <QtGui>
#include "saxhandler.h"

SaxHandler::SaxHandler(QTreeWidget *tree)
{
    treeWidget = tree;
    treeWidget->clear();
    depth = 0;
    currentItem = 0;

    /******************调试信息********************************/
    QStringList labels;
    labels << QObject::tr("element") << QObject::tr("content");
    treeWidget->setHeaderLabels(labels);
    treeWidget->setWindowTitle(QObject::tr("DebugWindow"));
}

bool SaxHandler::startElement(const QString &namespaceURI ,
                              const QString &localName,
                              const QString &qName,
                              const QXmlAttributes &attributes)
{
    ++depth;
    if (currentItem) {
        currentItem = new QTreeWidgetItem(currentItem);
    } else {
        currentItem = new QTreeWidgetItem(treeWidget);
    }
    currentItem->setText(0, qName);
    QString attrs;
    attrs += QString("namespace=")+namespaceURI+QString(";");
    attrs += QString("localName=")+localName+QString(";");
    for(int i=0; i<attributes.count(); i++)
    {
        attrs += attributes.qName(i) + QString("=");
        attrs += attributes.value(i) + QString(";");
    }
    currentItem->setData(0, Qt::UserRole, attrs);
    return true;
}

bool SaxHandler::endElement(const QString & /* namespaceURI */,
                            const QString & /* localName */,
                            const QString &qName)
{
    --depth;
    if(depth == 0)
        currentItem = 0;
    else
        currentItem = currentItem->parent();
    return true;
}

bool SaxHandler::characters(const QString &str)
{
    currentItem->setText(1, str);
    return true;
}

bool SaxHandler::fatalError(const QXmlParseException &exception)
{
    QMessageBox::warning(0, QObject::tr("SAX Handler"),
                         QObject::tr("Parse error at line %1, column "
                                     "%2:\n%3.")
                         .arg(exception.lineNumber())
                         .arg(exception.columnNumber())
                         .arg(exception.message()));

    return true;
}



