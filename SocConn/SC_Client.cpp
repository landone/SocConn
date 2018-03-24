#include "SC_Client.h"
#include <string>
#include <thread>

using namespace std;

SC_Client::SC_Client(std::string ip, unsigned int port) : me{ SC_Socket(ip, port, true), SC_Socket(ip, port, false) } {

	//Do nothing
	cout << "me[0] = " << (me[0].isTCP() ? "TCP" : "UDP") << endl;
	cout << "me[1] = " << (me[1].isTCP() ? "TCP" : "UDP") << endl;

}

SC_Client::SC_Client(unsigned int port) : me{ SC_Socket(port, true), SC_Socket(port, false) } {

	//Do nothing

}

void SC_Client::send(std::string msg) {

	me[SC_TCP].send(msg);

}

void SC_Client::send(int val) {

	me[SC_TCP].send(val);

}

void SC_Client::send(const char* buf, int len) {

	me[SC_TCP].send(buf, len);

}

bool SC_Client::connect() {

	if (isConnected()) {
		return false;
	}

	if (me[SC_TCP].join()) {

		me[SC_UDP].join();

		mainThr[SC_TCP] = (char*)new thread(&SC_Client::mainThread, this, SC_TCP);
		mainThr[SC_UDP] = (char*)new thread(&SC_Client::mainThread, this, SC_UDP);
		return true;

	}

	return false;

}

void SC_Client::disconnect() {

	if (!isConnected()) {
		return;
	}

	stopThreads = true;
	((thread*)mainThr[SC_UDP])->join();
	((thread*)mainThr[SC_TCP])->join();
	stopThreads = false;

	me[SC_UDP].disconnect();
	me[SC_TCP].disconnect();

}

void SC_Client::mainThread(SC_SocType type) {

	SC_Packet pack;
	while (true) {

		if (stopThreads) {
			return;
		}

		if (!me[type].hasData(50)) {
			continue;
		}

		pack = me[type].receivePacket();

		if (pack == SC_Packet_ERROR || pack == SC_Packet_DISCONNECT) {
			if (type == SC_TCP) {
				break;
			}else{
				continue;
			}
		}

		switch (pack) {
		case SC_Packet_INT:
			onInt(me[type].receiveInt());
			break;
		case SC_Packet_BYTES: {
			int len = 0;
			onBytes(me[type].receiveBytes(len), len);
			}
			break;
		case SC_Packet_STRING:
			onString(me[type].receiveStr());
			break;
		}

	}

	//ONLY TCP THREAD REACHES THE CODE BELOW

	stopThreads = true;
	((thread*)mainThr[SC_UDP])->join();
	stopThreads = false;
	me[SC_UDP].disconnect();
	delete ((thread*)mainThr[SC_UDP]);

	me[SC_TCP].disconnect();
	((thread*)mainThr[SC_TCP])->detach();
	delete ((thread*)mainThr[SC_TCP]);

}