#include "probe.h"
/* Linked with the exact v2.1 sample CRT, supplied locally. No globals,
 * absolute code/data pointers, allocation, or V-thread dispatch in this test.
 * Execute directly on the bridge entry context; worker-core tests follow. */
int main(void *processing_resource, void *user_arg, unsigned int size) {
    (void)processing_resource;
    return probe_handle((ProbePacket *)user_arg,size);
}
