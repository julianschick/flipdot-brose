#include "comx.h"

#include "wifi.h"

string Comx::interpret(std::string line) {
	line = trim(line);
	string result = "";
	size_t first_space = line.find_first_of(" ");

	string cmd;
	string args;
	if (first_space != -1) {
		cmd = line.substr(0, first_space);
		args = line.substr(first_space + 1);
	} else {
		cmd = line;
		args = "";
	}

	std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

	if (cmd == "whoareyou") {
		return "Flipdot v1.0\n";
	} else if (cmd == "reboot") {
		//
		esp_restart();
		//
	} else if (cmd == "getssid") {

		return Wifi::get_ssid() + "\n";

	} else if (cmd == "setssid") {

		if (args.length() == 0) {
			return "ERR 2: Number of arguments wrong.\n";	
		}		

		if (Wifi::set_ssid(args)) {
			return "SETSSID\n";
		} else {
			return "ERR 11: SSID could not be set.\n";
		}

	} else if (cmd == "setpass") {

		if (args.length() == 0) {
			return "ERR 2: Number of arguments wrong.\n";
		}		

		if (Wifi::set_pass(args)) {
			return "SETPASS\n";
		} else {
			return "ERR 12: Password could not be set.\n";
		}

	} else if (cmd == "getip") {

 		return Wifi::get_ip() + "\n";

	} else if (cmd == "setwifi") {

		vector<string> tokens = tokenize(args);

		if (tokens.size() != 1) {
			err2(1, tokens.size());
		}

		if (tokens[0] == "disabled") {
			Wifi::disable();
			return "SETWIFI\n";
		} else if (tokens[0] == "enabled") {
			Wifi::enable();
			return "SETWIFI\n";
		} else {
			return "ERR 3: Argument format wrong.\n";
		}

	} else if (cmd == "getwifi") {

		if (Wifi::is_enabled()) {
			return "ENABLED\n";
		} else {
			return "DISABLED\n";
		}

	} else {
		return "ERR 1: Command unknown.\n";
	}

	return "";
}

int Comx::stoi(const std::string &str, bool* failure) {
	const char* c_str = str.c_str();
	char* endptr = const_cast<char*>(c_str);

	int result = strtol(c_str, &endptr, 10);

	if (endptr == c_str) {
		*failure = true;
	}

	return result;
}

std::string Comx::trim(const std::string &s) {
	std::string::const_iterator it = s.begin();
	while (it != s.end() && isspace(*it))
		it++;

	std::string::const_reverse_iterator rit = s.rbegin();
	while (rit.base() != it && isspace(*rit))
		rit++;

	return std::string(it, rit.base());
}

std::vector<std::string> Comx::tokenize(const std::string &str) {
	vector<string> result;
	istringstream f(str);
	string token;
	while (getline(f, token, ' ')) {
		token = trim(token);
		if (!token.empty()) {
			result.push_back(token);
		}
		
	}
	return result;
}