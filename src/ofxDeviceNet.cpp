#include "ofxDeviceNet.h"

using namespace std;

ofxDeviceNet::ofxDeviceNet()
{

}

ofxDeviceNet::~ofxDeviceNet()
{
	close(); //close module connection
}

void ofxDeviceNet::listModules() {
	auto moduleList = this->getModuleList();

	std::cout << "Number of Modules:" << int(moduleList.size()) << std::endl;
	for (auto module : moduleList) {
		std::cout << "Module #" << int(module) << std::endl;
	}
}

std::vector<unsigned char> ofxDeviceNet::getModuleList()
{
	DWORD err;
	BYTE moduleCount = 0;
	BYTE moduleList[100];

	//sleep before starting
	this_thread::sleep_for(std::chrono::milliseconds(100));

	err = I7565DNM_TotalI7565DNMModule(&moduleCount, moduleList);
	DNM_CHECK_FATAL_ERROR;

	if (moduleCount == 0) {
		return std::vector<unsigned char>();
	}

	auto moduleListVector = std::vector<unsigned char>((size_t) moduleCount);
	memcpy(&moduleListVector[0], moduleList, moduleCount);
	return moduleListVector;
}

bool ofxDeviceNet::setup()
{
	auto portVector = getModuleList();
	if (!portVector.empty()) {
		return setup(portVector[0]);
	} 
	return false; 
}

bool ofxDeviceNet::setup(int portNumber)
{
	DWORD err;

	this->portNum = portNumber;
	err = I7565DNM_ActiveModule(portNumber);

	if (err != DNMXS_NoError) {
		std::cout << "Error Activating Module, Error code#" << err << std::endl;
		return false;
	}
	return true;
	
}

void ofxDeviceNet::close()
{
	DWORD err;
	err = I7565DNM_CloseModule(portNum);
	DNM_CHECK_ERROR;
}

std::vector <unsigned char> ofxDeviceNet::searchAllDevices()
{
	DWORD err;

	if (portNum == 0xFF) {
		std::cout << "Setup module before searching for slave devices" << std::endl;
		return std::vector<unsigned char>();
	}

	std::cout << "Searching for Devices";
	err = I7565DNM_SearchAllDevices(this->portNum);
	DNM_CHECK_ERROR;

	//wait whilst searching
	while (I7565DNM_IsSearchOK(this->portNum)) {
		std::cout << ".";
		std::this_thread::sleep_for(std::chrono::milliseconds(300)); //sleep
	}
	std::cout << std::endl;

	//traverse the list
	{
		WORD TotalDevices = 0;
		BYTE DesMACID[64], Type[64];
		WORD DeviceInputLen[64], DeviceOutputLen[64];
		err = I7565DNM_GetSearchedDevices(this->portNum, &TotalDevices, DesMACID, Type, DeviceInputLen, DeviceOutputLen);
		DNM_CHECK_ERROR;

		if (TotalDevices != 0) {
			cout << "Found " << (int)TotalDevices << " devices" << endl;
			for (BYTE i = 0; i < TotalDevices; i++) {
				cout << "Slave device mac ID: " << DesMACID[i] << std::endl;
				cout << "Type: " << (int)Type[i] << std::endl;
			}
			vector<unsigned char> deviceIDs((size_t)TotalDevices);
			memcpy(&deviceIDs[0], DesMACID, TotalDevices);
			return deviceIDs;
		}
		else {
			std::cout << "No slave devices found" << std::endl;
			return std::vector<unsigned char>();
		}
	}
}

std::vector <unsigned char> ofxDeviceNet::listStoredDevices() {
	WORD TotalDevices = 0;
	BYTE DeviceList[64];
	BYTE Type[64];
	WORD InputListLength[64];
	WORD OutputListLength[64];
	WORD EPRList[64];

	auto err = I7565DNM_GetScanList(this->portNum
		, & TotalDevices
		, DeviceList
		, Type
		, InputListLength
		, OutputListLength
		, EPRList);
	DNM_CHECK_ERROR;

	vector<unsigned char> result;
	for (int i = 0; i < TotalDevices; i++) {
		result.push_back(DeviceList[i]);
	}
	return result;
}

void ofxDeviceNet::getBaudRate()
{
}

void ofxDeviceNet::addConnection(unsigned char deviceMacID,int inByteLen, int outByteLen, int pollRateMS)
{
	DWORD err;
	if (portNum == 0xFF) {
		std::cout << "Setup module before connecting to slave device" << std::endl;
		return;
	}

	err = I7565DNM_AddIOConnection(portNum, deviceMacID, ConType_Poll, inByteLen, outByteLen, pollRateMS);
	if (err) {
		printf("Add IO Connection Error Code:%d\n", err);
	}
		

}

void ofxDeviceNet::removeConnection(unsigned char deviceMacID)
{
	DWORD err;
	if (portNum == 0xFF) {
		std::cout << "Setup module before disconnecting from slave device" << std::endl;
		return;
	}

	err = I7565DNM_RemoveIOConnection(portNum, deviceMacID, ConType_Poll);
	if (err) {
		printf("Error Code:%d\n", err);
	}
}

void ofxDeviceNet::startDevice(unsigned char deviceMacID)
{
	DWORD err;
	if (portNum == 0xFF) {
		std::cout << "Setup module before starting slave device" << std::endl;
		return;
	}

	//std::cout << portNum << std::endl;
	err = I7565DNM_StartDevice(portNum, deviceMacID);
	DNM_CHECK_ERROR;
}

void ofxDeviceNet::stopDevice(unsigned char deviceMacID)
{
	DWORD err;
	if (portNum == 0xFF) {
		std::cout << "Setup module before stopping slave device" << std::endl;
		return;
	}

	err= I7565DNM_StopDevice(portNum, deviceMacID);
	DNM_CHECK_ERROR;
}

bool ofxDeviceNet::readBytes(unsigned char deviceMacID, unsigned char * buffer, unsigned short *length)
{
	DWORD err;
	
	if (portNum == 0xFF) {
		std::cout << "setup module before reading bytes" << std::endl;
		return false;
	}

	err = I7565DNM_ReadInputData(portNum, deviceMacID, ConType_Poll, length, buffer);
	if (err == I7565DNM_NoError) {
		return true;
	}
	else {
		printf("Error Reading Input Data, Error Code:%d\n", err);
		return false;
	}
	
}

bool ofxDeviceNet::writeBytes(unsigned char deviceMacID, unsigned char * buffer, unsigned short length)
{
	DWORD err;


	if (portNum == 0xFF) {
		std::cout << "setup module before writing bytes" << std::endl;
		return false;
	}

	err = I7565DNM_WriteOutputData(portNum, deviceMacID, ConType_Poll, length, buffer);
	if (err == I7565DNM_NoError) {
		return true;
	}
	else {
		std::cout << "Error Writing Output Data, Error code#" << int(err) << std::endl;
		return false;
	}
}
