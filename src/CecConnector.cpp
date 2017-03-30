#include "CecConnector.h"

using namespace CEC;

void CecConnector::cecLogMessageCallback(void*, const cec_log_message* message)
{
	// Detect an explicit power report message
	if ((message->message[6] == '9' && message->message[7] == '0') ||
		 (message->message[6] == '7' && message->message[7] == '2'))
		CecConnector::instance().onPowerStateReport(message);

	// Detect when the TV seems to wake up (might come much earlier to the dedicated power on message
	// Here, the language set broadcast is being detected
	if (message->message[3] == '0' && message->message[4] == 'f' &&
		 message->message[6] == '3' && message->message[7] == '2')
	{
		if (CecConnector::instance().tvPowerState_ != cec_power_status::CEC_POWER_STATUS_ON)
			CecConnector::instance().tvIsBeingTurnedOnHeuristic_ = true;
	}


	std::string level;
	switch (message->level)
	{
		case CEC_LOG_ERROR:   level = "CEC MONITOR ERROR:   "; break;
		case CEC_LOG_WARNING: level = "CEC MONITOR WARNING: "; break;
		case CEC_LOG_NOTICE:  level = "CEC MONITOR NOTICE:  "; break;
		case CEC_LOG_TRAFFIC: level = "CEC MONITOR TRAFFIC: "; break;
		case CEC_LOG_DEBUG:   level = "CEC MONITOR DEBUG:   "; break;
		default: break;
	}

	std::cout << level << message->message << std::endl;
}

void CecConnector::cecAlertCallback(void*, const libcec_alert type, const libcec_parameter)
{
	switch (type)
	{
		case CEC_ALERT_CONNECTION_LOST:
			CecConnector::instance().reconnect();
			break;
		default:
			break;
	}
}

CecConnector& CecConnector::instance()
{
	static CecConnector c;
	return c;
}

CecConnector::CecConnector()
{
	callbacks_.Clear();
	callbacks_.logMessage	= CecConnector::cecLogMessageCallback;
	callbacks_.alert		= CecConnector::cecAlertCallback;

	config_.Clear();
	snprintf(config_.strDeviceName, 13, "%s", osdName_.c_str());
	config_.clientVersion		= LIBCEC_VERSION_CURRENT;
	config_.bActivateSource		= 0;
	config_.callbacks				= &callbacks_;
	config_.deviceTypes.Add(CEC_DEVICE_TYPE_RECORDING_DEVICE);

	cecAdapter_ = CECInitialise(&config_);

	if (!cecAdapter_)
		throw std::logic_error{"Could not initialize CEC library."};

	cecAdapter_->InitVideoStandalone();

	std::cout << "CEC library initialized" << std::endl;

	autoDetectPort();

	std::cout << "Connecting to the CEC adapter..." << std::endl;

	if (!cecAdapter_->Open(port_.c_str()))
		throw std::logic_error{"Could not connect to the device."};

	// Need this in order to detect audio systems
	cecAdapter_->RescanActiveDevices();

	// Need this in order to speed up future calls of scanForDevices(). Not sure why the call above is not enough.
	scanForDevices();

	// Monitor device state
	monitorTimer_.setInterval(1000);
	QObject::connect(&monitorTimer_, &QTimer::timeout, [this](){onMonitorTimer();});
	monitorTimer_.start();
}

CecConnector::~CecConnector()
{
	if (cecAdapter_) {
		cecAdapter_->Close();
		CECDestroy(cecAdapter_);
		cecAdapter_ = nullptr;
	}
}

void CecConnector::autoDetectPort()
{
	std::cout << "Autodetecting port: ";
	cec_adapter_descriptor devices[10];
	uint8_t iDevicesFound = cecAdapter_->DetectAdapters(devices, 10, nullptr, true);
	if (iDevicesFound <= 0)
		throw std::logic_error{"No CEC devices found"};
	else
	{
		std::cout << std::endl << " path:     " << devices[0].strComPath << std::endl <<
		" com port: " << devices[0].strComName << std::endl << std::endl;
		port_ = devices[0].strComName;
	}
}

void CecConnector::onMonitorTimer()
{
	std::cout << "CEC MONITOR HEARTBEAT:"
				 << "  tv:" << cecAdapter_->ToString(tvPowerState_)
				 << "  audio:" << cecAdapter_->ToString(audioPowerState_)
				 << "  tv_turning_on_heuristic:" << (tvIsBeingTurnedOnHeuristic_ ? "yes" : "no")
				 << std::endl;

	if (!tvIsBeingTurnedOnHeuristic_ && tvPowerState_ == cec_power_status::CEC_POWER_STATUS_UNKNOWN)
	{
		cecAdapter_->GetDevicePowerStatus(cec_logical_address::CECDEVICE_TV);
		return;
	}

	if (tvPowerState_ == cec_power_status::CEC_POWER_STATUS_ON ||
		 tvPowerState_ == cec_power_status::CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON ||
		 tvIsBeingTurnedOnHeuristic_)
	{
		if (audioPowerState_ == cec_power_status::CEC_POWER_STATUS_UNKNOWN)
		{
			cecAdapter_->GetDevicePowerStatus(cec_logical_address::CECDEVICE_AUDIOSYSTEM);
			return;
		}

		if (audioPowerState_ ==  cec_power_status::CEC_POWER_STATUS_STANDBY)
			cecAdapter_->PowerOnDevices(cec_logical_address::CECDEVICE_AUDIOSYSTEM);
	}
}

void CecConnector::onPowerStateReport(const CEC::cec_log_message* message)
{
	cec_power_status powerState = cec_power_status::CEC_POWER_STATUS_UNKNOWN;

	if (message->message[6] == '9' && message->message[7] == '0')
	{
		if (message->message[10] == '0') powerState = cec_power_status::CEC_POWER_STATUS_ON;
		else if (message->message[10] == '1') powerState = cec_power_status::CEC_POWER_STATUS_STANDBY;
		else if (message->message[10] == '2') powerState = cec_power_status::CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON;
		else if (message->message[10] == '3') powerState = cec_power_status::CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY;
	}
	else if (message->message[6] == '7' && message->message[7] == '2')
	{
		if (message->message[10] == '0') powerState = cec_power_status::CEC_POWER_STATUS_STANDBY;
		else if (message->message[10] == '1') powerState = cec_power_status::CEC_POWER_STATUS_ON;
	}

	int sourceLogicalAddress = message->message[3] - '0';
	if (sourceLogicalAddress == cec_logical_address::CECDEVICE_TV)
	{
		if (tvPowerState_ != powerState) tvIsBeingTurnedOnHeuristic_ = false;
		tvPowerState_ = powerState;
	}
	else if (sourceLogicalAddress == cec_logical_address::CECDEVICE_AUDIOSYSTEM)
		audioPowerState_ = powerState;
}

bool CecConnector::reconnect() const
{
	if (cecAdapter_)
	{
		cecAdapter_->Close();
		if (cecAdapter_->Open(port_.c_str())) return true;

		std::cout << "Failed to reconnect" << std::endl;
	}

	return false;
}

void CecConnector::turnOn(cec_logical_address address) const
{
	cecAdapter_->PowerOnDevices(address);
}

void CecConnector::turnOff(cec_logical_address address) const
{
	cecAdapter_->StandbyDevices(address);
}

void CecConnector::activateDevice(std::string name) const
{
	auto deviceIt = devicesOnCecBus_.find(name);
	auto thisDeviceIt =  devicesOnCecBus_.find(osdName_);

	if (deviceIt == devicesOnCecBus_.end() || thisDeviceIt == devicesOnCecBus_.end())
		return;

	cec_command command;

	command.initiator = thisDeviceIt->second.logicalAddress_;
	command.destination = cec_logical_address::CECDEVICE_BROADCAST;

	command.opcode_set = 1;
	command.opcode = cec_opcode::CEC_OPCODE_ACTIVE_SOURCE;

	command.parameters.size = 2;
	command.parameters.data[0] = ((deviceIt->second.physicalAddress_ >> 8) & 0xFF);
	command.parameters.data[1] = (deviceIt->second.physicalAddress_ & 0xFF);

	command.transmit_timeout = 0;
	command.ack = 0;
	command.eom = 0;

	cecAdapter_->Transmit(command);
}

int CecConnector::volumeUp() const
{
	return cecAdapter_->VolumeUp();
}

int CecConnector::volumeDown() const
{
	return cecAdapter_->VolumeDown();
}

void CecConnector::toggleMute() const
{
	cecAdapter_->AudioToggleMute();
}

void CecConnector::enableCecBusMonitoring(bool enable) const
{
	cecAdapter_->SwitchMonitoring(enable);
}

bool CecConnector::isDeviceOn(cec_logical_address address) const
{
	return cecAdapter_->GetDevicePowerStatus(address) == CEC_POWER_STATUS_ON;
}

bool CecConnector::isDeviceActive(cec_logical_address address) const
{
  return cecAdapter_->IsActiveDevice(address);
}

std::vector<std::string> CecConnector::scanForDevices()
{
	std::map<std::string, CecDevice> discoveredDevices;

	auto addresses = cecAdapter_->GetActiveDevices();
	std::vector<std::string> result;

	for (uint8_t logicalAddress = 0; logicalAddress < 16; logicalAddress++)
	{
		if (addresses.addresses[logicalAddress])
		{
			CecDevice device;
			device.logicalAddress_ = (cec_logical_address) logicalAddress;
			device.physicalAddress_ = cecAdapter_->GetDevicePhysicalAddress(device.logicalAddress_);
			device.name_ = cecAdapter_->GetDeviceOSDName(device.logicalAddress_);
			result.push_back(device.name_);

			std::cout << "Detected device: " << device.name_ << " : " << cecAdapter_->ToString(device.logicalAddress_)
						 << " PHY: " << ((device.physicalAddress_ >> 12) & 0xF) << '.'
								<< ((device.physicalAddress_ >> 8) & 0xF) << '.'
								<< ((device.physicalAddress_ >> 4) & 0xF) << '.'
								<< ((device.physicalAddress_) & 0xF) << std::endl;

			discoveredDevices[device.name_] = device;
		}
	}

	devicesOnCecBus_ = discoveredDevices;

	return result;
}
