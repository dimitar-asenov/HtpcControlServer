#include "Server.h"

Server::Server()
{
	if (!tcpServer_.listen(QHostAddress::Any, port_))
		throw std::logic_error(QString{"Could not listen on TCP server port %1: %2"}
									  .arg(port_).arg(tcpServer_.errorString()).toStdString());

	QObject::connect(&tcpServer_, &QTcpServer::newConnection, [this](){newConnection();});
}

void Server::newConnection()
{
	auto client = tcpServer_.nextPendingConnection();
	QObject::connect(client, &QAbstractSocket::disconnected, client, &QObject::deleteLater);

	QTextStream{client} << "TvControlServer\n";

	QObject::connect(client, &QAbstractSocket::readyRead, [this, client](){onReceivedDataFromClient(client);});
}

void Server::onReceivedDataFromClient(QTcpSocket* client)
{
	if (client->canReadLine())
	{
		QTextStream stream{client};
		auto line = stream.readLine();
		while (!line.isNull())
		{
			Request request{client, line};
			QObject::connect(client, &QAbstractSocket::disconnected, [&request](){request.clientDisconnected();});

			for (auto& processor : requestProcessors_)
				processor(request);

			if (client->canReadLine()) line = stream.readLine();
			else break;
		}
	}
}

