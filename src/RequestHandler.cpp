#include "RequestHandler.h"
#include "CecConnector.h"

inline bool RequestHandler::isCommand(Request& request, std::vector<std::string> testCommand)
{
	// This indirection is necessary to enable the use of vector literals (initializer lists of string elements)
	return request.command() == testCommand;
}

void RequestHandler::handleRequest(Request request)
{
	std::cout << request.clientIpAddress() << ": ";
	for (auto s : request.command())
		std::cout << s << ' ';
	std::cout << std::endl;

	if (request.command().empty()) return;

	if (processSystemCommand(request)) return;

	std::string subsystem = request.command().front();

	if (subsystem == "cec" && processCecCommand(request)) return;
	if (subsystem == "menu" && processMenuCommand(request)) return;

	request.respondWord("Invalid command");
}

bool RequestHandler::processSystemCommand(Request& request) const
{
	if (isCommand(request, {"quit"}))
	{
		QCoreApplication::exit(0);
		return true;
	}

	if (isCommand(request, {"ping"}))
	{
		request.respondWord("pong");
		return true;
	}

	return false;
}

bool RequestHandler::processCecCommand(Request& request) const
{
	if (isCommand(request, {"cec", "tv", "on"}))
	{
		CecConnector::instance().turnOn(CEC::cec_logical_address::CECDEVICE_TV);
		return true;
	}

	if (isCommand(request, {"cec", "tv", "off"}))
	{
		CecConnector::instance().turnOff(CEC::cec_logical_address::CECDEVICE_TV);
		return true;
	}

	if (isCommand(request, {"cec", "volume", "up"}))
	{
		CecConnector::instance().volumeUp();
		return true;
	}

	if (isCommand(request, {"cec", "volume", "down"}))
	{
		CecConnector::instance().volumeDown();
		return true;
	}

	if (isCommand(request, {"cec", "volume", "mute"}))
	{
		CecConnector::instance().toggleMute();
		return true;
	}

	if (isCommand(request, {"cec", "scan"}))
	{
		auto deviceNames = CecConnector::instance().scanForDevices();
		request.respondLine(deviceNames);
		return true;
	}

	if (request.command().size() == 3 && request.command().front() == "cec" && request.command()[1] == "activate")
	{
		CecConnector::instance().activateDevice(request.command().back());
		return true;
	}

	return false;
}


bool RequestHandler::processMenuCommand(Request& request) const
{
	if (isCommand(request, {"menu", "tabs"}))
	{
		request.respondMultiLine({{"Movies"}, {"TV"}, {"Series"}});
		return true;
	}

	if (isCommand(request, {"menu", "tab", "Movies"}))
	{
		request.respondMultiLine({{"The Matrix"}, {"Terminator"}, {"Star Wars: The Emptire Strikes Back"}});
		return true;
	}

	if (isCommand(request, {"menu", "tab", "TV"}))
	{
		request.respondMultiLine({{"SRF 1"}, {"BBC"}, {"Channel 4"}});
		return true;
	}

	if (isCommand(request, {"menu", "tab", "Series"}))
	{
		request.respondMultiLine({{"The Big Bang Theory S2 E14"}, {"Nashville S5 E3"}});
		return true;
	}

	return false;
}
