#pragma once
#include "precompiled.h"
#include "Request.h"

class RequestHandler
{
	public:
		void handleRequest(Request request);

	private:
		bool processSystemCommand(Request& request) const;
		bool processCecCommand(Request& request) const;
		bool processMenuCommand(Request& request) const;

		static bool isCommand(Request& request, std::vector<std::string> testCommand);
};
