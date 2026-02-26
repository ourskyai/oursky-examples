#pragma once

#include <crow.h>

class SnpeWorker;

void register_routes(crow::SimpleApp& app, SnpeWorker& worker);
