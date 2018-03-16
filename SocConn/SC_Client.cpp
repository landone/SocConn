#include "SC_Client.h"
#include <string>
#include <thread>

using namespace std;

SC_Client::SC_Client(std::string ip, unsigned int port) : me(ip, port) {

	//Do nothing

}

SC_Client::SC_Client(unsigned int port) : me(port) {

	//Do nothing

}

void SC_Client::send(std::string msg) {

	me.send(msg);

}

void SC_Client::send(int val) {

	me.send(val);

}

void SC_Client::send(const char* buf, int len) {

	me.sendBytes(buf, len);

}

bool SC_Client::connect() {

	if (me.join()) {

		mainThr = (char*)new thread(&SC_Client::mainThread, this);
		return true;

	}

	return false;

}

void SC_Client::disconnect() {

	if (!isConnected()) {
		return;
	}

	stopThreads = true;
	((thread*)mainThr)->join();
	stopThreads = false;
	me.disconnect();

}

void SC_Client::mainThread() {

	Packet pack;
	while (true) {

		if (stopThreads) {
			return;
		}

		if (!me.hasData(50)) {
			continue;
		}

		pack = me.receivePacket();

		if (pack == Packet_ERROR || pack == Packet_DISCONNECT) {
			break;
		}

		switch (pack) {
		case Packet_PING:
			break;
		case Packet_INT:
			onInt(me.receiveInt());
			break;
		case Packet_BYTES: {
				int len = 0;
				onBytes(me.receiveBytes(len), len);
			}
			break;
		case Packet_STRING:
			onString(me.receiveStr());
			break;
		}

	}

	me.disconnect();
	((thread*)mainThr)->detach();
	delete ((thread*)mainThr);

}