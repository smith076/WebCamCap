#include "localserver.h"

#include <QDataStream>

LocalServer::LocalServer(QString name, QObject *parent) : IServer(parent)
{
    setName(name);
    m_server = new QLocalServer();
    connect(m_server, &QLocalServer::newConnection, this, &IServer::newConnection);
}

LocalServer::~LocalServer()
{
    if(m_enabled)
    {
        setEnabled(false);
    }

    delete m_server;
    qDeleteAll(m_sockets);
}

bool LocalServer::setEnabled(bool enabled)
{
    if(enabled == m_enabled)
    {
        return false;
    }

    if(enabled)
    {
        m_server->removeServer(m_name);

        if(!m_server->listen(m_name))
        {
            qDebug() << "Server down somehow!";
            m_enabled = false;
            return false;
        }
        qDebug() << "Server with name: " << m_name << " created";
        m_enabled = true;
        return true;
    }
    else
    {
        m_server->close();

        foreach (QLocalSocket *socket, m_sockets)
        {
            if(socket)
            {
                socket->disconnectFromServer();
                socket->deleteLater();
            }
        }

        m_sockets.clear();

        qDebug() << "Server with name: " << m_name << " closed";
        m_enabled = false;
        return true;
    }
}

void LocalServer::sendMesage(QVariantMap message)
{
    foreach (QLocalSocket *socket, m_sockets)
    {
        if(socket != nullptr && socket->isOpen() && socket->isWritable())
        {
            QDataStream out(socket);
            out.setVersion(QDataStream::Qt_5_4);

            out << message;

            socket->flush();
        }
    }
}

const QString nameKey("name");
const QString enabledKey("enabled");

QVariantMap LocalServer::toVariantMap() const
{
    QVariantMap varMap;

    varMap[nameKey] = m_name;
    varMap[enabledKey] = m_enabled;

    return varMap;
}

void LocalServer::fromVariantMap(QVariantMap varMap)
{
    m_name = varMap[nameKey].toString();

    bool enabled = varMap[enabledKey].toBool();

    setEnabled(enabled);
}

void LocalServer::newConnection()
{
    auto socket = m_server->nextPendingConnection();

    if(socket != nullptr)
    {
        connect(socket, &QLocalSocket::disconnected, this, &IServer::socketDisconnected);
    }

    m_sockets.push_back(socket);
}

void LocalServer::socketDisconnected()
{
    QObject *snd = sender();

    if(snd)
    {
        m_sockets.removeOne(qobject_cast<QLocalSocket*>(snd));
        snd->deleteLater();
    }
}
