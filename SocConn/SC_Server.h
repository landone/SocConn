/*

	SC_Server
	(TCP/UDP : IPV4)
	----------------
Abstract class that interprets sockets as a server would.
Sends & receives packets with clients
Clients are organized by an ID (unchanging integer)
Functions for receiving data must be defined

*/

#pragma once

#include "SC_Socket.h"

class SC_Server {
public:

	SC_Server(std::string ip, unsigned int port, unsigned int maxClients);
	SC_Server(unsigned int port, unsigned int maxClients);
	~SC_Server();

	void start();
	void stop();

	void send(int client, std::string msg, bool TCP = true);
	void send(int client, int val, bool TCP = true);
	void send(int client, const char* buf, int len, bool TCP = true);

	void sendToAll(std::string msg, bool TCP = true);
	void sendToAll(int val, bool TCP = true);
	void sendToAll(const char* buf, int len, bool TCP = true);

	void kick(unsigned int clientID);
	void kickAll();

	bool isRunning() {
		return server[SC_TCP].isConnected();
	}
	int getClientCount() { return clientCount; }
	int getMaxClients() { return maxClients; }
	std::string getClientIP(unsigned int clientID) { return clients[clientID]->soc->getIP(); }
	int getClientPort(unsigned int clientID) { return clients[clientID]->soc->getPort(); }
	int getPort() { return server[SC_TCP].getPort(); }

	void setPort(int port);
	void setMaxClients(unsigned int maxClients);

protected:

	virtual void onString(int client, std::string msg) = 0;
	virtual void onInt(int client, int value) = 0;
	virtual void onBytes(int client, const char* buf, int len) = 0;//Char* received should be deleted
	virtual void onConnect(int client) = 0;
	virtual void onDisconnect(int client, bool timeout) = 0;

private:

	struct SC_ClientHandle {
		char* thr;
		SC_Socket* soc;
		SC_ADDR addr;
		bool stopMe;
	};

	const unsigned int ABS_MAX_CLIENT = 64;

	unsigned int maxClients = 0;
	bool stopThreads = false;

	SC_Socket server[2];
	char* connThr;
	char* udpThr;

	SC_ClientHandle** clients = nullptr;
	unsigned int clientCount = 0;
	bool isValidID(unsigned int id);

	void connectThread();
	void udpThread();
	void clientThread(unsigned int id);

};