#pragma once

#include "Point.h"

#include <routingkit/osm_simple.h>
#include <routingkit/contraction_hierarchy.h>
#include <routingkit/inverse_vector.h>
#include <routingkit/timer.h>
#include <routingkit/geo_position_to_node.h>
#include "ChargingPark.h"
using namespace RoutingKit;

#define MIN_CHARGER_KW 0 // The minimum rated power that a charging station needs to be considered.

struct Graph {
    RoutingKit::SimpleOSMCarRoutingGraph graph;
    std::vector<unsigned> tail;
    RoutingKit::ContractionHierarchy ch;
    vector<ChargingPark*> chargingParks;
    unordered_map<unsigned, ChargingPark*> parkMap;

    void loadGraph(string pbf_file, bool precomputed = false) {
        auto setup_start_time = chrono::high_resolution_clock::now();
        cout << "Loading graph..." << endl;
        graph = simple_load_osm_car_routing_graph_from_pbf(pbf_file);
        tail = invert_inverse_vector(graph.first_out);

        auto graphDuration = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - setup_start_time);
        cout << "Loading took " << graphDuration.count() / 1000 << " s." << endl;

        // Build the shortest path index
        cout << "Building shortest path index..." << endl;
        string ch_save = pbf_file + ".ch";
        // bool precomputed = boost::filesystem::exists(ch_save); // If you have boost, you can uncomment this and remove the parameter.
        ch = precomputed ? ContractionHierarchy::load_file(ch_save) : ContractionHierarchy::build(
            graph.node_count(), 
            tail, graph.head, 
            graph.travel_time
        );

        if (!precomputed) ch.save_file(ch_save);
        cout << "Done!" << endl;

        auto setup_finish_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(setup_finish_time - setup_start_time);
        cout << "Graph setup took " << duration.count() / 1000 << " s." << endl;
    }

    void loadChargers(string path) {
        cout << "Loading charging stations..." << endl;
        auto start_time = chrono::high_resolution_clock::now();
        GeoPositionToNode map_geo_position(graph.latitude, graph.longitude);
        ifstream file(path);
        string line;
        getline(file, line); // skip first line
        while (getline(file, line)) {
            //id,name,entry_lat,entry_lon,lat,lon,kws,types,currentTypes
            vector<string> csv = split(line, ",", true);
            if (csv[6] == "" || csv[7] == "" || csv[8] == "")
                continue;
            long long id = stoll(csv[0]);
            string name = csv[1];
            Point* location = new Point(stod(csv[4]), stod(csv[5]));

            vector<string> kws = split(csv[6], "|", false);
            vector<string> types = split(csv[7], "|", false);
            vector<string> currentTypes = split(csv[8], "|", false);

            if(kws.size() == 0 || types.size() != kws.size() || types.size() != currentTypes.size())
                continue;

            if (max(kws) < MIN_CHARGER_KW)
                continue;

            auto park = new ChargingPark(id, name, location);
            float entry_lat = stod(csv[2]);
            float entry_lon =  stod(csv[3]);
            unsigned node = map_geo_position.find_nearest_neighbor_within_radius(entry_lat, entry_lon, 1000).id;
            park->node = node;
            chargingParks.push_back(park);
            parkMap[node] = park;
            for (int i = 0; i < kws.size(); ++i)
                chargingParks.back()->connectors.push_back(
                    new ChargingConnector(types[i], stof(kws[i]), currentTypes[i])
                );
        }
        auto finish_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(finish_time - start_time);
        cout << "Loading charging stations took " << duration.count() / 1000 << " s." << endl;
    }

    vector<ChargingPark*> findKNearestChargers(Point* p, int k, int maxDist) {
        vector<ChargingPark*> all;
        auto compare = [p](ChargingPark* a, ChargingPark* b) {
            return (distance_in_km(p, a->location) < distance_in_km(p, b->location)); 
        };
        for (auto park : chargingParks) {
            if (distance_in_km(p, park->location) > maxDist)
                continue;
            all.push_back(park);
            if (all.size() <= k)
                continue;
            sort(all.begin(), all.end(), compare);
            all.pop_back();
        }
        return all;
    }

    float travelTimeInSec(unsigned edge) {
        return graph.travel_time[edge] / 1000.0;
    }

    float distanceInMeter(unsigned edge) {
        return graph.geo_distance[edge];
    }
};
