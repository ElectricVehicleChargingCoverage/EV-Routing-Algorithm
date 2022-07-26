/**
 * @file EvCar.h
 * @brief Defines an electric vehicle that is used for calculations.
 */
#pragma once

#include "ChargingConnector.h"
#include "StringUtil.h"
#include "json.hpp"
using json = nlohmann::json;

using namespace std;

struct EvCar {
	string car_model;
	float maxChargeInKwh;
	float currentChargeInKwh;
	double vehicleMaxSpeed = std::numeric_limits<float>::max();
	int chargingTimeOffsetInSec = 120; // Default 2 minutes
	float minChargeAtDestinationInkWh = 0.0;
	float minChargeAtChargingStopsInkWh = 0.0;
	int maxChargingTimeInSec = 20 * 60; // Default to 20 minutes
	bool hasChargingCurve = false;
	float weight = 0.0;
	std::vector<pair<int, float>> consumptionList = {};
	std::vector<pair<float, float>> chargingCurve = {};

	/**
	 * @brief Construct a new Ev Car object
	 * 
	 * @param _model string that contains the vehicle's name
	 * @param _maxChargeInKwh The max charge of the vehicle. This also sets currentChargeInKwh to 80 % of this value as well as minChargeAtChargingStopsInkWh to 10 % and minChargeAtDestinationInkWh to 20 %.
	 * @param consumptionCurve a string of the format "speedInKmh,consumptionInKWh:speedInKmh,consumptionInKWh:[...]", should be sorted ascending by speedInKmh.
	 */
	EvCar(string _model, float _maxChargeInKwh, string consumptionCurve)
		: car_model{_model}, maxChargeInKwh{_maxChargeInKwh} {
		currentChargeInKwh = 0.8 * _maxChargeInKwh;
		minChargeAtChargingStopsInkWh = 0.1 * _maxChargeInKwh;
		minChargeAtDestinationInkWh = 0.2 * _maxChargeInKwh;
		std::vector<std::string> constantSpeedConsumption = split(consumptionCurve, ":", false);
        for (string entry : constantSpeedConsumption) {
            std::vector<std::string> pair = split(entry, ",", false);
            consumptionList.push_back(make_pair(stoi(pair[0]), stof(pair[1])));
        }
	}

	void setChargingCurve(std::vector<pair<float, float>> curve) {
		chargingCurve = curve;
		hasChargingCurve = true;
        if (chargingCurve[0].first != 0.0) { // ensure that the first pair has "stateOfChargeInkWh" of 0
            std::pair<float, float> firstPair = make_pair(0.0, chargingCurve[0].second);
            chargingCurve.insert(chargingCurve.begin(), firstPair);
        }
	}

	size_t getConsumptionCurveIndex(float kmh) {
		size_t i = 0;
		for (; i < consumptionList.size() - 1; ++i) {
			if (consumptionList[i + 1].first >= kmh) return i;
		}
		return consumptionList.size() - 2;
	}

	/**
	 * @brief Calcualtes the consumption of the EV for a given edge.
	 * 
	 * @param edge The edge to drive along (with allowed max speed)
	 * @return the consumption for this edge in kWh, can be negative.
	 */
	float energyCost(float timeInSeconds, float lengthInMeters, float altitudeGainInKm = 0.0) {
		float velocity = speedInKmH(timeInSeconds, lengthInMeters);
		float lengthInKm = lengthInMeters/1000;
		float consumption = consumptionPerKmAtVelocity(velocity) * lengthInKm;
		if (altitudeGainInKm > 0) {
			consumption += altitudeGainInKm * vehicleConsumptionInKwhAltitudeGain();
		} else {
			consumption -= altitudeGainInKm * vehicleRecuperationInKwhAltitudeLoss();
		}
		return consumption;
	}

	float speedInKmH(float timeInSeconds, float lengthInMeters) {
		return (lengthInMeters / timeInSeconds) * 3.6;
	}

	float consumptionPerKmAtVelocity(float velocity) {
		size_t i = getConsumptionCurveIndex(velocity);
		auto [x, fx] = consumptionList[i]; auto [z, fz] = consumptionList[i + 1];
		float proportion = (velocity - x) / (z - x);
		float consumptionPerKm = (fx + proportion * (fz - fx)) / 100;
		return consumptionPerKm;
	}

	/**
	 * @brief Returns a factor for the additional consumption on uphill roads based on the vehicle's weight.
	 * 
	 * @return A factor that should be multiplied with the km of altitude change if road is uphill.
	 */
	float vehicleConsumptionInKwhAltitudeGain() {
		if (weight <= 0.0)
			return 0.0;
		return (weight * 9.81 * 1000.0 / 3600000.0) / 0.85;
	}

	/**
	 * @brief Returns a factor for the recuperation on downhill roads based on the vehicle's weight.
	 * 
	 * @return A factor that should be multiplied with the km of altitude change if road is downhill.
	 */
	float vehicleRecuperationInKwhAltitudeLoss() {
		if (weight <= 0.0)
			return 0.0;
		return (weight * 9.81 * 1000.0 / 3600000.0) * 0.9;
	}

	size_t getChargingCurveIndex(float soc) {
		for (size_t i = 0; i < chargingCurve.size() - 1; ++i)
			if (chargingCurve[i + 1].first > soc) return i;
		return chargingCurve.size() - 1;
	}

	/**
	 * @brief Get the Charging Speed at given soc as interpolated value of the given charging curve
	 * 
	 * @param soc The current state of charge of the vehicle
	 * @return float kW that the vehicle can charge with 
	 */
	float getChargingSpeed(float soc) {
		size_t index = getChargingCurveIndex(soc);
		if (index == chargingCurve.size() - 1)
			return chargingCurve[index].second;
		auto [xSoC, xPower] = chargingCurve[index];
		auto [zSoC, zPower] = chargingCurve[index + 1];
		float proportion = (soc - xSoC) / (zSoC - xSoC);
		float chargingSpeed = xPower + proportion * (zPower - xPower);
		return chargingSpeed;
	}

	/**
	 * @brief Calculates the SoC after charging a given time at a connector.
	 * 
	 * @param conn The connector to charge at
	 * @param soc The SoC at arrival
	 * @param chargingTimeInSeconds The charging time in seconds. This should be greater than the chargingOffsetInSec.
	 * @return SoC after the given time, capped at the maxChargeInKw.
	 */
	float chargeAfterTime(const ChargingConnector& conn, float soc, int chargingTimeInSeconds) {
		if (!hasChargingCurve)
			return -1.0;
		if (chargingTimeInSeconds > chargingTimeOffsetInSec)
			chargingTimeInSeconds -= chargingTimeOffsetInSec;
		else
			return soc;
		float vehicleChargingSpeed = getChargingSpeed(soc);
		while (chargingTimeInSeconds > 0 && soc < maxChargeInKwh) {
			int timeInSeconds = min(chargingTimeInSeconds, 30); // this means: update charging speed after 30s
			float speed = min(conn.ratedPowerKw, vehicleChargingSpeed);
			soc += (static_cast<float>(timeInSeconds) / 3600) * speed;
			chargingTimeInSeconds -= timeInSeconds;
			vehicleChargingSpeed = getChargingSpeed(soc);
		}
		return soc;
	}

	/**
	 * Calculate the time (in seconds) to charge at a given connector to a specific value.
	 *
	 * @param conn The ChargingConnector to charge at.
	 * @param soc The state of charge to start charging
	 * @param goal_soc The state of charge to charge to.
	 * @return Time in seconds to reach the demanded charge.
	 */
	int time_needed(const ChargingConnector& conn, float soc, float goal_soc) {
		if (!hasChargingCurve)
			return -1;
		goal_soc = min(goal_soc, maxChargeInKwh);
		if (soc > goal_soc) return 0;
		int chargingTimeInSeconds = chargingTimeOffsetInSec;
		float vehicleChargingSpeed = getChargingSpeed(soc);
		while (soc < goal_soc) {
			int timeInSeconds = 30; // this means: update charging speed after 30s
			float speed = min(conn.ratedPowerKw, vehicleChargingSpeed);
			if (speed <= 0)
				return -1;
			soc += (static_cast<float>(timeInSeconds) / 3600) * speed;
			chargingTimeInSeconds += timeInSeconds;
			vehicleChargingSpeed = getChargingSpeed(soc);
		}
		return chargingTimeInSeconds;
	}

	/**
	 * @brief Returns the remaining soc after the edge is driven (at speed limit velocity)
	 * 
	 * @param soc State of charge before edge
	 * @param e The edge to drive
	 * @return Remaining state of charge after edge, might be negative. 
	 */
	float socAfterEdge(float soc, float timeInSeconds, float lengthInMeters, float altitudeGainInKm = 0.0) {
		return min(maxChargeInKwh, soc - energyCost(timeInSeconds, lengthInMeters, altitudeGainInKm));
	}
};
