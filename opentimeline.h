
#include <stdint.h>

struct TimelineId;
typedef struct TimelineId TimelineId;
struct IntervalOidId;
typedef struct IntervalOidId IntervalOidId;

typedef struct {
    TimelineId id;
    IntervalOidId seq;
    IntervalOidId sync;
} IntervalOid;

struct TimelineTopologyDetail;
typedef struct TimelineTopologyDetail TimelineTopologyDetail;

typedef struct {
    TimelineId* timeline_data = NULL;
    IntervalOid* timeline_root = NULL;

    void (*deinit)(TimelineTopology* self) = NULL;
    void* detail = NULL; 
} TimelineTopology;

typedef struct {
    void* (*malloc)(size_t) = NULL;
    void (*free)(void*) = NULL;
} TimelineAllocator;

TimelineTopology* 
timeline_topology_create(
        int initial_capacity,
        TimelineAllocator*);

#define OPENTIMELINE_IMPL
#ifdef OPENTIMELINE_IMPL

#define TESTING
#ifdef TESTING
#include <stdlib.h>
#endif

struct TimelineId {
    uint32_t val = 0;
};

struct IntervalOidId {
    uint32_t val = 0;
};

struct TimelineTopologyDetail {
    TimelineAllocator* alloc = NULL;
};

static void topo_deinit(TimelineTopology* self) {
    if (!self || !self->detail || !self->detail->alloc)
        return;


    self->detail->alloc->free(self->detail);
    self->detail->alloc-

TimelineTopology* 
timeline_topology_create(
        int initial_capacity,
        TimelineAllocator* alloc) {

    if (!alloc)
        return NULL;

    TimelineTopology* topo = (TimelineTopology*) alloc->malloc(sizeof(TimelineTopology));
    if (!topo)
        return NULL;

    topo->detail = (TimelineTopologyDetail) alloc->malloc(sizeof(TimelineTopologyDetail));
    topo->detail->alloc = alloc;
    topo->deinit = topo_deinit;
    return topo;
}


#ifdef TESTING

void test_creation() {
    TimelineAllocator alloc { .malloc = malloc; .free = free };
    TimelineTopology* topo = timeline_topology_create(100, &alloc);
    if (topo) {
        topo->deinit(topo);
    }
}

#endif


int main(int argc, char** argc) {
    test_creation();
    return 0;
}

#endif //OPENTIMELINE_IMPL



