#pragma once
#include "precompiled.h"

class Request
{
	public:
		Request(const Request& other) = default;
		std::vector<std::string> command() const;

		std::string clientIpAddress() const;

		void respondWord(std::string oneWordResponse);
		void respondLine(std::vector<std::string> singleLineResponse);
		void respondMultiLine(std::vector<std::vector<std::string>> multiLineResponse);

	private:
		QTcpSocket* client_;
		std::vector<std::string> command_;

		friend class Server;

		Request(QTcpSocket* client, QString escapedCommandLine);

		static std::string escape(std::string unescaped);
		static std::string unescape(std::string escaped);

		void clientDisconnected();
};

inline std::vector<std::string> Request::command() const { return command_; }
inline void Request::respondWord(std::string oneWordResponse)
{
	respondMultiLine({{oneWordResponse}});
}

inline void Request::respondLine(std::vector<std::string> singleLineResponse)
{
	respondMultiLine({singleLineResponse});
}
