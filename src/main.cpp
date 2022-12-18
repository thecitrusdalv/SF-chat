//main.cpp
#include <iostream>
#include <exception>

#include "../headers/client.h"
#include "../headers/server.h"

using namespace std;

int main()
{
	try {
		Server server;
		Client client;

		client.connect(&server);
	}

	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return -1;
	}

	catch (const char* message)
	{
		std::cerr << "\nCritical Error: " << message << std::endl;
		return -2;
	}

	catch (...)
	{
		std::cerr << "\nUndefined error" << std::endl;
		return -3;
	}

	return 0;
}
