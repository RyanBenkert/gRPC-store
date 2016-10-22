#include "threadpool.h"
#include <string>
#include <vector>
#include <iostream>
#include <grpc++/grpc++.h>

using grpc::Server;
using grpc::ServerBuilder;

class Store {
public:
	void RunServer(std::string portNumber) {
		ServerBuilder builder;
		std::string server_address("0.0.0.0:" + portNumber);
		builder.AddListeningPort(server_address,
					 grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		std::cout << "Starting to run under " << server_address << "\n";
	}
private:
};

std::vector<std::string> getVendorAddresses(std::string filename) {
	return {"haha", "hehe"};

}

int main(int argc, char** argv) {
	std::vector<std::string> vendorAddresses;
	int nrOfThreads;
        std::string portNumber;
	if (argc == 4) {
		getVendorAddresses(std::string(argv[1]));
                nrOfThreads = atoi(argv[2]);
		portNumber = argv[3];
	} else {
		std::cerr << "Usage: ./store vendorAddresses nrOfThreads portNumber\n" << std::endl;
		return EXIT_FAILURE;
	}
	Store store;
	store.RunServer(portNumber);
}
