#include "precompiled.h"

#include "CecConnector.h"
#include "Server.h"
#include "RequestHandler.h"

void exitSignalHandler(int signal)
{
	std::cout << "Caught signal " << signal << std::endl << "Exiting" << std::endl;
	QCoreApplication::exit(0);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

	 if (signal(SIGINT, exitSignalHandler) == SIG_ERR)
	 {
		 std::cerr << "Could not register signal handler" << std::endl;
		 return 1;
	 }

	 // Create the instance and connect to CEC
	 //CecConnector::instance();

	 Server server;
	 RequestHandler handler;

	 server.addRequestProcessor([&handler](Request r){ handler.handleRequest(r); });

	 return a.exec();
}
