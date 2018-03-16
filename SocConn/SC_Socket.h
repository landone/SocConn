/*

	SC_Socket
	(TCP/IPV4)
	---------
Fundamental class that organizes common operations of a WinSock socket.
Offers functions and enum of common packet types.
Static function for cleaning up all WinSock data when finished.

*/

#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS		1
#pragma comment(lib, "ws2_32.lib")
#include <iostream>

enum Packet {
	Packet_ERROR,
	Packet_PING,
	Packet_INT,
	Packet_BYTES,
	Packet_STRING,
	Packet_DISCONNECT
};

class SC_Socket {
public:

	SC_Socket(std::string ip, unsigned int port);
	SC_Socket(unsigned int port);
	~SC_Socket();

	static void cleanUpAll();

	void open();
	SC_Socket* acquire();
	bool join();
	void disconnect();
	bool hasData(long timeout);

	void ping();
	void send(int val);
	void send(std::string msg);
	void sendBytes(const char* bytes, int length);

	Packet receivePacket();
	int receiveInt();
	std::string receiveStr();
	char* receiveBytes(int& length);

	void setIP(std::string ip);
	void setPort(unsigned int port);

	bool isConnected() {
		return connected;
	}

	std::string getIP() {
		return ip;
	}

	int getPort() {
		return port;
	}

private:

	unsigned int mySoc;
	char* addr = nullptr;

	static bool wsaEnabled;

	std::string ip;
	unsigned int port;
	bool connected = false;

	void sendPacket(Packet pack);

	char* readfds = nullptr;

protected:
	SC_Socket(unsigned int socket, bool extraParam);

};