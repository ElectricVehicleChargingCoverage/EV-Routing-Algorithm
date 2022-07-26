/**
 * @file ChargingConnector.h
 * @brief Defines the struct ChargingConnector that is associated with a charging station.
 */
#pragma once

#include "json.hpp"
using json = nlohmann::json;
using namespace std;


struct ChargingConnector {
	string connectorType;
	float ratedPowerKw;
	string currentType;
	ChargingConnector(string _connectorType, float _ratedPowerKw, string _currentType) : connectorType{ _connectorType }, ratedPowerKw{ _ratedPowerKw }, currentType{ _currentType} {}

	json toJson() {
		json result = {
			{"connectorType", connectorType},
			{"ratedPowerKW", ratedPowerKw},
			{"currentType", currentType}
		};
		return result;
	}
};