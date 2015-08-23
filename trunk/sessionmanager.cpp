#include <QtNetwork/QTcpSocket>
#include <QTreeWidget>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <iostream>
#include "sessionmanager.h"
#include "saxhandler.h"
#include "md5.h"
#include "util.h"

SessionManager::SessionManager(QWidget *parent) :
    QDialog(parent)
{
    needBind = false;
    needSess = false;
    firstTimeShowMainWindow = true;
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

    sessionId = random()%50;
}

SessionManager *SessionManager::sm = NULL;
SessionManager* SessionManager::getSessionManager()
{
    /*如果是第一次使用sm*/
    if(!sm)
    {
        sm = new SessionManager;
    }

    return sm;
}

QString SessionManager::getError()
{
    return errString;
}

void SessionManager::connectionClosedByServer()
{
    std::cerr << "Connection closed by the server.\n";
    tcpSocket->close();
}

void SessionManager::socketError()
{
    std::cerr << "Error: "
            << tcpSocket->errorString().toStdString()
            <<"\n";
    tcpSocket->close();
}

/*向socket里面发送数据*/
void SessionManager::sendRequest(QString request)
{
    tcpSocket->write(request.toStdString().c_str());
    std::cerr << "<!-- OUT -->\n"<< request.toStdString() << "\n\n";
}

/*设置登录用户的身份信息*/
bool SessionManager::setUserInfo(QString node, QString domain,
                                 QString resource, QString password)
{
    if(0==node.length() || 0==domain.length()
        || 0==resource.length() || 0==password.length()){
        return false;
    }

    user["node"] = node;
    user["domain"] = domain;
    user["resource"] = resource;
    user["password"] = password;
    return true;
}

bool SessionManager::start()
{
    status = NONE;

    /*第一步:连接到服务器*/
    tcpSocket->connectToHost(user["domain"], defaultPort);
    if(!tcpSocket->waitForConnected(5000)){
        errString = tcpSocket->errorString();
        return false;
    }

    /*第二步，发送初始化数据*/
    /*连接已建立，可以发送第一个请求*/
    outData = QString("<?xml version='1.0'?>"
                     "<stream:stream xmlns='jabber:client'"
                     " to='%1' version='1.0'"
                     " xmlns:stream='http://etherx.jabber.org/streams'"
                     " xml:lang='zh'>").arg(user["domain"]);
    emit send(outData);

    return true;
}


void SessionManager::handleRead()
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
    else
        data = QString("<stream:stream>")+data+QString("</stream:stream>");

    /*解析xml流,生成xml树*/
    xmlTree->clear();
    QXmlInputSource inputSource;
    inputSource.setData(data);
    QXmlSimpleReader reader;
    reader.setContentHandler(xmlHandler);
    reader.setErrorHandler(xmlHandler);
    reader.parse(inputSource);

    /*解析xmlTree,生成状态信息*/
    if(!parseTree())
    {
        QMessageBox::warning(0, tr("Error"),
                             tr("Error: %1\n").arg(getError()));
        return;
    }

    /*根据解析生成的状态信息，生成回复*/
    outData = generateResponse();

    if(status >= PRESENCEUPDATED)
        return;

    if(outData.isEmpty())
    {
        QMessageBox::warning(0, tr("Error"),
                             tr("Error: %1\n").arg(getError()));
    }else{
        emit send(outData);
    }
}

bool SessionManager::parseTree()
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
            if(!parseFeaturesItem(item))
                return false;
        }else if(item->text(0)=="challenge" || item->text(0)=="success"
                 || item->text(0)=="failure"){
            if(!parseSaslItem(item))
                return false;
        }else if(item->text(0)=="iq"){
            if(!parseIqItem(item))
                return false;
        }else if(item->text(0) == "presence"){
            if(!parsePresenceItem(item))
                return false;
        }else if(item->text(0) == "message"){
            if(!parseMessageItem(item))
                return false;
        }
    }
    return true;
}

bool SessionManager::parseFeaturesItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *feasItem = item;
    int childCount = feasItem->childCount();
    for(int i=0; i<childCount; i++){
        QTreeWidgetItem *child = feasItem->child(i);

        /*如果特性下面跟随的是机制<machanisms />，作如下处理*/
        if(child->text(0)=="mechanisms")
        {
            for(int i=0;i<child->childCount();i++)
            {
                QTreeWidgetItem *c = child->child(i);
                /*如果此孩子文本内容为空，说明孩子节点遍历完毕*/
                if(c->text(0).isEmpty())
                    break;

                /*如果孩子节点为机制*/
                if(c->text(0) == "mechanism")
                {
                    saslMechs.append(c->text(1));

                }//end if(c->text(0) == "mechanism")

            }// end for--结束machanism的处理

            status = INITIALIZED;
        }//结束machanisms的处理

        if(child->text(0) == "bind"){
            needBind = true;
            status = REINIT;
        }
        if(child->text(0) == "session"){
            needSess = true;
            status = REINIT;
        }
    }
    return true;
}

bool SessionManager::parseSaslItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *root = item;

    if(root->text(0)=="failure"){
        //如果认证失败，重新开始认证
        errString = QString("SASL authentication failed.\n");
        status = INITIALIZED;
    }else if(root->text(0)=="challenge"){
        QString base64str = root->text(1);
        QByteArray text(base64str.toStdString().c_str());
        QString utf8str = text.fromBase64(text);

        if(INITIALIZED == status)/*第一次收到challenge信息*/
        {
            fillChallengeInfo(utf8str);
            status = CHALLENGE;
        }else if(CHALLENGE == status) /*第二次收到response信息*/
        {
            /*期望收到rspauth信息*/
            QStringList list = utf8str.split("=");
            if(list.at(0).trimmed()=="rspauth") //这个就是预期收到的信息
            {
                status = CHALLENGE2;
            }else{
                errString = QString("Unknow error with SASL challenge 2.");
                return false;
            }
        }
    }else if(root->text(0)=="success"){
        status = SASLSUCC;
    }
    return true;
}

bool SessionManager::parseIqItem(QTreeWidgetItem *item)
{
    QTreeWidgetItem *root = item;
    int childCount = root->childCount();
    QMap<QString, QString> attrs = getAttrList(root);
    QString type = attrs["type"];

    /*如果childCount为0，恭喜，绑定完成，会话已经建立*/
    if(childCount == 0 && type == "result")
    {
        if(status == REINIT)
            status = SESSESTAB;
        else if(status == ADDITEM)
            status = ADDITEMSUCCESS;
        else if(status == REMOVEITEM)
            status = REMOVEITEMSUCCESS;
        return true;
    }

    for(int i=0; i<childCount; i++)
    {
        QTreeWidgetItem *child = root->child(i);
        /*如果iq下面跟随的是bind*/
        if(child->text(0)=="bind")
        {
            QTreeWidgetItem *secondLevel = child->child(0);
            /*如果bind的孩子为jid信息，说明绑定成功了*/
            if(secondLevel->text(0) == "jid")
            {
                /*成功收到了bind和jid信息，说明绑定成功了*/
                fullJid = secondLevel->text(1);
                needBind = false;
            }
        }// end if(child->text(0)=="bind")
        else if(child->text(0) == "error")
        {
            errString = child->child(0)->text(0)+QString("--")+child->child(0)->text(1);
            return false;
        }else if(child->text(0) == "query")
        {
            return parseQueryItem(child, type);
        }
    }// end for(int i=0; i<childCount; i++)
    return true;
}

/*处理query节*/
bool SessionManager::parseQueryItem(QTreeWidgetItem *item, QString type)
{
    QMap<QString, QString> attrs = getAttrList(item);

    /*如果是名册管理信息*/
    if(type == "result"){
        friendsList.clear();
        int itemCount = item->childCount();
        for(int j=0; j<itemCount; j++)
        {
            QTreeWidgetItem *itemItem = item->child(j);
            QMap<QString, QString> friends = getAttrList(itemItem);
            QString subscription = friends["subscription"];
            QString groupName;

            if(itemItem->childCount() >= 0)
                groupName = itemItem->child(0)->text(1);
            else
                groupName = QString("friends");
            friends["group"] = groupName;
            friends["presence"] = QString("unavailable");

            friendsList.append(friends);

        }/*end for(int j=0; j<itemCount; j++) 结束item项处理*/
        status = FRIENDLISTUPDATED;

    }else if(type == "set"){
        QTreeWidgetItem *setItem = item->child(0);
        QMap<QString, QString> friends = getAttrList(setItem);
        QString subscription = friends["subscription"];
        QString ask = friends["ask"];
        if(ask == "subscribe")
        {
            /**
             *ask数据包发生在自己请求加对方好友的过程中，
             *此时若同意，则对方能够看到自己的在线状态，subscription='from'
             */
            status = SUBSCRIBEASK;
            subscribeAskJids.append(friends["jid"]);
            return true;
        }

        if(subscription == "remove"){
            QString jid = friends["jid"];
            /*从列表中删除*/
            for(int k=0; k<friendsList.count(); k++)
            {
                QMap<QString, QString> info = friendsList[k];
                if(info["jid"] == jid)
                {
                    friendsList.removeAt(k);
                    break;
                 }
            }// end for(int k=0; k<friendsList.count(); k++)
            status = REMOVEITEM;
        }else if(subscription=="none" || subscription == "both"){
            QString groupName = setItem->child(0)->text(1);
            friends["group"] = groupName;
            friends["presence"] = QString("unavailable");
            int k;

            /*检查是否是重复数据，如果是，删掉旧信息*/
            for(k=0; k<friendsList.count(); k++)
            {
                QMap<QString, QString> info = friendsList[k];
                if(info["jid"] == friends["jid"])
                {
                    friends["presence"] = info["presence"];
                    friendsList.removeAt(k);
                    break;
                }
            }//end for(k=0; k<friendsList.count(); k++)

            this->currentAddedJid = friends["jid"];
           friendsList.append(friends);
           status = ADDITEM;

       }//end else if(subcription == "none" || subcription == "both")
    }// end else if(type == set)
    return true;
}

bool SessionManager::parsePresenceItem(QTreeWidgetItem *item)
{
    QMap<QString, QString> friendPre = getAttrList(item);
    QString jid = friendPre["from"].split("/").at(0);
    QString type = friendPre["type"];


    if(type == "subscribe")
    {
        /*presence中type=subscribe，则此数据为对方向自己请求订阅
         用户可选择同意或不同意*/
        status = SUBSCRIBE;
        subscribeJids.append(jid);
    }else{
        status = PRESENCEUPDATED;

        int listCount = friendsList.count();
        for(int i=0; i<listCount; i++)
        {
            QMap<QString, QString> fri = friendsList.at(i);
            if(fri["jid"] == jid)
            {
                if(type.isEmpty())
                    friendsList[i]["presence"] = QString("presence");
                else
                    friendsList[i]["presence"] = type;
            }
        }
    }

    return true;
}

bool SessionManager::parseMessageItem(QTreeWidgetItem *item)
{
    QMap<QString, QString> info = getAttrList(item);

    /*jid(node@domain/resource)*/
    QString jid = info["from"];
    QString message;

    for(int k=0; k<item->childCount(); k++)
    {
        if(item->child(k)->text(0) == "body")
        {
            message.append(item->child(k)->text(1));
            status = MESSAGE;
        }//end if
    }//end for

    emit receiveMessage(jid, message);
    return true;
}

QMap<QString, QString> SessionManager::getAttrList(QTreeWidgetItem *item)
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

QString SessionManager::generateResponse()
{
    outData.clear();
    //errString.clear();
    QString response;
    switch(status){
    case NONE:
    case NEEDMOREDATA:
        break;
    case INITIALIZED:
        {
        /*如果流刚刚被初始化,查看是否需要SASL认证
          如果支持的机制为空，则无法完成，出错*/
        static int retryTime = 0;
        if(retryTime++ >= 3)
        {
            //重新认证次数为3次，如果超过3次，则认为认证失败
            errString = QString("SASL auth times more than 3,SASL failed.");
            emit send(QString("</stream:stream>"));
            break;
        }
        if(saslMechs.isEmpty())
        {
            errString = QString("SASL mechanisms support null!");
            break;
        }

        /*优先支持md5*/
        if(saslMechs.contains("DIGEST-MD5"))
        {
            response = QString("<auth xmlns='urn:ietf:params:xml:ns:xmpp-sasl'"
                               " mechanism='DIGEST-MD5' />");
        }else{
            /*如果是plain机制，客户端暂时不支持*/
            errString = QString("Client has no supported SASL mechanism.");
        }
        break;
    }
    case CHALLENGE:
        response = getSaslResponse();
        break;

    case CHALLENGE2:
        response = QString("<response xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\" />");
        break;

    case SASLSUCC:
        response = QString("<?xml version='1.0'?><stream:stream"
                           " xmlns=\"jabber:client\" to=\"%1\" version=\"1.0\""
                           " xmlns:stream=\"http://etherx.jabber.org/streams\""
                           " xml:lang=\"zh\" >").arg(user["domain"]);
        break;

    case REINIT:
        if(needBind)
        {
            response = QString("<iq type=\"set\" id=\"%1\">"
                               "<bind xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\">"
                               "<resource>%2</resource></bind></iq>")
                               .arg(sessionId++).arg(user["resource"]);
        }else if(needSess){
            response = QString("<iq type=\"set\" id=\"%1\">"
                               "<session xmlns=\"urn:ietf:params:xml:ns:xmpp-session\" />"
                               "</iq>").arg(sessionId++);
        }
        break;
    case SESSESTAB:
        /*会话建立后的第一件事情就是获取好友名单*/
        response = QString("<iq from='%1' type='get' "
                           "id='roster_%2'><query xmlns='jabber:iq:roster'/>"
                           "</iq>").arg(this->fullJid).arg(sessionId++);
        break;
    case FRIENDLISTUPDATED:
        response = QString("<presence />");
        if(firstTimeShowMainWindow){
            emit showMainWindow();
            firstTimeShowMainWindow = false;
        }else{
            emit updateRosters();
        }
        break;
    case PRESENCEUPDATED:
        emit updatePresence();
        break;
    case MESSAGE:
        break;
    case ADDITEM:
        emit updateRosters();
        break;
    case REMOVEITEM:
        emit updateRosters();
        break;
    case ADDITEMSUCCESS:
        emit updateRosters();
        break;
    case REMOVEITEMSUCCESS:
        emit updateRosters();
        break;
    case SUBSCRIBEASK:
        emit subscribeAsk(subscribeAskJids);
        subscribeAskJids.clear();
        break;
    case SUBSCRIBE:
        emit subscribeFrom(subscribeJids);
        //emit updatePresence();
        subscribeJids.clear();
        break;
    default:
        break;
    }
    return response;
}

bool SessionManager::fillChallengeInfo(QString challenge)
{
    QStringList list = challenge.split(",");
    foreach(QString option, list)
    {
        QStringList tmp = option.split("=");
        if(tmp.length() != 2)
            return false;
        QString value = (QString)tmp.at(1);
        value = value.replace(QString("\""), QString("")).trimmed();
        QString name = tmp.at(0);
        if(name == "realm")
            challengeMap["realm"] = value;
        else if(name == "nonce")
            challengeMap["nonce"] = value;
        else if(name == "qop")
            challengeMap["qopOptions"] = value;
        else if(name == "stale")
            challengeMap["stale"] = value;
        else if(name == "maxbuf")
            challengeMap["maxbuf"] = value;
        else if(name == "charset")
            challengeMap["charset"] = value;
        else if(name == "algorithm")
            challengeMap["algorithm"] = value;
        else if(name == "cipher")
            challengeMap["cipherOpts"] = value;
        else if(name == "token")
            challengeMap["authParam"] = value;
    }
}

QString SessionManager::getSaslResponse()
{
    responseMap["charset"] = "utf-8";
    responseMap["username"] = user["node"];
    responseMap["realm"] = QString(challengeMap["realm"]).isEmpty()? QString(user["domain"]):challengeMap["realm"];
    responseMap["nonce"] = challengeMap["nonce"];
    responseMap["nonceCount"] = "00000001";
    responseMap["digestUri"] = QString("xmpp/")+responseMap["realm"];
    responseMap["qop"]="auth";

    QString response;
    response.append("charset="); response.append(responseMap["charset"]);
    response.append(",username=\""); response.append(responseMap["username"]);
    response.append("\",realm=\""); response.append(responseMap["realm"]);
    response.append("\",nonce=\""); response.append(responseMap["nonce"]);
    response.append("\",nc="); response.append(responseMap["nonceCount"]);
    response.append(",digest-uri=\""); response.append(responseMap["digestUri"]);
    response.append("\",qop="); response.append(responseMap["qop"]);

    const char *num = "0123456789abcdef";
    srand(time(0));
    QString cnonceTmp;
    for(int i=0; i<49; i++)
    {
        int index = rand()%16;
        cnonceTmp.append((char)num[index]);
    }
    responseMap["cnonce"] = cnonceTmp;
    response.append(",cnonce=\""); response.append(responseMap["cnonce"]);

    /*下面开始最重要的任务，计算response的值*/
    responseMap["response"] = calculateRsp();
    response.append("\",response="); response.append(responseMap["response"]);

    QByteArray array(response.toStdString().c_str());
    response = QString("<response xmlns=\"urn:ietf:params:xml:ns:xmpp-sasl\">")
               +QString(array.toBase64()) + QString("</response>");
    return response;
}

/*计算response值*/
QString SessionManager::calculateRsp()
{
    char *userName =(char*)malloc(QString(responseMap["username"]).length()+1);
    strcpy(userName, QString(responseMap["username"]).toStdString().c_str());

    char *realm = (char*)malloc(QString(responseMap["realm"]).length()+1);
    strcpy(realm,QString(responseMap["realm"]).toStdString().c_str());

    char *passwd = (char*)malloc(user["password"].length()+1);
    strcpy(passwd, user["password"].toStdString().c_str());

    char *nonceValue = (char*)malloc(QString(responseMap["nonce"]).length()+1);
    strcpy(nonceValue, QString(responseMap["nonce"]).toStdString().c_str());

    char *cnonceValue = (char*)malloc(QString(responseMap["cnonce"]).length()+1);
    strcpy(cnonceValue, QString(responseMap["cnonce"]).toStdString().c_str());

    char *ncValue =(char*)malloc(QString(responseMap["nonceCount"]).length()+1);
    strcpy(ncValue, QString(responseMap["nonceCount"]).toStdString().c_str());

    char *qopValue = (char*)malloc(QString(responseMap["qop"]).length()+1);
    strcpy(qopValue,QString(responseMap["qop"]).toStdString().c_str());

    char *digestUri = (char*)malloc(QString(responseMap["digestUri"]).length()+1);
    strcpy(digestUri, QString(responseMap["digestUri"]).toStdString().c_str());

    unsigned char digest[16];
    MD5_CTX *ctx = new MD5_CTX;

    QString a = QString(userName)+QString(":")+QString(realm)+QString(":")+QString(passwd);
    MD5Init(ctx);
    MD5Update(ctx, (const md5byte*)a.toStdString().c_str(), a.length());
    MD5Final(ctx,digest);

    //QString b = translate(a.toStdString().c_str(), a.length());
    QString A1 = QString((char*)&digest[0])+QString(":")+QString(nonceValue)+QString(":")+QString(cnonceValue);
    QString A2 = QString("AUTHENTICATE:")+QString(digestUri);

    MD5Init(ctx);
    MD5Update(ctx, (const md5byte*)A1.toStdString().c_str(), A1.length());
    MD5Final(ctx,digest);
    QString HEXHA1 = translate((const char*)&digest[0], 16);

    MD5Init(ctx);
    MD5Update(ctx, (const md5byte*)A2.toStdString().c_str(), A2.length());
    MD5Final(ctx,digest);
    QString HEXHA2 = translate((const char*)&digest[0], 16);

    QString KDStr = HEXHA1+QString(":")+QString(nonceValue)+QString(":")
                 +QString(ncValue)+QString(":")+QString(cnonceValue)
                 +QString(":")+QString(qopValue)+QString(":")+HEXHA2;
    MD5Init(ctx);
    MD5Update(ctx, (const md5byte*)KDStr.toStdString().c_str(), KDStr.length());
    MD5Final(ctx,digest);
    QString HEXKD = translate((const char*)&digest[0], 16);

    free(userName); free(realm); free(passwd);
    free(nonceValue);free(cnonceValue);free(ncValue);
    free(qopValue);free(digestUri);
    delete(ctx);
    return HEXKD;
}

QString SessionManager::translate(const char *digest, int len)
{
    char *list = "0123456789abcdef";
    char *result = new char[2*len+1];
    memset(result, 0, 2*len+1);
    for(int i=0; i<len; i++)
    {
       int first = (digest[i]& 0XF0) >> 4;
       int second = digest[i]& 0X0F;
       result[2*i] = list[first];
       result[2*i+1] = list[second];
    }
    return QString((char*)&result[0]);
}
