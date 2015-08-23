#ifndef SAXHANDLER_H
#define SAXHANDLER_H

#include <QXmlDefaultHandler>
class QTreeWidget;
class QTreeWidgetItem;

class SaxHandler : public QXmlDefaultHandler
{
public:
    SaxHandler(QTreeWidget *tree);
    bool startElement(const QString &namespaceURI,
                          const QString &localName,
                          const QString &qName,
                          const QXmlAttributes &attributes);
    bool endElement(const QString &namespaceURI,
                        const QString &localName,
                        const QString &qName);
    bool characters(const QString &str);
    bool fatalError(const QXmlParseException &exception);

    QTreeWidget *getXmlTree(){return treeWidget;}
signals:

public slots:

private:
    QTreeWidgetItem *currentItem;
    QString currentText;
    int depth;                      /*当前解析的深度*/
    QTreeWidget *treeWidget;
};

#endif // SAXHANDLER_H
