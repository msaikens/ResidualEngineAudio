#pragma once
#include <stdint.h>

typedef struct rv_transport_steam rv_transport_steam_t;

rv_transport_steam_t* rv_transport_steam_create(uint64_t steam_id);

void rv_transport_steam_destroy(rv_transport_steam_t* t);

int rv_transport_steam_send(rv_transport_steam_t* t,
                            const uint8_t* data,
                            uint32_t size);

int rv_transport_steam_poll(rv_transport_steam_t* t,
                            uint8_t* out,
                            uint32_t cap,
                            uint32_t* size);
