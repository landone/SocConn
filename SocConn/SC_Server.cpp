#include "SC_Server.h"
#include <string>
#include <thread>

using namespace std;

SC_Server::SC_Server(unsigned int port, unsigned int maxClients) : server(port) {

	setMaxClients(maxClients);//Also initiates/modifies clients array

}

SC_Server::~SC_Server() {

	stop();

	delete[] clients;

}

void SC_Server::send(int id, string msg) {

	if (!isValidID(id)) {
		return;
	}
	clients[id]->soc->send(msg);

}

void SC_Server::send(int id, int val) {

	if (!isValidID(id)) {
		return;
	}
	clients[id]->soc->send(val);

}

void SC_Server::send(int id, const char* buf, int len) {

	if (!isValidID(id)) {
		return;
	}
	clients[id]->soc->sendBytes(buf, len);

}

void SC_Server::sendToAll(string msg) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			clients[i]->soc->send(msg);
		}
	}
}

void SC_Server::sendToAll(int val) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			clients[i]->soc->send(val);
		}
	}
}

void SC_Server::sendToAll(const char* buf, int len) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			clients[i]->soc->sendBytes(buf, len);
		}
	}
}

void SC_Server::start() {

	if (!isRunning()) {
		server.open();
		connThr = (char*)new thread(&SC_Server::connectThread, this);
	}

}

void SC_Server::stop() {

	if (isRunning()) {

		stopThreads = true;
		((thread*)connThr)->join();
		stopThreads = false;
		delete (thread*)connThr;

		kickAll();

		server.disconnect();
	}

}

void SC_Server::connectThread() {//Accepts new connections

	SC_Socket* client;

	while (true) {

		if (stopThreads) {
			return;
		}

		if (!server.hasData(50)) {
			continue;
		}

		client = server.acquire();
		if (client == nullptr) {
			continue;
		}

		if (clientCount >= maxClients) {
			client->disconnect();
			delete client;
			continue;
		}

		for (unsigned int i = 0; i < maxClients; i++) {//Look for open index
			if (clients[i] == nullptr) {
				clients[i] = new SC_ClientHandle{ nullptr, client, false };//Establish data before starting thread
				clientCount++;
				clients[i]->thr = (char*)new thread(&SC_Server::clientThread, this, i);
				onConnect(i);
				break;
			}
		}

	}

}

void SC_Server::clientThread(unsigned int id) {

	Packet pack;
	bool timeout = false;

	while (true) {

		if (clients[id]->stopMe) {
			return;
		}

		if (!clients[id]->soc->hasData(50)) {
			continue;
		}

		pack = clients[id]->soc->receivePacket();

		if (pack == Packet_ERROR) {
			timeout = true;
			break;
		}

		if (pack == Packet_DISCONNECT) {
			break;
		}

		switch (pack) {
		case Packet_PING:
			break;
		case Packet_INT:
			onInt(id, clients[id]->soc->receiveInt());
			break;
		case Packet_BYTES: {
			int len = 0;
			onBytes(id, clients[id]->soc->receiveBytes(len), len);
			}
			break;
		case Packet_STRING:
			onString(id, clients[id]->soc->receiveStr());
			break;
		}

	}

	onDisconnect(id, timeout);//Call right before actual disconnection

	clients[id]->soc->disconnect();
	delete clients[id]->soc;
	SC_ClientHandle* info = clients[id];//Hold so that it may be made null before deletion
	clients[id] = nullptr;
	clientCount--;//After setting to nullptr
	thread* thr = (thread*)info->thr;
	delete info;
	thr->detach();
	delete thr;

}

void SC_Server::kick(unsigned int id) {

	if (!isValidID(id)) {
		return;
	}

	clients[id]->stopMe = true;
	((thread*)clients[id]->thr)->join();
	clients[id]->stopMe = false;

	onDisconnect(id, false);

	clients[id]->soc->disconnect();
	delete clients[id]->soc;

	SC_ClientHandle* info = clients[id];
	clients[id] = nullptr;
	clientCount--;
	thread* thr = (thread*)info->thr;
	delete info;
	delete thr;

}

void SC_Server::kickAll() {

	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			kick(i);
		}
	}

}

void SC_Server::setPort(int port) {

	server.setPort(port);

}

void SC_Server::setMaxClients(unsigned int newAmt) {

	if (maxClients <= ABS_MAX_CLIENT) {
		if (maxClients >= 0) {
			this->maxClients = newAmt;
		}
		else {
			this->maxClients = 0;
		}
	}
	else {
		this->maxClients = ABS_MAX_CLIENT;
	}

	SC_ClientHandle** newArray = new SC_ClientHandle*[newAmt];
	for (unsigned int i = 0; i < newAmt; i++) {
		if (i < maxClients && clients != nullptr && clients[i] != nullptr) {
			newArray[i] = clients[i];//Move allowed indices over
		}
		else {
			newArray[i] = nullptr;
		}
	}

	if (maxClients > newAmt && clients != nullptr) {
		for (unsigned int i = newAmt; i < maxClients; i++) {
			if (clients[i] != nullptr) {
				kick(i);//Kick clients that aren't in new array
			}
		}
	}

	//Finally set values
	if (clients != nullptr) {
		delete[] clients;
	}
	clients = newArray;
	maxClients = newAmt;

}

bool SC_Server::isValidID(unsigned int id) {

	return id < maxClients && id >= 0 && clients[id] != nullptr;

}