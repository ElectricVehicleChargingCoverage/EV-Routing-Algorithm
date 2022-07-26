/**
 * @file ChargeEvent.h
 * @brief Defines the struct ChargeEvent that is used for one part of the result of a route.
 * Each ChargeEvent contains information about the traveled "leg" until the charger
 * as well as the required charging information.
 */
#pragma once
#include "ChargingPark.h"
#include "ChargingConnector.h"
#include "json.hpp"
using json = nlohmann::json;

struct ChargeEvent {
	ChargeEvent(ChargingPark* _park, ChargingConnector* _connector) : park{_park}, connector{_connector} {}
	ChargingPark* park;
	ChargingConnector* connector;
	int chargingTimeInSeconds = 0;
	float remainingChargeAtArrivalInkWh = 0.0;
	float targetChargeInkWh = 0.0;
	float lengthInMeters = 0.0;
	float travelTimeInSeconds = 0.0;
	float batteryConsumptionInkWh = 0.0;

	json toJson() {
		json result;
		result = {
			{"lengthInMeters", lengthInMeters},
			{"travelTimeInSeconds", travelTimeInSeconds},
			{"batteryConsumptionInkWh", batteryConsumptionInkWh}
		};
		result["chargingInformationAtEndOfLeg"] = {
			{"chargingTimeInSeconds", chargingTimeInSeconds},
			{"remainingChargeAtArrivalInkWh", remainingChargeAtArrivalInkWh},
			{"targetChargeInkWh", targetChargeInkWh},
			{"chargingConnectionInfo", connector->toJson()}
		};
		result["chargingInformationAtEndOfLeg"].merge_patch(park->toJson());
		result["chargingConnectionInfo"] = connector->toJson();
		return result;
	}
};