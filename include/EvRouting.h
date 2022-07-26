/**
 * A simple EV-Routing algorithm:
 * Very simple assumptions and some ideas on how they can be optimized.
 */
#pragma once

#include "RoutingResult.h"
#include "ChargeEvent.h"
#include "ChargingPark.h"
#include "Graph.h"
#include "Point.h"
#include <set>
#include "EvCar.h"
using namespace std;

#define BACKTRACE_START_PCT 0.25
#define BACKTRACE_END_KW 200

class EvRouting {
private:
	EvCar& car;
	Graph g;
public:
	EvRouting(EvCar& _car, Graph _graph) : car{_car}, g{_graph} {
	}

	/**
	 * @brief Returns the combined time of driving from the start to this park and from this park to the target.
	 * 
	 * @param park The charging park
	 * @param source The start of the route
	 * @param target The destination of the route
	 * @return float the required travel time via the charging park (without charging time). Retunrs inf, if the park is not reachable with the given minChargeAtChargingStops of the vehicle. 
	 */
	float rateChargingPark(ChargingPark* park, unsigned long source, unsigned long target) {
		auto firstPart = calculateDistances(source, park->node);
		if (firstPart.first < car.minChargeAtChargingStopsInkWh) // We don't need to consider this park even further!
			return std::numeric_limits<float>::max();
		return firstPart.second + calculateDistances(park->node, target).second;
	}

	/**
	 * For a node, check the surrounding charging parks and return the best one for the given position.
	 * @param location The node to search the charging parks around
	 * @param source_id The start of the trip (osm_id)
	 * @param target_it The destination of the trip (osm_id)
	 * @param currentBestKw The charging power of the currently best charger
	 * @return Pair of ChargingPark* and score of charging park.
	 */
	pair<ChargingPark*, double> getBestChargingPark(Point* location, unsigned long source_id, unsigned long target_id, set<ChargingPark*>* blacklist, float currentBestKw = 0.0) {
		// Get 10 nearest chargers and filter out all that are too far away (10 km)
		vector<ChargingPark*> stations = g.findKNearestChargers(location, 10, 10);
		vector<ChargingPark*> bestStations = {};
		float bestChargingPower = currentBestKw;
		for (ChargingPark* park : stations) {
			if (blacklist->find(park) != blacklist->end()) // if Charger is already on the blacklist -> Skip it
				continue;
			float ratedPower = park->getBestConnFor(car)->ratedPowerKw;
			if (ratedPower < bestChargingPower) {
				blacklist->insert(park);
				continue;
			}
			if (bestStations.empty()) {
				bestStations.push_back(park);
				bestChargingPower = ratedPower;
				continue;
			}
			if (ratedPower > bestChargingPower) {
				for (auto station : bestStations) // All parks in the current list can be skipped in the future.
					blacklist->insert(station);
				bestStations.clear();
				bestStations.push_back(park);
				bestChargingPower = ratedPower;
			} else if (ratedPower == bestChargingPower) {
				bestStations.push_back(park);
			}
		}
		if (bestStations.size() == 0)
			return make_pair(nullptr, std::numeric_limits<float>::max());
		ChargingPark* best = bestStations[0];
		double best_score = rateChargingPark(best, source_id, target_id);
		if (bestStations.size() == 1)
			return make_pair(best, best_score);
		// Rate all charging stations and select the station with the lowest score.
		for (auto station : bestStations) {
			double score = rateChargingPark(station, source_id, target_id);
			if (score < best_score) {
				best_score = score;
				best = station;
			}
		}
		blacklist->insert(best); // We can add the best to the blacklist so we don't find it in the future.
		return make_pair(best, best_score);
	}

	/**
	 * @brief Returns the remaining charge at the destination and distance in km. SoC can be negative.
	 * 
	 * @param from the id of the source node 
	 * @param to the id of the target node
	 * @return pair<float, float> result.first is the remaining SoC, result.second is the time in seconds. 
	 */
	pair<float, float> calculateDistances(unsigned long from, unsigned long to) {
		ContractionHierarchyQuery ch_query(g.ch);
		ch_query.reset().add_source(from).add_target(to).run();
		vector<unsigned> edges = ch_query.get_arc_path();
		vector<float> soc = { car.currentChargeInKwh };
		double distanceInMeters = 0.0;
		double timeInSeconds = 0.0;
		for (auto edge : edges) {
			float distance = g.distanceInMeter(edge);
			float time = g.travelTimeInSec(edge);
            timeInSeconds += time;
            distanceInMeters += distance;
			soc.push_back(car.socAfterEdge(soc[soc.size() - 1], time, distance));
        }
		return make_pair(soc[soc.size() - 1], timeInSeconds);
	}

	/**
	 * Calculate a route for an electric vehicle
	 *
	 * @param source_id: The id of the source node.
	 * @param target_id: The id of the target node.
	 * @return The route to drive.
	 */
	Route* calculateRoute(unsigned long source_id, unsigned long target_id) {
		auto start_time = chrono::high_resolution_clock::now();
		cout << "Calculating route..." << endl;
		Route* evRoute = new Route(g);

		while (source_id != target_id) { // Start an iterative search for the route
			ContractionHierarchyQuery ch_query(g.ch);
			ch_query.reset().add_source(source_id).add_target(target_id).run(); // Calculate the complete route
			vector<unsigned> edges = ch_query.get_arc_path();
			vector<float> soc = { car.currentChargeInKwh };
			// Define variables for later use
			float lengthInMeters = 0.0;
			float travelTimeInSeconds = 0.0;
			float socAtStart = car.currentChargeInKwh;
			set<ChargingPark*> blacklist;
			pair<ChargingPark*, float> bestPark = make_pair(nullptr, std::numeric_limits<float>::max()); // this stores our current optimal charger
			// Compute remaining SoC after each edge on the path
			for (auto edge : edges) {
				float distance = g.distanceInMeter(edge);
				float time = g.travelTimeInSec(edge);
				soc.push_back(car.socAfterEdge(soc[soc.size() - 1], time, distance));
			}
			// Check if the destination can be reached
			if (soc[(soc.size() - 1)] >= car.minChargeAtDestinationInkWh) { // The destination can be reached with the current charge
				evRoute->route.push_back(edges);
				for (auto edge : edges) {
					evRoute->lengthInMeters += g.distanceInMeter(edge);
					evRoute->travelTimeInSeconds += g.travelTimeInSec(edge);
				}
				evRoute->batteryConsumptionInkWh += socAtStart - soc[soc.size()-1];
				evRoute->remainingChargeAtArrivalInkWh = soc[soc.size()-1];
				source_id = target_id; // This will result in a termination of the loop.
				continue; 
			}
			// In case the destination cannot be reached, a charger must be found:
			int i; // Go to the point on the route where the vehicle has at least minChargeAtChargingStopsInkWh remaining. This point might be the destination!
			for (i = 0; i < soc.size(); ++i)
				if (soc[i] <= car.minChargeAtChargingStopsInkWh || i == soc.size() - 1)
					break;
			while (i >= 0 && (bestPark.first == nullptr || soc[i] < BACKTRACE_START_PCT * car.maxChargeInKwh)) {
				unsigned edge = edges[i];
				auto fromNode = g.tail[edge];
				Point coordinates = Point(g.graph.latitude[fromNode], g.graph.longitude[fromNode]);
				pair<ChargingPark*, double> parkCandidate = getBestChargingPark(&coordinates, source_id, target_id, &blacklist, bestPark.first == nullptr ? 0 : bestPark.first->getBestConnFor(car)->ratedPowerKw);
				if (bestPark.first == nullptr) {
					bestPark = parkCandidate; // If no charger has been found yet, the candidate ist the new best.
				} else {
					float bestKw = bestPark.first->getBestConnFor(car)->ratedPowerKw;
					float candidateKw = parkCandidate.first->getBestConnFor(car)->ratedPowerKw;
					if(bestKw < candidateKw || (bestKw == candidateKw && bestPark.second > parkCandidate.second))
						bestPark = parkCandidate;
                }
				if (bestPark.first->getBestConnFor(car)->ratedPowerKw > BACKTRACE_END_KW)
					break;
				i--;
			}
			if (bestPark.first == nullptr) { // can't find a charger -> route fails
				evRoute->fail = true;
				return evRoute;
			}
			// Drive from start to charging park:
			ch_query.reset().add_source(source_id).add_target(bestPark.first->node).run(); // calculate route from start to charging park
			edges = ch_query.get_arc_path();
			for (auto edge : edges) { // save route from Start to ChargingPark as a leg in the result
				float distance = g.distanceInMeter(edge);
				float time = g.travelTimeInSec(edge);
				lengthInMeters += distance;
				travelTimeInSeconds += time;
				car.currentChargeInKwh = car.socAfterEdge(car.currentChargeInKwh, time, distance);
			}
			evRoute->route.push_back(edges);
			evRoute->lengthInMeters += lengthInMeters;
			evRoute->travelTimeInSeconds += travelTimeInSeconds;
			// Charge at the charging station:
			// Charge to 80 % but at most car.maxChargingTimeInSec seconds:
			ChargingConnector* conn = bestPark.first->getBestConnFor(car);
			ChargeEvent* chargeEvent = new ChargeEvent(bestPark.first, conn);
			float targetChargeInkWh = car.maxChargeInKwh * 0.8;
			chargeEvent->remainingChargeAtArrivalInkWh = car.currentChargeInKwh;
			int chargingTime = car.time_needed(*conn, car.currentChargeInKwh, targetChargeInkWh);
			// Check if required charging time exceeds charging time limit of the vehicle.
			if (chargingTime > car.maxChargingTimeInSec) {
				chargingTime = car.maxChargingTimeInSec;
				targetChargeInkWh = car.chargeAfterTime(*conn, car.currentChargeInKwh, car.maxChargingTimeInSec);
			}
			chargeEvent->targetChargeInkWh = targetChargeInkWh;
			chargeEvent->chargingTimeInSeconds = chargingTime;
			// Add time to route result:
			evRoute->totalChargingTimeInSeconds += chargeEvent->chargingTimeInSeconds;
			evRoute->travelTimeInSeconds += chargeEvent->chargingTimeInSeconds;
			// add summary info to charge event.
			chargeEvent->travelTimeInSeconds = travelTimeInSeconds;
			chargeEvent->lengthInMeters = lengthInMeters;
			chargeEvent->batteryConsumptionInkWh = socAtStart - car.currentChargeInKwh;
			evRoute->batteryConsumptionInkWh += socAtStart - car.currentChargeInKwh;
			car.currentChargeInKwh = targetChargeInkWh;
			evRoute->chargeEvents.emplace_back(chargeEvent);
			// Start journey from ChargingPark to destination in next iteration.
			source_id = bestPark.first->node;
		}
		auto finish_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(finish_time - start_time);
        cout << "Calculating route took " << duration.count() << " ms." << endl;
		return evRoute;
	}
};