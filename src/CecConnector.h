#pragma once
#include "precompiled.h"

struct CecDevice {
		CEC::cec_logical_address logicalAddress_;
		uint16_t physicalAddress_;
		std::string name_;
};

class CecConnector
{
	public:
		static CecConnector& instance();

		bool reconnect() const;

		void turnOn(CEC::cec_logical_address address) const;
		void turnOff(CEC::cec_logical_address address) const;
		bool isDeviceOn(CEC::cec_logical_address address) const;

		void activateDevice(std::string name) const;
		int volumeUp() const;
		int volumeDown() const;
		void toggleMute() const;
		std::vector<std::string> scanForDevices();

		void enableCecBusMonitoring(bool enable) const;

		bool isDeviceActive(CEC::cec_logical_address address) const;

	private:
		CEC::ICECAdapter* cecAdapter_{};
		std::string port_{};
		std::string osdName_{"HTPC"};

		CEC::ICECCallbacks callbacks_{};
		CEC::libcec_configuration config_{};

		QTimer monitorTimer_{};
		CEC::cec_power_status tvPowerState_{CEC::cec_power_status::CEC_POWER_STATUS_UNKNOWN};
		CEC::cec_power_status audioPowerState_{CEC::cec_power_status::CEC_POWER_STATUS_UNKNOWN};
		bool tvIsBeingTurnedOnHeuristic_{};

		std::map<std::string, CecDevice> devicesOnCecBus_;

		CecConnector();
		~CecConnector();

		void autoDetectPort();
		void onMonitorTimer();

		void onPowerStateReport(const CEC::cec_log_message* message);

		static void cecLogMessageCallback(void*, const CEC::cec_log_message* message);
		static void cecAlertCallback(void*, const CEC::libcec_alert type, const CEC::libcec_parameter);
};
