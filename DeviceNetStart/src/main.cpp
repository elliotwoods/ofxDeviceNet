#include "ofMain.h"
#include "ofApp.h"
#include "ofxDeviceNet.h"
#include "ofxOsc.h"

#define HOST "localhost"
#define PORT 8888
#define NUM_MSG_STRINGS 20

ofxOscReceiver receiver;
ofxOscSender sender;

int current_msg_string;
string msg_strings[NUM_MSG_STRINGS];
float timers[NUM_MSG_STRINGS];

//========================================================================
//	isChange checks if the read byte has changed its bit values
//========================================================================
bool isChange(ofxDeviceNet & dNet, const int slaveID, const unsigned char checkByte) {

	unsigned char readBuffer[50]; //50 bytes because
	unsigned short dataLength = 50;

	dNet.readBytes(slaveID, readBuffer, &dataLength);

	auto nextByte = readBuffer[0];
	cout << int(nextByte) << endl;
	if (nextByte == checkByte) {
		cout << "ready" << endl;
		return true;
	}
	
	else{
		cout << "not ready" << endl;
		return false;
	}
	
}

struct Connection {
	ofxDeviceNet deviceNet;
	unsigned char deviceID;
};

//========================================================================
int main() {

	receiver.setup(PORT);
	cout << "listening for osc messages on port " << PORT << "\n";

	//get robots
	vector<unique_ptr<Connection>> connections;
	{
		ofxDeviceNet deviceNet;
		auto modules = deviceNet.getModuleList();		
		cout << modules.size() << " modules found" << endl;

		for (auto module : modules) {
			cout << "Setting up module #" << (int)module << endl;

			auto connection = make_unique<Connection>();
			connection->deviceNet.setup(module);

			auto devices = connection->deviceNet.listStoredDevices();
			if (devices.empty()) {
				ofLogError() << "No devices found for module #" << (int)module;
				continue;
			}

			unsigned char deviceID = 0;
			if (devices.size() != 1) {
				ofLogWarning() << "Too many devices (" << devices.size() << ") found for module #" << (int)module;
				for (auto device : devices) {
					cout << (int)device << ", ";
				}
				cout << endl;

				//search for 25 or 37
				for (auto device : devices) {
					if (device == 25) {
						deviceID = 25;
						break;
					} else if (device == 37) {
						deviceID = 37;
						break;
					}
				}
				if (deviceID == 0) {
					ofLogError() << "Couldn't find our device";
					std::exit(1);
				}
			}
			else {
				deviceID = devices.front();
			}

			connection->deviceNet.startDevice(deviceID);
			connection->deviceID = deviceID;
			connections.push_back(move(connection));

			cout << "Module #" << (int)module << " connected to device #" << (int)deviceID << endl;
		}
	}

	if (connections.empty()) {
		ofLogError() << "No connections";
		std::exit(1);
	}

	if (connections.size() != 2) {
		ofLogError() << "We need exactly 2 connections";
		std::exit(1);
	}

	for(;;) { // user breaks by hard quit

		//wait for incoming OSC message
		ofxOscMessage m;
		while (receiver.getNextMessage(m)) {

			//print the time
			{
				char buffer[100];
				auto rawTime = time(NULL);
				auto timeinfo = localtime(&rawTime);
				strftime(buffer, 100, "%c", timeinfo);
				cout << string(buffer) << endl;
			}
			
			cout << m.getAddress() << endl;

			if (m.getAddress() == "/startRobot") {
				int value = m.getArgAsInt(0);

				if (value == 1) {
					//raise play flag
					unsigned char startCommandRaise = 0x01; //0x01
					for (const auto & connection : connections) {
						cout << "Sending start command to device #" << (int) connection->deviceID << endl;
						connection->deviceNet.writeBytes(connection->deviceID, &startCommandRaise, 1);
					}

					this_thread::sleep_for(chrono::milliseconds(1000));

					//lower play flag
					unsigned char startCommandLower = 0x00;
					for (const auto & connection : connections) {
						connection->deviceNet.writeBytes(connection->deviceID, &startCommandLower, 1);
					}
				}
			}
		}

		this_thread::sleep_for(chrono::milliseconds(50));
	}
}

