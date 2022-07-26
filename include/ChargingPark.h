/**
 * @file ChargingPark.h
 * @brief Defines a ChargingPark that is used in the routing.
 * Offers a method that can be implemented to specify what charging connectors can be used with the vehicle.
 * Currently assumes that each vehicle can charge at each connector.
 */
#pragma once
#include "ChargingConnector.h"
#include "EvCar.h"
#include "Point.h"
#include "json.hpp"
using json = nlohmann::json;

using namespace std;

struct ChargingPark {
	ChargingPark(long long _id, string _name, Point* _location) {
		id = _id;
		name = _name;
		location = _location;
	};
	string name;
	long long id;
	vector<ChargingConnector*> connectors;
	Point* location;
	unsigned long node;

	/**
	 * @brief Get the best Charging Connector.
	 * Adapt this method if you want to limit the vehicle to certain connection types!
	 * 
	 * @param car The vehicle to charge (CURRENTLY NOT USED!!!)
	 * @return ChargingConnector* with the highest ratedPowerKw for the vehicle.
	 */
	ChargingConnector* getBestConnFor(EvCar car) {
		return getBestConnector().first;
	}

	/**
	 * @brief Get the highest ratedPowerKw of all connectors.
	 * 
	 * @return highest ratedPowerKw 
	 */
	double getBestPower() {
		return getBestConnector().second;
	}

	/**
	 * @brief Get the best connector and its highest ratedPowerKw.
	 * 
	 * @return best connector and highest ratedPowerKw 
	 */
	pair<ChargingConnector*, float> getBestConnector() {
		pair<ChargingConnector*, float> best = make_pair(connectors[0], connectors[0]->ratedPowerKw);
		for (auto conn : connectors)
			if (conn->ratedPowerKw > best.second)
				best = make_pair(conn, conn->ratedPowerKw);
		return best;
	}

	json toJson() {
		json result;
		result["chargingParkName"] = name;
		result["chargingParkExternalId"] = id;
		result["chargingParkLocation"] = location->toJson();
		result["ratedPowerKw"] = getBestPower();
		for (ChargingConnector* conn : connectors) {
			result["connectors"] += conn->toJson();
		}
		return result;
	}
};