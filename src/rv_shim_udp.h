#pragma once
#include <stdint.h>
#include "rv_shim_transport.h"
#include "rv_udp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rv_shim_udp rv_shim_udp_t;

typedef struct rv_shim_udp_config {
    const char* relay_host;   // literal IP
    uint16_t    relay_port;
    uint16_t    local_port;   // 0 => bind to relay_port
    int         nonblocking;  // 1 recommended
} rv_shim_udp_config_t;

// Create/Destroy the UDP shim instance
rv_shim_udp_t* rv_shim_udp_create(const rv_shim_udp_config_t* cfg);
void rv_shim_udp_destroy(rv_shim_udp_t* u);

// Get a generic transport shim interface for this UDP instance
rv_shim_transport_t rv_shim_udp_as_transport(rv_shim_udp_t* u);

#ifdef __cplusplus
}
#endif
