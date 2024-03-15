#ifndef EXPRESSIF_PATHVARS_H
#define EXPRESSIF_PATHVARS_H

#include <string>
#include <map>

namespace expressif::http::server {
using PathVars = std::map<std::string, std::string>;
}

#endif //EXPRESSIF_PATHVARS_H
