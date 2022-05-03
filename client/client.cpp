#include <iostream>
#include <boost/asio.hpp>

#include "../Tron.hpp"

// parsing arguments from cmd
std::string parse_raw_ip_address(int argc, char* argv[])
{
	if (argc == 2)
	{
		return std::string(argv[1]);
	}
	if (argc > 2)
	{
		std::cerr << "Too many parameters. Use: client <ip address>" << std::endl;
		exit(EXIT_FAILURE);
	}
	return "127.0.0.1";
}

int main(int argc, char* argv[])
{
	system("chcp 1251");

	std::cout << "Client launched" << std::endl;
	    
	Session().start_client(parse_raw_ip_address(argc, argv));

	return EXIT_SUCCESS;
}