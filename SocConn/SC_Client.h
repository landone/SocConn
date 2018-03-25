/*

	SC_Client
	(TCP/UDP : IPV4)
	----------------
Abstract class that interprets sockets as a client would.
Sends & receives packets with servers
Functions for receiving data must be defined

*/

#pragma once

#include "SC_Socket.h"

class SC_Client {
public:

	SC_Client(std::string ip, unsigned int port);
	SC_Client(unsigned int port);//Assumes localhost here

	bool connect();
	void disconnect();

	void send(std::string msg, bool TCP = true);
	void send(int val, bool TCP = true);
	void send(const char* buf, int len, bool TCP = true);

	std::string getIP() { return me[SC_TCP].getIP(); }
	int getPort() { return me[SC_TCP].getPort(); }
	bool isConnected() { return me[SC_TCP].isConnected(); }

	void setIP(std::string ip) { me[SC_TCP].setIP(ip); me[SC_UDP].setIP(ip); }
	void setPort(int port) { me[SC_TCP].setPort(port); me[SC_UDP].setPort(port); }

protected:

	virtual void onString(std::string msg) = 0;
	virtual void onInt(int value) = 0;
	virtual void onBytes(const char* buf, int len) = 0;//Char* received should be deleted

private:

	SC_Socket me[2];
	bool stopThreads = false;

	char* mainThr[2] = { nullptr, nullptr };//TCP & UDP threads
	void mainThread(SC_SocType type);

};