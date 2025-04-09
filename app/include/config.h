#pragma once

#include <string>

typedef struct {
    int sort = 0;
    bool dark_theme = false;
    bool dev_options = false;
    std::string cwd;
} config_t;

extern config_t cfg;

namespace Config {
    int Save(config_t config);
	int Load(void);
}
