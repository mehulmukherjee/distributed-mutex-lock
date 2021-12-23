#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

//Number of nodes to participate in DML, total nodes=NODES+1
#define NODES 3
//buffer size for the hostname
#define HOSTNAME_LEN 32
//number of locks supported by this program
#define LOCK_COUNT 8

#define DATASEG_COUNT 3

#define PAGE_SIZE 4096

#define PAGE_COUNT 4


class Node {
public:
	std::string hostname;
	std::string ip_addr;

	//array of client node names	
	static char nodes[NODES+1][HOSTNAME_LEN];
	
	//constructor for Node class; sets hostname and ip_addr to local machine	
	Node() {
		//sets hostname and ip_addr to node's local machine
		char hostbuf[32];
		char *ipbuf;	
		gethostname(hostbuf, sizeof(hostbuf));
		hostname = std::string(hostbuf);	
		struct hostent *localhost = gethostbyname(hostbuf);
		ipbuf = inet_ntoa(*((struct in_addr*) localhost->h_addr));
		ip_addr = std::string(ipbuf);
	}
	
	//static method for getting ip_addr of remote hosts given hostname	
	static std::string getIPAddress(std::string hostStr) {
		const char* hostbuf;
		char *ipbuf;	
		hostbuf = hostStr.c_str();
		struct hostent *localhost = gethostbyname(hostbuf);
		ipbuf = inet_ntoa(*((struct in_addr*) localhost->h_addr));
		return std::string(ipbuf);	
	}

	//static method for generating a unique port address per lock number	
	static std::string generatePort(unsigned int lockno) {
		char portno[6];
		snprintf(portno, 6, "52%03d", lockno);
		return std::string(portno);	
	}
	
	//static method for populating node hostnames array with client nodes (no server)	
	static void readNodeList() {
		std::ifstream nodeList("node_list.txt", std::ifstream::in);
		for(int i = 0; i < NODES+1; i++ )
			nodeList.getline(nodes[i], HOSTNAME_LEN);
	}
};


class DMLNode : public Node {
public:
	int myseqno = 0;;
	int highestseqno = 0;
	bool requesting_cs[LOCK_COUNT] = { false };

	struct args {
		char hostname[HOSTNAME_LEN];
		unsigned int lockno;
	};
	//constructor for DMLNode class...	
	DMLNode() : Node() {}
};


class DSMNode : public Node {
public:
	static char server[HOSTNAME_LEN]; 
	
	struct dataseg_t {
		void* start;
		size_t size;
		int prot;
		bool dirty;
		char value[PAGE_SIZE * PAGE_COUNT];
	};
	static dataseg_t datasegs[NODES][DATASEG_COUNT];
	
	DSMNode() : Node() {}

	//static method for populating server var
	static void setServer() {
		strncpy(server, nodes[NODES], HOSTNAME_LEN);
	}
};

void logDML(std::string sender, std::string receiver, std::string function, unsigned int lockno, int seqno) {
	//log rpc info message to file, appending to the end
	std::ofstream log;
	log.open("rpc-log-file.txt", std::ios::out | std::ios::app);
	if(!log.is_open()) {std::cout << "error: couldn't open file" << std::endl; return;}
	log << "----RPC call from <" << sender << "> to <" << receiver << "> for <" << 
		function << "> with arguments: [lockno=" << lockno << ", seqno=" << seqno << "]----" << std::endl;
	log.close();
}

void logDSM(std::string sender, std::string receiver, std::string function, int prot) {
	//log rpc info message to file, appending to the end
	std::ofstream log;
	log.open("rpc-log-file.txt", std::ios::out | std::ios::app);
	if(!log.is_open()) {std::cout << "error: couldn't open file" << std::endl; return;}
	log << "----RPC call from <" << sender << "> to <" << receiver << "> for <" << 
		function << "> with arguments: [protections=" << prot << "]----" << std::endl;
	log.close();
}

