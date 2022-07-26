#pragma once
#include <list>
#include "Graph.h"
#include "ChargeEvent.h"
#include "json.hpp"
using json = nlohmann::json;

struct Route {
	bool fail = false;
	float batteryConsumptionInkWh = 0.0;
	double lengthInMeters = 0.0;
	float travelTimeInSeconds = 0.0;
	float remainingChargeAtArrivalInkWh = 0.0;
	float totalChargingTimeInSeconds = 0.0;
	Graph g;
	vector<vector<unsigned>> route; // Array of arrays since each segment of the route to a charging stop is its own element
	vector<ChargeEvent*> chargeEvents;

	Route(Graph _g) : g{_g} {};

	json toJson() {
		json result;
		if (fail)
			result["fail"] = true;
		vector<json> legs;
		for (size_t idx = 0; idx < route.size(); ++idx) { // Iterate legs:
			auto leg = route[idx];
			json legjson;
			vector<json> points;
			for (auto edge : leg) { // Iterate edges of leg and insert "from" node
				json point;
				unsigned long node = g.tail[edge];
				point["latitude"] = g.graph.latitude[node];
				point["longitude"] = g.graph.longitude[node];
				points.emplace_back(point);
			}
			json lastPoint;
			unsigned long lastNode = g.graph.head[leg.back()];
			lastPoint["latitude"] = g.graph.latitude[lastNode];
			lastPoint["longitude"] = g.graph.longitude[lastNode];
			points.emplace_back(lastPoint); // Add target from last edge.
			legjson["points"] = points;
			if (idx < chargeEvents.size())
				legjson["summary"] = chargeEvents[idx]->toJson();
			legs.emplace_back(legjson);
		}
		result["legs"] = legs;
		result["summary"] = {
			{"batteryConsumptionInkWh", batteryConsumptionInkWh},
			{"lengthInMeters", static_cast<int>(lengthInMeters)},
			{"travelTimeInSeconds", static_cast<int>(travelTimeInSeconds)},
			{"remainingChargeAtArrivalInkWh", remainingChargeAtArrivalInkWh},
			{"totalChargingTimeInSeconds", static_cast<int>(totalChargingTimeInSeconds)}
		};
		return result;
	}
};
