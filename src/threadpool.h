#pragma once
#include <thread>
#include <queue>
#include <future>
#include <condition_variable>
#include <grpc++/grpc++.h>
#include "vendor.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using vendor::Vendor;
using vendor::BidQuery;
using vendor::BidReply;

struct VendorBid {
  double price;
  std::string vendor_id;
};

class VendorClient {
public:
  explicit VendorClient(std::shared_ptr<Channel> channel)
      : stub_(Vendor::NewStub(channel)) {}

  void AsyncAskBid(const std::string& product_name);

  vendor::BidReply AsyncCompleteRpc();

private:
  // struct for keeping state and data information
  struct AsyncClientCall {
    // Container for the data we expect from the server.
    BidReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // Storage for the status of the RPC upon completion.
    Status status;

    std::unique_ptr<grpc::ClientAsyncResponseReader<BidReply>> response_reader;
  };

  std::unique_ptr<Vendor::Stub> stub_;
  grpc::CompletionQueue cq_;
};

class ThreadPool {
public:
        // Constructor that takes in the number of threads and a file location
	// to retrieve the vendor addresses
	ThreadPool(int, std::vector<std::string>);
        // Destructor which simply joins the threads to clean up the memory
	// footprint of the pool
	~ThreadPool();
	// Method to submit work to the thread pool. The pool decides which
	// worker will use it
	int enqueue(std::function<void()>);
	enum Status { AVAILABLE, WORKING, WAITING };
	template<class F, class... Args>
	auto addTask(F&& f, Args&&... args)
	-> std::future<typename std::result_of<F(Args...)>::type>;

	std::future<std::vector<VendorBid>> appendQuery(const std::string& query);
private:
	void run();
	std::vector<VendorBid> AskBid(const std::string& query);
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
	std::condition_variable condition;
	std::mutex queueMutex;
};

