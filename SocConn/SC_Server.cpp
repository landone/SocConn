#include "SC_Server.h"
#include <string>
#include <thread>

using namespace std;

SC_Server::SC_Server(std::string ip, unsigned int port, unsigned int maxClients) : server{ SC_Socket(ip, port, true), SC_Socket(ip, port, false) } {

	setMaxClients(maxClients);//Also initiates/modifies clients array

}

SC_Server::SC_Server(unsigned int port, unsigned int maxClients) : server{ SC_Socket(port, true), SC_Socket(port, false) } {
	setMaxClients(maxClients);
}

SC_Server::~SC_Server() {

	stop();

	delete[] clients;

}

void SC_Server::send(int id, string msg, bool TCP) {

	if (!isValidID(id)) {
		return;
	}
	if (TCP) {
		clients[id]->soc->send(msg);
	}
	else {
		server[SC_UDP].send(msg, clients[id]->addr);
	}

}

void SC_Server::send(int id, int val, bool TCP) {

	if (!isValidID(id)) {
		return;
	}
	if (TCP) {
		clients[id]->soc->send(val);
	}
	else {
		server[SC_UDP].send(val, clients[id]->addr);
	}

}

void SC_Server::send(int id, const char* buf, int len, bool TCP) {

	if (!isValidID(id)) {
		return;
	}
	if (TCP) {
		clients[id]->soc->send(buf, len);
	}else {
		server[SC_UDP].send(buf, len, clients[id]->addr);
	}

}

void SC_Server::sendToAll(string msg, bool TCP) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			if (TCP) {
				clients[i]->soc->send(msg);
			}
			else {
				server[SC_UDP].send(msg, clients[i]->addr);
			}
		}
	}
}

void SC_Server::sendToAll(int val, bool TCP) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			if (TCP) {
				clients[i]->soc->send(val);
			}
			else {
				server[SC_UDP].send(val, clients[i]->addr);
			}
		}
	}
}

void SC_Server::sendToAll(const char* buf, int len, bool TCP) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			if (TCP) {
				clients[i]->soc->send(buf, len);
			}else{
				server[SC_UDP].send(buf, len, clients[i]->addr);
			}
		}
	}
}

void SC_Server::start() {

	if (!isRunning()) {
		server[SC_TCP].open();
		server[SC_UDP].open();
		connThr = (char*)new thread(&SC_Server::connectThread, this);
		udpThr = (char*)new thread(&SC_Server::udpThread, this);
	}

}

void SC_Server::stop() {

	if (isRunning()) {

		stopThreads = true;
		((thread*)connThr)->join();
		((thread*)udpThr)->join();
		stopThreads = false;
		delete (thread*)connThr;
		delete (thread*)udpThr;

		kickAll();

		server[SC_TCP].disconnect();
		server[SC_UDP].disconnect();
	}

}

void SC_Server::connectThread() {//Accepts new connections

	SC_Socket* client;

	while (true) {

		if (stopThreads) {
			return;
		}

		if (server[SC_TCP].hasData(50)) {
			client = server[SC_TCP].acquire();
			if (client == nullptr) {
				continue;
			}

			if (clientCount >= maxClients) {
				client->disconnect();
				delete client;
				continue;
			}

			for (unsigned int u = 0; u < maxClients; u++) {//Look for open index
				if (clients[u] == nullptr) {
					clients[u] = new SC_ClientHandle{ nullptr, client, client->getLastAddr(), false };
					clientCount++;
					onConnect(u);
					clients[u]->thr = (char*)new thread(&SC_Server::clientThread, this, u);
					break;
				}
			}

		}

	}

}

void SC_Server::udpThread() {

	SC_Packet pack;
	SC_ADDR addr;

	while (true) {

		if (stopThreads) {
			return;
		}

		if (!server[SC_UDP].hasData(50)) {
			continue;
		}

		std::cout << "UDP FOUND DATA\n";

		pack = server[SC_UDP].receivePacket();
		addr = server[SC_UDP].getLastAddr();

		std::cout << "pack = " << pack << std::endl;

		if (pack == SC_Packet_ERROR || pack == SC_Packet_DISCONNECT) {
			continue;
		}

		std::cout << "DEBUG\n";

		int id = -1;
		for (unsigned int i = 0; i < maxClients; i++) {//Find address corresponding client

			if (clients[id]->addr.ip == addr.ip && clients[id]->addr.port == addr.port) {
				id = i;
				break;
			}

		}

		std::cout << "ID: " << id << std::endl;

		switch (pack) {
		case SC_Packet_INT:
			onInt(id, server[SC_UDP].receiveInt());
			break;
		case SC_Packet_BYTES: {
			int len = 0;
			onBytes(id, server[SC_UDP].receiveBytes(len), len);
			}
			break;
		case SC_Packet_STRING:
			onString(id, server[SC_UDP].receiveStr());
			break;
		}

	}

}

void SC_Server::clientThread(unsigned int id) {

	SC_Packet pack;
	bool timeout = false;

	while (true) {

		if (clients[id]->stopMe) {
			return;
		}

		if (!clients[id]->soc->hasData(50)) {
			continue;
		}

		pack = clients[id]->soc->receivePacket();

		if (pack == SC_Packet_ERROR) {
			timeout = true;
			break;
		}

		if (pack == SC_Packet_DISCONNECT) {
			break;
		}

		switch (pack) {
		case SC_Packet_INT:
			onInt(id, clients[id]->soc->receiveInt());
			break;
		case SC_Packet_BYTES: {
			int len = 0;
			onBytes(id, clients[id]->soc->receiveBytes(len), len);
			}
			break;
		case SC_Packet_STRING:
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
	thread* tcp = (thread*)info->thr;
	delete info;
	tcp->detach();
	delete tcp;

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
	thread* tcp = (thread*)info->thr;
	delete info;
	if (tcp != nullptr) {
		delete tcp;
	}

}

void SC_Server::kickAll() {

	for (unsigned int i = 0; i < maxClients; i++) {
		//isValidID() not required, as it is in kick()
		kick(i);
	}

}

void SC_Server::setPort(int port) {

	server[SC_TCP].setPort(port);
	server[SC_UDP].setPort(port);

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