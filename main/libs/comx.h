#ifndef COMX_H_
#define COMX_H_

#include "globals.h"

#include <vector>
#include <algorithm>

using namespace std;

class Comx {

public:
	static string interpret(string line);

private:
	static vector<string> tokenize(const string &str);
	static string trim(const string &str); 
	static int stoi(const string &str, bool* failure);

	static inline string err2(int expected, int got) { 
		ostringstream oss;
		oss << "ERR 2: Number of arguments wrong, expected " << expected << " but got " << got << ".\n";
		return oss.str();
	};
	
	static inline string err3() { return "ERR 3: Argument format wrong.\n"; };

};

#endif //COMX_H_
