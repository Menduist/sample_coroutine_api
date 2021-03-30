#ifndef SAMPLE_API_H
#define SAMPLE_API_H

#include "config.h"
struct main;

struct api;
struct odict;
struct api *api_init(struct main *main, struct config_api *conf);

#endif

