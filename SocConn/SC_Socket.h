/*

	SC_Socket
	(TCP/UDP : IPV4)
	----------------
Fundamental class that organizes common operations of a WinSock socket.
Offers functions and enum of common packet types.
Static function for cleaning up all WinSock data when finished.
Changing properties such as IP, port, or protocol require reconnection.

*/

#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS		1
#pragma comment(lib, "ws2_32.lib")
#include <iostream>

enum SC_SocType {
	SC_TCP,
	SC_UDP
};

enum SC_Packet {
	SC_Packet_ERROR,
	SC_Packet_INT,
	SC_Packet_BYTES,
	SC_Packet_STRING,
	SC_Packet_DISCONNECT
};

struct SC_ADDR {
	unsigned long ip;
	unsigned short port;
};

class SC_Socket {
public:

	SC_Socket(std::string ip, unsigned int port, bool TCP);
	SC_Socket(unsigned int port, bool TCP);
	~SC_Socket();

	static void cleanUpAll();

	void open();
	SC_Socket* acquire();
	bool join();
	void disconnect();
	bool hasData(long timeout);

	void send(int val, SC_ADDR client = { NULL, NULL });
	void send(std::string msg, SC_ADDR client = { NULL, NULL });
	void send(const char* bytes, int length, SC_ADDR client = { NULL, NULL });

	SC_Packet receivePacket();
	int receiveInt();
	std::string receiveStr();
	char* receiveBytes(int& length);

	void setIP(std::string ip);
	void setPort(unsigned int port);
	void setProtocol(bool TCP);

	bool isConnected() { return connected; }
	bool isTCP() { return TCP; }
	std::string getIP() { return ip; }
	int getPort() { return port; }
	SC_ADDR getLastAddr();

private:

	unsigned int mySoc;

	static bool wsaEnabled;

	bool connected = false;
	bool TCP = true;

	void sendPacket(SC_Packet pack);

	char* readfds = nullptr;

protected:
	SC_Socket(unsigned int socket);
	char* addr = nullptr;
	char* recvAddr = nullptr;
	std::string ip;
	unsigned int port;
};