#ifndef UTIL_H
#define UTIL_H

static std::vector<std::string> split(std::string str, char delimiter) {
    std::stringstream ss(str);
    std::string item;
    std::vector<std::string> result;

    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }

    return result;
}

#endif //UTIL_H
