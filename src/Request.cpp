#include "Request.h"

Request::Request(QTcpSocket* client, QString escapedCommandLine) : client_{client}
{
	for (auto escaped : escapedCommandLine.split(' ', QString::SkipEmptyParts))
		command_.push_back(unescape(escaped.toStdString()));
}

void Request::respondMultiLine(std::vector<std::vector<std::string>> multiLineResponse)
{
	if (client_)
	{
		QTextStream stream{client_};

		int remainingLines = multiLineResponse.size() - 1;
		for (auto line : multiLineResponse)
		{
			QString lineToSend = QString::number(remainingLines);

			for (auto word : line)
			{
				lineToSend += ' ';
				lineToSend.append(QString::fromStdString(escape(word)));
			}

			if (client_->state() == QAbstractSocket::ConnectedState) stream << lineToSend << '\n';
			--remainingLines;
		}
	}
}

void Request::clientDisconnected()
{
	client_ = nullptr;
}

std::string Request::escape(std::string unescaped)
{
	std::string result;
	for (auto c : unescaped)
	{
		switch (c)
		{
			case '\n': result += "\\n"; break;
			case '\r': result += "\\r"; break;
			case ' ' : result += "\\s"; break;
			case '\\': result += "\\\\"; break;
			default: result += c; break;
		}
	}

	return result;
}

std::string Request::clientIpAddress() const
{
	if (!client_) return "null client";
	return client_->peerAddress().toString().toStdString();
}

std::string Request::unescape(std::string escaped)
{
	std::string result;

	bool isEscaped = false;
	for (auto c : escaped) {
		if (isEscaped)
		{
			switch (c)
			{
				case 'n': result += '\n'; break;
				case 'r': result += '\r'; break;
				case 's': result += ' '; break;
				default: result += c; break;
			}
			isEscaped = false;
		}
		else {
			if (c == '\\') isEscaped = true;
			else result += c;
		}
	}

	return result;
}
