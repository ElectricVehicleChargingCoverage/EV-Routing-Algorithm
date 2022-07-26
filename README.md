# EV-Routing-Algorithm

This repository contains a simple routing algorithm for electric vehicles. This algorithm takes a model of an electric vehicle that is described by some parameters and computes the consumption of it for the driven route. If the route cannot be driven without a charging stop, the algorithm selects charging stops along the route.

## Setup

### RoutingKit

The [RoutingKit](https://github.com/RoutingKit/RoutingKit) requires zlib to work:

```bash
sudo apt-get install zlib1g-dev
```

Download your map as a `.pbf` file from [Geofabrik](https://download.geofabrik.de/index.html) and copy it into the `data` folder. Make sure to paste the name of the file as `pbf_file` into the `loadGraph()` method in `src/Main.cpp`. If you run the program for the first time for this graph, you need to make sure that the boolean `precomputed` is set to false. This will run the contraction hierarchy computations and save the result in a separate file. Afterwards you can set `precomputed` to `true` and save some time.

You can also use `boost` to check for the existence of the file automatically. In this case you need to uncomment the line in `loadGraph()` in `include/Graph.h`

### Charging stations

This routing algorithm also requires a list of charging stations provided as `charger.csv` in the `data` directory. Such list can be created with the `gatherEVStations.py` script which uses the TomTom API to collect such a list. However, you need your own API Key that you paste in the `INSERT_API_KEY_HERE` field at the beginning of that script. You can apply for a Key on the [TomTom Webpage](https://developer.tomtom.com/user/me/apps) after you registered.

You also have to adapt the `bounds` array and `countryCode` in the `main` function. The bounds are the top-left and bottom-right coordinates of the country and are used for the search process.

After you adapted these values you can run the script with

```bash
python3 gatherEVStations.py
```

## Using this code

### Building

Make sure you have CMake installed:

```bash
sudo apt install cmake
```

In the file called `platform.txt` you need to enter your platform in a single line (default: `WSL`, other options: `NATIVE_UNIX`, `WINDOWS`).

If you use the CMake extension from VS Code you can choose the "Build All Projects" options in the CMake tab to build the project.

You can then execute

```bash
cd build
./Routing
```

to run the code. This should print some information of the loading process to the console create an `output.json` file in the `build` directory. This is the calculated route of the algorithm.

### Parameters

You can change the start and destination of the route in `calculateExampleRoute()` in `src/Main.cpp`. There you can also define the electric vehicle that you want to use.

In `main()` you need the insert the name of the graph that you want to use and set the `precomputed` value accordingly.

_Note: You need to recompile the project after changing those values before running it._

### Result

The result of the routing algorithm will be exported as JSON.  This response is structured just like the result from the [TomTom Long Distance EV Routing API](https://developer.tomtom.com/routing-api/documentation/extended-routing/long-distance-ev-routing#response-data). However, since this may change you should check the `exampleResult.json` file to get an overview of the provided information.

### Visualization

You can use the basic [webpage](https://electricvehiclechargingcoverage.github.io/OverheadVisualization/EV-Routing-Algorithm) to upload the `output.json` file and get an interactive visualization of the JSON result.

### Electric Vehicles

Here are some code snippets for four vehicles that can be used in the `Main.cpp` file.

#### Tesla Model 3 LR

```cpp
EvCar car = EvCar("Tesla Model 3 LR", 70.0, "10,10.7:50,10.7:80,13.3:120,16.3");
car.weight = 2019;
car.setChargingCurve({{ 7.0, 190 }, { 7.7, 187 }, {8.4, 182}, {9.1, 175}, {9.8, 170}, {10.5, 166}, {11.2, 162}, {11.9, 159}, {12.6, 156}, {14.0, 150}, {14.7, 148}, {15.4, 147}, {16.1, 145}, {16.8, 144}, {18.9, 138}, {19.6, 136}, {21.7, 131}, {22.4, 128}, {23.1, 126}, {23.8, 124}, {24.5, 122}, {25.2, 120}, {25.9, 118}, {26.6, 116}, {28.7, 109}, {30.8, 102}, {31.5, 99}, {32.2, 97}, {32.9, 95}, {33.6, 92}, {34.3, 90}, {35.0, 88}, {35.7, 86}, {36.4, 85}, {37.1, 83}, {37.8, 81}, {38.5, 79}, {39.2, 77}, {39.9, 75}, {40.6, 72}, {41.3, 70}, {42.0, 69}, {42.7, 67}, {43.4, 66}, {44.1, 64}, {44.8, 64}, {46.9, 60}, {50.4, 56}, {51.1, 54}, {53.9, 50}, {56.7, 45}, {59.5, 40}});
```

#### Volkswagen ID.4

```cpp
EvCar car = EvCar("Volkswagen ID.4", 77.0, "10,12.8:50,12.8:80,16.4:120,20.5");
car.weight = 2224;
car.setChargingCurve({{0.0, 122}, {3.85, 127}, {7.7, 126}, {11.55, 127}, {15.4, 127}, {19.25, 126}, {23.1, 124}, {26.95, 117}, {30.8, 108}, {34.65, 99}, {38.5, 92}, {42.35, 85}, {46.2, 75}, {50.05, 68}, {53.9, 65}, {57.75, 64}, {61.6, 60}, {65.45, 43}, {69.3, 35}, {73.15, 26}});
```

#### Renault Zoe

```cpp
EvCar car = EvCar("Renault Zoe", 52.0, "10,10.9:50,10.9:80,14.2:120,18.2");
car.weight = 1677;
car.setChargingCurve({{1.56, 44}, {2.6, 44}, {5.2, 45}, {10.4, 45}, {13.0, 46}, {15.6, 44}, {18.2, 42}, {20.8, 40}, {23.4, 39}, {26.0, 36}, {28.6, 33}, {31.2, 31}, {33.8, 29}, {36.4, 26}, {39.0, 25}, {41.6, 24}});
```

#### Fiat 500e

```cpp
EvCar car = EvCar("Fiat 500e", 37.3, "10,10.5:50,10.5:80,13.8:120,17.3");
car.weight = 1465;
car.setChargingCurve({{2.98, 72}, {3.73, 76}, {5.6, 82}, {7.46, 85}, {9.32, 82}, {11.19, 80}, {13.06, 74}, {14.92, 68}, {16.78, 67}, {18.65, 66}, {20.52, 58}, {22.38, 53}, {24.24, 53}, {26.11, 51}, {27.98, 47}, {29.84, 44}, {31.7, 15}});
```

## Dependencies

This repository uses [json](https://github.com/nlohmann/json) for creating an output that can easily be visualized.

It also uses [RoutingKit](https://github.com/RoutingKit/RoutingKit) for the graph and basic routing.
