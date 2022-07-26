#!/usr/bin/python
import requests
import time
import json

# This code collects the charging station for a given country from the TomTom API

api_key = "INSERT_API_KEY_HERE" # Insert your API Key here.

def findEVStations(bounds, countryCode = 'DE', category = 7309, minPowerKw = 0, limit = 100):
    lastTime= time.time_ns() // 1_000_000
    num_current = 0
    
    topLeft = bounds[0], bounds[1]
    bottomRight = bounds[2], bounds[3]
    
    url = f'https://api.tomtom.com/search/2/poiSearch/Charging%20Station.json?' \
        f'countrySet={countryCode}'\
        f'&topLeft={topLeft[0]},{topLeft[1]}'\
        f'&btmRight={bottomRight[0]},{bottomRight[1]}'\
        f'&categorySet={category}'\
        f'&key={api_key}'\
        f'&limit={limit}'\
        f'&minPowerKW={minPowerKw}'
    
    r = requests.get(url)
    if r.status_code != 200:
        print(r.content)
        return
    
    total_results = r.json()['summary']['totalResults']
    
    # This method gets called recursively and splits the search area into 4 smaller areas
    def findStationsInBounds(bounds):
        nonlocal lastTime
        nonlocal num_current
        nonlocal total_results
        nonlocal countryCode
        nonlocal category
        nonlocal minPowerKw
        nonlocal limit

        while(((time.time_ns() // 1_000_000) - lastTime) < 250): # wait to fulfill max of 5 qps, maybe make this buffer even higher
            pass
        
        topLeft = bounds[0], bounds[1]
        bottomRight = bounds[2], bounds[3]

        url = f'https://api.tomtom.com/search/2/poiSearch/Charging%20Station.json?' \
        f'countrySet={countryCode}'\
        f'&topLeft={topLeft[0]},{topLeft[1]}'\
        f'&btmRight={bottomRight[0]},{bottomRight[1]}'\
        f'&categorySet={category}'\
        f'&key={api_key}'\
        f'&limit={limit}'\
        f'&minPowerKW={minPowerKw}'  

        r = requests.get(url)
        if r.status_code != 200:
            print("ERROR:", r.content)
            return []

        response = r.json()
        summary = response['summary']
        stations = []
        
        if(summary['numResults'] != summary['totalResults']):
            center = [(topLeft[0] + bottomRight[0])/2, (topLeft[1] + bottomRight[1]) / 2]
            stations = findStationsInBounds(bounds=[center[0], topLeft[1], bottomRight[0], center[1]]) + \
                findStationsInBounds(bounds=[topLeft[0], topLeft[1], center[0], center[1]]) + \
                findStationsInBounds(bounds=[topLeft[0], center[1], center[0], bottomRight[1]]) + \
                findStationsInBounds(bounds=[center[0], center[1], bottomRight[0], bottomRight[1]])        
        else:
            for result in response['results']:
                stations.append(result)
            if len(stations) > 0:
                num_current += len(stations)
                print(f'{num_current}/{total_results} = {round(num_current * 100 / total_results, 3)}%')
        return stations
    
    return findStationsInBounds(bounds)

def writeToCsv(stations, filename):
    with open(f'{filename}.csv', 'w') as f:
        f.write("id,name,entry_lat,entry_lon,lat,lon,kws,types,currentTypes,node\n") 
        for charger in stations:
            f.write(f"{charger['id']},{charger['poi']['name']},")
            try:
                entryPoints = charger["entryPoints"]
                entryPoints = [entry for entry in entryPoints if entry["type"] == "main"]
                entry = entryPoints[0]
                f.write(f"{entry['position']['lat']},{entry['position']['lon']},")
            except:
                # On error use lat and lon of charger as entry points
                print(f"Error with entry points of: {charger['poi']['name']}")
                f.write(f"{charger['position']['lat']},{charger['position']['lon']},")
            f.write(f"{charger['position']['lat']},{charger['position']['lon']},")
            kws = ""
            names = ""
            currentTypes = ""
            if 'chargingPark' in charger and 'connectors' in charger['chargingPark']:
                for connector in charger['chargingPark']['connectors']: 
                    if 'ratedPowerKW' in connector and 'connectorType' in connector and 'currentType' in connector:
                        if (len(kws) != 0):
                            kws += "|"
                            names += "|"
                            currentTypes += "|"
                        kws += str(connector['ratedPowerKW'])
                        names += connector['connectorType']
                        currentTypes += connector['currentType']
            f.write(f"{kws},{names},{currentTypes}\n")

def main():
    bounds = [55.215, 5.050, 47.604, 14.388] # Bounds of Germany
    stations = findEVStations(bounds=bounds, countryCode = 'DE', minPowerKw = 0)

    filename = "data/chargers"

    with open(f'{filename}.json', 'w') as outfile:
        json.dump({'results': stations}, outfile, indent=4)
    print("JSON output created.")
    writeToCsv(stations, filename)
    print("CSV output created.")

if __name__ == '__main__':
    main()