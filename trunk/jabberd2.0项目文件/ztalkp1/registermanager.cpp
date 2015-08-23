#include <QTcpSocket>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <iostream>
#include "saxhandler.h"
#include "registermanager.h"
#include "util.h"

RegisterManager::RegisterManager(QWidget *parent) :
    QDialog(parent)
{
    status = NONE;
    tcpSocket = new QTcpSocket();
    xmlTree = new QTreeWidget;
    xmlHandler = new SaxHandler(xmlTree);

    connect(tcpSocket, SIGNAL(readyRead()),
            this, SLOT(handleRead()));
    connect(tcpSocket, SIGNAL(disconnected()),
            this, SLOT(connectionClosedByServer()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(socketError()));
    connect(this, SIGNAL(send(QString)),
            this, SLOT(sendRequest(QString)));
}

RegisterManager *RegisterManager::rm = NULL;
RegisterManager* RegisterManager::getRegisterManager()
{
    if(!rm)
    {
        rm = new RegisterManager;
    }
    return rm;
}

void RegisterManager::connectionClosedByServer()
{
    std::cerr << "Connection closed by the server.\n";
    tcpSocket->close();
}

void RegisterManager::socketError()
{
    std::cerr << "From socket: "
            << tcpSocket->errorString().toStdString()
            <<"\n";
    tcpSocket->close();
}

/*向socket里面发送数据*/
void RegisterManager::sendRequest(QString request)
{
    tcpSocket->write(request.toStdString().c_str());
    std::cerr << "<!-- OUT -->\n"<< request.toStdString() << "\n\n";
}


bool RegisterManager::start(QMap<QString, QString> user)
{
    /*注册需要的用户信息,包括node,domain,password,email*/
    this->user = user;
    if(user["name"].isEmpty()){
        errString = QString("Miss name");
        return false;
    }else if(user["domain"].isEmpty()){
        errString = QString("Miss domain");
        return false;
    }else if(user["password"].isEmpty()){
        errString = QString("Miss password");
        return false;
    }

    /*第一步:连接到服务器*/
    tcpSocket->connectToHost(user["domain"], defaultPort);
    if(!tcpSocket->waitForConnected(5000)){
        errString = tcpSocket->errorString();
        return false;
    }

    /*初始化信息*/
    outData = QString("<?xml version='1.0'?>"
                     "<stream:stream xmlns='jabber:client'"
                     " to='%1' version='1.0'"
                     " xmlns:stream='http://etherx.jabber.org/streams'"
                     " xml:lang='zh'>").arg(user["domain"]);
    emit send(outData);
    return true;
}

void RegisterManager::handleRead()
{
    int count = tcpSocket->bytesAvailable();
    if(count <= 0)
        return;

    char dataIn[count+1];
    memset(dataIn, 0, count+1);

    //从socket中读取数据
    tcpSocket->read(dataIn, count);
    QString data = QString(dataIn);

    /*打印接收到的服务器数据*/
    std::cerr << "<!-- IN -->\n" << dataIn << "\n\n";

    //对数据进行预处理
    /*如果数据包交换已经结束，即已经收到</stream:stream>数据包*/
    if(data.contains(QString("</stream:stream>"), Qt::CaseInsensitive))
    {
        /*****************************************************
         *我们仍然需要处理</stream:stream>之前的部分数据
         *此处我们暂时不考虑，但是要注意时候必须补上，此处一定会出错！
         **************************************************/
        tcpSocket->close();
        return;
    }

    /*如果是第一个数据包，必须在后面加一个</stream:stream>, 否则解析的过程中会报错*/
    if(data.contains(QString("<?xml version='1.0'?>"), Qt::CaseInsensitive))
    {
        data += QString("</stream:stream>");
    }
    /**
     *为了能正确的解析数据，对数据进行预处理
     *所有的非初始数据都将被<stream:stream />对包裹
     */
    else{
        data = QString("<stream:stream>")+data+QString("</stream:stream>");
    }

    /*解析xml流,生成xml树*/
    xmlTree->clear();
    QXmlInputSource inputSource;
    inputSource.setData(data);
    QXmlSimpleReader reader;
    reader.setContentHandler(xmlHandler);
    reader.setErrorHandler(xmlHandler);
    reader.parse(inputSource);


    if(!parseTree())
    {
        QMessageBox::warning(0, tr("Error"),
                             tr("Error: %1\n").arg(getError()));
        return;
    }

    /*根据解析生成的状态信息，生成回复*/
    outData = generateResponse();

    if(status >= NEEDMOREDATA)
        return;

    if(outData.isEmpty())
    {
        QMessageBox::warning(0, tr("Error"),
                             tr("Error: %1\n").arg(getError()));
    }else{
        emit send(outData);
    }

}

bool RegisterManager::parseTree()
{
    QTreeWidgetItem *root = xmlTree->itemAt(0,0);
    /*根据协定，我们的xmlTree总是以<stream></stream>包裹*/
    int count = root->childCount();
    if(0 == count)
    {
        errString = QString("Data stream needs more data!");
        /*虽然出错，但是我们等待下一轮数据*/
        status = NEEDMOREDATA;
        return true;

    }

    for(int i=0; i<count; i++)
    {
        QTreeWidgetItem *item = root->child(i);
        if(item->text(0)=="stream:features"
           || item->text(0) == "features"){
            status = INITIALIZED;
        }else if(item->text(0)=="iq"){
            if(!parseIqItem(item))
                return false;
        }
    }
    return true;
}

bool RegisterManager::parseIqItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *iqItem = item;
    QMap<QString,QString> attrs = getAttrList(iqItem);
    if(attrs["type"]=="result")
    {
        int childCount = iqItem->childCount();
        if(childCount==0)
        {
            status = REGISTERSUCCESS;
            return true;
        }

        for(int j=0; j<childCount; j++){
            QTreeWidgetItem *child = iqItem->child(j);
            if(child->text(0) == "query"){
                int count = child->childCount();
                for(int i=0; i<count; i++)
                {
                    QString text = child->child(i)->text(0);
                    if(text=="instructions")
                        continue;
                    else
                    {
                        needInfo.append(text);
                    }
                }
                status = REGISTERINFO;
            }
        }
    }else if(attrs["type"]=="error"){
        int childCount = iqItem->childCount();
        for(int j=0; j<childCount; j++){
            QTreeWidgetItem *child = iqItem->child(j);
            if(child->text(0) == "error"){
                errString = child->child(0)->text(0);
                status = REGISTERERROR;
                break;
            }
        }
    }
    return true;
}

QString RegisterManager::generateResponse()
{
    outData.clear();
    QString response;
    switch(status){
    case NONE:
    case NEEDMOREDATA:
        break;
    case INITIALIZED:
        response = QString("<iq type='get' id='reg1'>"
                           "<query xmlns='jabber:iq:register'/></iq>");
        break;
    case REGISTERINFO:
        int count;
        int i;
        count = needInfo.count();
        response = QString("<iq type='set' id='reg'><query xmlns='jabber:iq:register'>");

        for(i=0; i<count; i++)
        {
            QString text = needInfo.at(i);
            if(text=="username"){
                QString userName = user["name"];
                response += QString("<username>%1</username>").arg(userName);
            }else if(text == "password"){
                response += QString("<password>%1</password>").arg(user["password"]);
            }else if(text=="email"){
                response += QString("<email>%1</email>").arg(user["email"]);
            }else{
                errString = QString(tr("Register need information [%1]").arg(text));
                break;
            }
        }
        /*如果注册信息不完备，则说明失败了*/
        if(i<count){
            //出错
            response.clear();
        }else{
            response += QString("</query></iq>");
        }
        break;
    case REGISTERSUCCESS:
        response = QString("</stream:stream>");
        emit registerSuccess();
        break;
    case REGISTERERROR:
        response = QString("</stream:stream>");
        emit registerError();
        break;
    default:
        break;
    }
    return response;
}

QMap<QString, QString> RegisterManager::getAttrList(QTreeWidgetItem *item)
{
    QMap<QString,QString> result;
    QTreeWidgetItem *root = item;
    QString attr = root->data(0, Qt::UserRole).toString();
    QStringList list = attr.split(";");
    foreach(QString pair, list)
    {
        QStringList attrPair = pair.split("=");
        if(attrPair.length() == 2)
        {
            result[attrPair.at(0)] = attrPair.at(1);
        }
    }

    return result;
}
