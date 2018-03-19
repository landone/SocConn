/*

	SC_Server
	(TCP/IPV4)
	---------
Abstract class that interprets sockets as a server would.
Sends & receives packets with clients
Clients are organized by an ID (unchanging integer)
Functions for receiving data must be defined

*/

#pragma once

#include "SC_Socket.h"

class SC_Server {
public:

	SC_Server(unsigned int port, unsigned int maxClients);
	~SC_Server();

	void start();
	void stop();

	void send(int client, std::string msg);
	void send(int client, int val);
	void send(int client, const char* buf, int len);

	void sendToAll(std::string msg);
	void sendToAll(int val);
	void sendToAll(const char* buf, int len);

	void kick(unsigned int clientID);
	void kickAll();

	bool isRunning() {
		return server.isConnected();
	}
	int getClientCount() { return clientCount; }
	int getMaxClients() { return maxClients; }
	int getPort() { return server.getPort(); }

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
		bool stopMe;
	};

	const unsigned int ABS_MAX_CLIENT = 64;

	unsigned int maxClients = 0;
	bool stopThreads = false;

	SC_Socket server;
	char* connThr;

	SC_ClientHandle** clients = nullptr;
	unsigned int clientCount = 0;
	bool isValidID(unsigned int id);

	void connectThread();
	void clientThread(unsigned int id);

};