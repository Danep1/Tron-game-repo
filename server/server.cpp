#include <iostream>
#include <boost/asio.hpp>

#include "../Tron.hpp"

int main(int argc, char* argv[])
{
	system("chcp 1251");

	std::cout << "Server is launching..." << std::endl;

    Session().launch_server();

	return EXIT_SUCCESS;
}