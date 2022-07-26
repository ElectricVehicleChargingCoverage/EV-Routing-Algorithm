#pragma once
#include "json.hpp"
using json = nlohmann::json;

class Point
{
public:
	double lat, lon;
	Point(double _lat, double _lon) {
		lat = _lat;
		lon = _lon;
	};
	double getLat() { return lat; }
	double getLon() { return lon; }
	void setLat(double _lat) { lat = _lat; };
	void setLon(double _lon) { lon = _lon; };

	json toJson() {
		json result;
		result["latitude"] = lat;
		result["longitude"] = lon;
		return result;
	}
};

inline long double toRadians(const long double degree) {
	return M_PI / 180 * degree;
}

long double distance_in_km(Point* p, Point* q) {
	long double p_lat = toRadians(p->lat);
	long double p_lon = toRadians(p->lon);
	long double q_lat = toRadians(q->lat);
	long double q_lon = toRadians(q->lon);
	// Haversine Formula
	long double dlong = q_lon - p_lon;
	long double dlat = q_lat - p_lat;
	long double ans = pow(sin(dlat / 2), 2) +
		cos(p_lat) * cos(q_lat) *
		pow(sin(dlong / 2), 2);
	ans = 2 * asin(sqrt(ans));
	long double R = 6371;
	ans = ans * R;
	return ans;
}