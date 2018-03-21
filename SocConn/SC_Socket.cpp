#include "SC_Socket.h"
#include <WinSock2.h>

using namespace std;

bool SC_Socket::wsaEnabled = false;

void SC_Socket::cleanUpAll() {

	WSACleanup();

}

SC_Socket::SC_Socket(unsigned int socket, char* extraParam) {

	mySoc = socket;
	connected = true;
	readfds = (char*)new fd_set;

	int type = 0;
	int len = sizeof(int);
	getsockopt(mySoc, SOL_SOCKET, SO_TYPE, (char*)&type, &len);
	TCP = (type == SOCK_STREAM);

	struct sockaddr_in ad;
	len = 0;
	getpeername(mySoc, (struct sockaddr*)&ad, &len);

	addr = (char*)new SOCKADDR_IN(ad);

	this->ip = inet_ntoa((in_addr)ad.sin_addr);
	this->port = ntohs(ad.sin_port);
	
}

SC_Socket::SC_Socket(unsigned int port, bool TCP) : SC_Socket("127.0.0.1", port, TCP) {

	//Do nothing

}

SC_Socket::SC_Socket(string ip, unsigned int port, bool TCP) {

	this->ip = ip;
	this->port = port;
	this->TCP = TCP;
	readfds = (char*)new fd_set;

	if (!wsaEnabled) {

		WSAData data;
		WORD DllVersion = MAKEWORD(2, 1);
		if (WSAStartup(DllVersion, &data) != 0) {

			cout << "SC_Socket Error : Winsock startup failed" << endl;
			return;

		}

	}
	addr = (char*)new SOCKADDR_IN;
	((SOCKADDR_IN*)addr)->sin_addr.s_addr = inet_addr(ip.c_str());
	((SOCKADDR_IN*)addr)->sin_port = htons(port);
	((SOCKADDR_IN*)addr)->sin_family = AF_INET;//IPv4

}

SC_Socket::~SC_Socket() {

	delete ((SOCKADDR_IN*)addr);
	delete (fd_set*)readfds;

}

bool SC_Socket::hasData(long timeOut) {
	timeval val;
	val.tv_sec = timeOut / 1000;
	val.tv_usec = timeOut % 1000;
	FD_ZERO(readfds);
	FD_SET(mySoc, readfds);
	int output = select(0, (fd_set*)readfds, NULL, NULL, &val);
	return output > 0;
}

void SC_Socket::sendPacket(SC_Packet pack) {

	sendto(mySoc, (char*)&pack, sizeof(pack), 0, (SOCKADDR*)addr, sizeof(SOCKADDR_IN));

}

void SC_Socket::disconnect() {

	if (connected) {
		sendPacket(SC_Packet_DISCONNECT);
		shutdown(mySoc, SD_SEND);
		closesocket(mySoc);
		connected = false;
	}

}

void SC_Socket::send(int num) {

	sendPacket(SC_Packet_INT);
	sendto(mySoc, (char*)&num, sizeof(int), 0, (SOCKADDR*)addr, sizeof(SOCKADDR_IN));

}

SC_Packet SC_Socket::receivePacket() {

	char buf[sizeof(SC_Packet)];
	if (recv(mySoc, buf, sizeof(SC_Packet), 0) == SOCKET_ERROR) {
		return SC_Packet_ERROR;
	}

	return *((SC_Packet*)buf);

}

int SC_Socket::receiveInt() {

	char buf[sizeof(int)];
	if (recv(mySoc, buf, sizeof(int), 0) == SOCKET_ERROR) {
		return -1;
	}

	return *((int*)buf);

}

char* SC_Socket::receiveBytes(int& length) {

	if (receivePacket() != SC_Packet_INT) {
		length = 0;
		return nullptr;
	}
	length = receiveInt();
	std::cout << "Byte Amount: " << length << std::endl;
	if (length < 0) {
		return nullptr;
	}

	char* buf = new char[length];
	if (recv(mySoc, buf, length, 0) == SOCKET_ERROR) {
		delete[] buf;
		length = 0;
		return nullptr;
	}

	return buf;

}

std::string SC_Socket::receiveStr() {

	if (receivePacket() != SC_Packet_INT) {
		return "";
	}
	int length = receiveInt();
	if (length < 0) {
		return "";
	}
	
	char* buf = new char[length+1];
	if (recv(mySoc, buf, length, 0) == SOCKET_ERROR) {
		delete[] buf;
		return "";
	}

	buf[length] = '\0';
	std::string output(buf);
	delete[] buf;
	return output;

}

void SC_Socket::send(std::string msg) {

	sendPacket(SC_Packet_STRING);
	send(msg.length());
	sendto(mySoc, (char*)msg.c_str(), msg.length(), 0, (SOCKADDR*)addr, sizeof(SOCKADDR_IN));

}

void SC_Socket::send(const char* bytes, int length) {

	sendPacket(SC_Packet_BYTES);
	send(length);
	sendto(mySoc, bytes, length, 0, (SOCKADDR*)addr, sizeof(SOCKADDR_IN));

}

void SC_Socket::setIP(string ip) {

	this->ip = ip;
	((SOCKADDR_IN*)addr)->sin_addr.s_addr = inet_addr(ip.c_str());

}

void SC_Socket::setPort(unsigned int port) {

	this->port = port;
	((SOCKADDR_IN*)addr)->sin_port = htons(port);

}

void SC_Socket::setProtocol(bool TCP) {

	this->TCP = TCP;

}

void SC_Socket::open() {

	connected = true;
	mySoc = socket(AF_INET, (TCP ? SOCK_STREAM : SOCK_DGRAM), NULL);
	FD_ZERO(readfds);
	FD_SET(mySoc, readfds);
	bind(mySoc, (SOCKADDR*)addr, sizeof(SOCKADDR_IN));
	listen(mySoc, SOMAXCONN);

}

SC_Socket* SC_Socket::acquire() {

	SOCKET newConn;
	int addrSize = sizeof(SOCKADDR_IN);
	newConn = accept(mySoc, (SOCKADDR*)addr, &addrSize);

	if (newConn == INVALID_SOCKET) {
		return nullptr;
	}
	
	return new SC_Socket(newConn, nullptr);

}

bool SC_Socket::join() {

	if (connected) {
		return false;
	}

	mySoc = socket(AF_INET, (TCP ? SOCK_STREAM : SOCK_DGRAM), NULL);
	FD_ZERO(readfds);
	FD_SET(mySoc, readfds);

	int addrSize = sizeof(SOCKADDR_IN);
	if (connect(mySoc, (SOCKADDR*)addr, addrSize) != 0) {

		cout << "SC_Socket Error : Failed to connect to address" << endl;
		connected = false;
		return false;

	}

	connected = true;
	return true;

}