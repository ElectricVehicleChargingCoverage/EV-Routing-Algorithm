#pragma once

using namespace std;

bool contains(string& delimiters, char c) {
	bool result = false;
	for (char ch : delimiters) result |= ch == c;
	return result;
}

vector<string> split(string& line, string delimiters, bool empty_allowed) {
	vector<string> tokens;
	size_t start = 0, end = 0;
	while (start < line.size()) {
		while (end < line.size() && !contains(delimiters, line[end])) ++end;
		if (start < end || empty_allowed)
			tokens.push_back(line.substr(start, end - start));
		start = ++end;
	}
	return tokens;
}

float max(vector<string> strings) {
	float max = 0;
	for (string s : strings) {
		if (stof(s) > max)
			max = stof(s);
	}
	return max;
}
