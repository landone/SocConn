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

void SC_Server::send(int id, string msg) {

	if (!isValidID(id)) {
		return;
	}
	clients[id]->soc[SC_TCP]->send(msg);

}

void SC_Server::send(int id, int val) {

	if (!isValidID(id)) {
		return;
	}
	clients[id]->soc[SC_TCP]->send(val);

}

void SC_Server::send(int id, const char* buf, int len) {

	if (!isValidID(id)) {
		return;
	}
	clients[id]->soc[SC_TCP]->send(buf, len);

}

void SC_Server::sendToAll(string msg) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			clients[i]->soc[SC_TCP]->send(msg);
		}
	}
}

void SC_Server::sendToAll(int val) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			clients[i]->soc[SC_TCP]->send(val);
		}
	}
}

void SC_Server::sendToAll(const char* buf, int len) {
	for (unsigned int i = 0; i < maxClients; i++) {
		if (clients[i] != nullptr) {
			clients[i]->soc[SC_TCP]->send(buf, len);
		}
	}
}

void SC_Server::start() {

	if (!isRunning()) {
		server[SC_TCP].open();
		server[SC_UDP].open();
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

		for (unsigned int i = 0; i < 2; i++) {//Go through both TCP & UDP

			if (server[i].hasData(50)) {

				client = server[i].acquire();
				if (client == nullptr) {
					continue;
				}

				bool matched = false;
				for (unsigned int u = 0; u < maxClients; u++) {//Look for matching alternate socket
					if (clients[u] != nullptr && clients[u]->soc[i] == nullptr) {
						SC_Socket* otherSoc = clients[u]->soc[(i + 1) % 2];
						if (otherSoc->getIP() == client->getIP()) {//Found match
							clients[u]->soc[i] = client;
							clients[u]->thr[i] = (char*)new thread(&SC_Server::clientThread, this, u, (SC_SocType)i);
							onConnect(u);//Call once fully connected
							matched = true;
							break;
						}
					}
				}

				if (matched) {
					continue;
				}

				if (clientCount >= maxClients) {
					client->disconnect();
					delete client;
					continue;
				}

				for (unsigned int u = 0; u < maxClients; u++) {//Look for open index
					if (clients[u] == nullptr) {
						if (i == 0) {
							clients[u] = new SC_ClientHandle{ {nullptr, nullptr}, {client, nullptr}, false };
						}else{
							clients[u] = new SC_ClientHandle{ { nullptr, nullptr }, {nullptr, client}, false };
						}
						clientCount++;
						clients[u]->thr[i] = (char*)new thread(&SC_Server::clientThread, this, u, (SC_SocType)i);
						break;
					}
				}

			}

		}

	}

}

void SC_Server::clientThread(unsigned int id, SC_SocType socType) {

	SC_Packet pack;
	bool timeout = false;

	while (true) {

		if (clients[id]->stopMe) {
			return;
		}

		if (!clients[id]->soc[socType]->hasData(50)) {
			continue;
		}

		pack = clients[id]->soc[socType]->receivePacket();

		if (pack == SC_Packet_ERROR) {
			if (socType == SC_TCP) {
				timeout = true;
				break;
			}else{
				continue;
			}
		}

		if (pack == SC_Packet_DISCONNECT) {
			if (socType == SC_TCP) {
				break;
			}else{
				continue;
			}
		}

		switch (pack) {
		case SC_Packet_INT:
			onInt(id, clients[id]->soc[socType]->receiveInt());
			break;
		case SC_Packet_BYTES: {
			int len = 0;
			onBytes(id, clients[id]->soc[socType]->receiveBytes(len), len);
			}
			break;
		case SC_Packet_STRING:
			onString(id, clients[id]->soc[socType]->receiveStr());
			break;
		}

	}

	//ONLY TCP THREAD CAN REACH THIS AREA OF CODE BELOW

	onDisconnect(id, timeout);//Call right before actual disconnection

	clients[id]->soc[SC_TCP]->disconnect();
	delete clients[id]->soc[SC_TCP];
	if (clients[id]->soc[SC_UDP] != nullptr) {//Stop UDP thread if exists

		clients[id]->stopMe = true;
		((thread*)clients[id]->thr[SC_UDP])->join();
		clients[id]->stopMe = false;
		delete clients[id]->thr[SC_UDP];

		clients[id]->soc[SC_UDP]->disconnect();
		delete clients[id]->soc[SC_UDP];

	}
	SC_ClientHandle* info = clients[id];//Hold so that it may be made null before deletion
	clients[id] = nullptr;
	clientCount--;//After setting to nullptr
	thread* tcp = (thread*)info->thr[SC_TCP];
	delete info;
	tcp->detach();
	delete tcp;

}

void SC_Server::kick(unsigned int id) {

	if (!isValidID(id)) {
		return;
	}

	clients[id]->stopMe = true;
	for (int i = 0; i < 2; i++) {//Close TCP/UDP threads
		if (clients[id]->thr[i] != nullptr) {
			((thread*)clients[id]->thr[i])->join();
		}
	}
	clients[id]->stopMe = false;

	onDisconnect(id, false);

	for (int i = 0; i < 2; i++) {
		if (clients[id]->soc[i] != nullptr) {//Could possibly be unpaired
			clients[id]->soc[i]->disconnect();
			delete clients[id]->soc[i];
		}
	}

	SC_ClientHandle* info = clients[id];
	clients[id] = nullptr;
	clientCount--;
	thread* tcp = (thread*)info->thr[SC_TCP];
	thread* udp = (thread*)info->thr[SC_UDP];
	delete info;
	if (tcp != nullptr) {
		delete tcp;
	}
	if (udp != nullptr) {
		delete udp;
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