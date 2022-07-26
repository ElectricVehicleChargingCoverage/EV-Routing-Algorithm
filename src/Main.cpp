#include <iostream>
#include <chrono>
#include <cstdio>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include "Graph.h"
#include "StringUtil.h"
#include "EvRouting.h"
#include "json.hpp"

#include <routingkit/osm_simple.h>
#include <routingkit/contraction_hierarchy.h>
#include <routingkit/inverse_vector.h>
#include <routingkit/timer.h>
#include <routingkit/geo_position_to_node.h>
#include <iostream>

using json = nlohmann::json;
using namespace RoutingKit;
using namespace std;

Graph g;

void writeToFile(json result) {
    std::ofstream file("output.json");
    file << result;
    file.close();
}

void calculateExampleRoute() {
    // Coordinates for the route
    double from_lat = 52.39385;
    double from_lon = 13.12964;
    double to_lat = 48.78128;
    double to_lon = 9.18676;

    // Map the coordinates to the nodes of the graph
    GeoPositionToNode map_geo_position(g.graph.latitude, g.graph.longitude);
    unsigned from = map_geo_position.find_nearest_neighbor_within_radius(from_lat, from_lon, 1000).id;
    unsigned to = map_geo_position.find_nearest_neighbor_within_radius(to_lat, to_lon, 1000).id;

    // Define the electric vehicle
    EvCar car = EvCar("Tesla Model 3 LR", 70.0, "10,10.7:50,10.7:80,13.3:120,16.3");
    car.weight = 2019;
    car.setChargingCurve({{ 7.0, 190 }, { 7.7, 187 }, {8.4, 182}, {9.1, 175}, {9.8, 170}, {10.5, 166}, {11.2, 162}, {11.9, 159}, {12.6, 156}, {14.0, 150}, {14.7, 148}, {15.4, 147}, {16.1, 145}, {16.8, 144}, {18.9, 138}, {19.6, 136}, {21.7, 131}, {22.4, 128}, {23.1, 126}, {23.8, 124}, {24.5, 122}, {25.2, 120}, {25.9, 118}, {26.6, 116}, {28.7, 109}, {30.8, 102}, {31.5, 99}, {32.2, 97}, {32.9, 95}, {33.6, 92}, {34.3, 90}, {35.0, 88}, {35.7, 86}, {36.4, 85}, {37.1, 83}, {37.8, 81}, {38.5, 79}, {39.2, 77}, {39.9, 75}, {40.6, 72}, {41.3, 70}, {42.0, 69}, {42.7, 67}, {43.4, 66}, {44.1, 64}, {44.8, 64}, {46.9, 60}, {50.4, 56}, {51.1, 54}, {53.9, 50}, {56.7, 45}, {59.5, 40}});

    EvRouting* algo = new EvRouting(car, g);

    // Calculate the route and print the result in the style of TomToms API
    Route* route = algo->calculateRoute(from, to);
	json result;
	result["routes"] = { route->toJson() }; // This is similar to the TomTom API
    writeToFile(result);
}

int main(){
	// Load a car routing graph from OpenStreetMap-based data
    string pbf_file = "../data/germany-latest.osm.pbf";
    bool precomputed = false;
    g.loadGraph(pbf_file, precomputed); // The second parameter needs to be false for the first run with the pbf graph.
    g.loadChargers("../data/chargers.csv");

    calculateExampleRoute();
}