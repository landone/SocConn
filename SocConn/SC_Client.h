/*

	SC_Client
	(TCP/IPV4)
	---------
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

	void send(std::string msg);
	void send(int val);
	void send(const char* buf, int len);

	std::string getIP() { return me.getIP(); }
	int getPort() { return me.getPort(); }
	bool isConnected() { return me.isConnected(); }

	void setIP(std::string ip) { me.setIP(ip); }
	void setPort(int port) { me.setPort(port); }

protected:

	virtual void onString(std::string msg) = 0;
	virtual void onInt(int value) = 0;
	virtual void onBytes(const char* buf, int len) = 0;//Char* received should be deleted

private:

	SC_Socket me;
	bool stopThreads = false;

	char* mainThr = nullptr;//Thread
	void mainThread();

};