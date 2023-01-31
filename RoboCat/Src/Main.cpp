
#include "RoboCatPCH.h"

#include <thread>
#include <iostream>
#include <string>
#include <sstream>

#if _WIN32

int DoTCPServer();
int DoTCPClient();

int main(int argc, const char** argv)
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#else
const char** __argv;
int __argc;
int main(int argc, const char** argv)
{
	__argc = argc;
	__argv = argv;
#endif

	SocketUtil::StaticInit();

	char serverChar;
	std::cout << "Enter 's' for server or 'c' for client: ";
	std::cout.flush();
	std::cin >> serverChar;
	bool isServer = serverChar == 's';

	int result = 0;
	if (isServer)
	{
		result = DoTCPServer();
	}
	else
	{
		result = DoTCPClient();
	}

	SocketUtil::CleanUp();

	return result;
}

int DoTCPClient() 
{
	TCPSocketPtr clientSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	if (clientSocket == nullptr) return 1;

	clientSocket->SetNonBlockingMode(true);
	std::cout << "Created Client Socket\n";

	SocketAddressPtr clientAddress = SocketAddressFactory::CreateIPv4FromString("127.0.0.1");
	if (clientAddress == nullptr) return 1;
	
	int result = 0;
	result = clientSocket->Bind(*clientAddress);
	if (result != 0) return 1;

	std::cout << "Bound Client Socket\n";

	SocketAddressPtr serverAddress = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:8080");

	clientSocket->SetNonBlockingMode(false);
	result = clientSocket->Connect(*serverAddress);
	if (result != 0) return 1;
	clientSocket->SetNonBlockingMode(true);

	std::cout << "Connected to Server\n";

	while (result < 0)
	{
		if (result != -WSAEWOULDBLOCK)
		{
			SocketUtil::ReportError("Client Connect");
			return 1;
		}

		result = clientSocket->Connect(*serverAddress);
	}

	std::string message("Hello Server!");
	clientSocket->Send(message.c_str(), message.length());

	return 0;
}

int DoTCPServer() 
{
	TCPSocketPtr listenSocket = SocketUtil::CreateTCPSocket(SocketAddressFamily::INET);
	if (listenSocket == nullptr) return 1;

	listenSocket->SetNonBlockingMode(true);

	std::cout << "Created ListenSocket" << std::endl;

	SocketAddressPtr address = SocketAddressFactory::CreateIPv4FromString("127.0.0.1:8080");
	if (address == nullptr) return 1;

	std::cout << "Created Address: " << address->ToString() << std::endl;

	int result = listenSocket->Bind(*address);
	if (result != 0) return 1;

	std::cout << "Bound Socket\n";

	if (listenSocket->Listen() != NO_ERROR) return 1;

	std::cout << "Listening on Socket\n";

	SocketAddress incomingAddress;
	int error = 0;
	TCPSocketPtr connectionSocket = listenSocket->Accept(incomingAddress, error);
	while (connectionSocket == nullptr)
	{
		if (error != WSAEWOULDBLOCK)
		{
			SocketUtil::ReportError("Accepting Connection");
			return 1;
		}
		else
		{
			//std::cout << "Failed to accept because socket would be blocked\n";
			connectionSocket = listenSocket->Accept(incomingAddress, error);
		}
	}

	connectionSocket->SetNonBlockingMode(true);

	std::cout << "Accepted connection from: " << incomingAddress.ToString() << std::endl;

	const size_t BUFFSIZE = 4096;
	char buffer[BUFFSIZE];

	//listenSocket->Receive();
	// OR
	int32_t len = connectionSocket->Receive(buffer, BUFFSIZE);
	while (len < 0)
	{
		if (len != -WSAEWOULDBLOCK)
		{
			SocketUtil::ReportError("Receiving Data");
			return 1;
		}

		len = connectionSocket->Receive(buffer, BUFFSIZE);
	}
	//std::cout << "This is a c string \
	//				a null terminator is automatically appended \
	//				by the compiler" << std::endl;


	////but -- our buffer doesnt have a null terminator!

	////So, we convert it to a string with length = len
	//std::string message(buffer, len);
	//std::cout << message << std::endl;

	OutputWindow win;

	std::thread t([&win]()
		{
			int msgNo = 1;
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		std::string msgIn("~~~auto message~~~");
		std::stringstream ss(msgIn);
		ss << msgNo;
		win.Write(ss.str());
		msgNo++;
	}
		});

	while (true)
	{
		std::string input;
		std::getline(std::cin, input);
		win.WriteFromStdin(input);
	}
}
