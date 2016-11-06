#include "threadpool.h"
#include <fstream>
#include <iostream>

ThreadPool::ThreadPool(std::string addressesLocation, int nrOfThreads) {
	for(std::string address : getAddresses(addressesLocation)) {
		std::cout << "Vendor addresses: " << address << std::endl;
	}
}

std::vector<std::string> ThreadPool::getAddresses(std::string addressesLocation) {
	std::vector<std::string> ipAddresses;
	std::ifstream vendorsFile (addressesLocation);
	int addrIndex = -1;
	if (vendorsFile.is_open()) {
		std::string ipAddr;
		while (getline(vendorsFile, ipAddr)) {
			if (addrIndex == -1) {
				ipAddresses.push_back(ipAddr);
			} else if (addrIndex == 0) {
				ipAddresses.push_back(ipAddr);
				break;
			} else {
				--addrIndex;
			}
		}
		vendorsFile.close();
		return ipAddresses;
	} else {
		std::cerr << "Failed to open file " << addressesLocation << std::endl;
		return {};
	}
}
