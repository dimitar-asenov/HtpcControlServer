#pragma once
#include "precompiled.h"
#include "Request.h"

class Server
{
	public:
		Server();

		using RequestProcessor = std::function<void (Request)>;
		void addRequestProcessor(RequestProcessor p);

	private:
		QTcpServer tcpServer_;
		quint16 port_{21369};

		std::vector<RequestProcessor> requestProcessors_;

		void newConnection();
		void onReceivedDataFromClient(QTcpSocket* client);
};

inline void Server::addRequestProcessor(RequestProcessor p) { requestProcessors_.push_back(p); }
