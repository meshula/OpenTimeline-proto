
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct { uint32_t id; } IntervalOidId;
const IntervalOidId IntervalOidId_default = { 0 };
struct TimelineTopology;
typedef struct TimelineTopology TimelineTopology;

typedef struct {
    IntervalOidId self;
    IntervalOidId seq;
    IntervalOidId sync;
} IntervalOid;
const IntervalOid IntervalOid_default = {{0}, {0}, {0}};

struct TimelineTopologyDetail;
typedef struct TimelineTopologyDetail TimelineTopologyDetail;

struct TimelineTopology {
    IntervalOid timeline_root;

    void (*deinit)(TimelineTopology* self);
    IntervalOidId (*new_oid)(TimelineTopology* self);
    void (*add_sync)(TimelineTopology* self, IntervalOidId parent, IntervalOidId);
    void (*add_seq)(TimelineTopology* self, IntervalOidId parent, IntervalOidId);
    void (*add_syncs)(TimelineTopology* self, IntervalOidId parent, 
            IntervalOidId* first, IntervalOidId* last);
    void (*add_seqs)(TimelineTopology* self, IntervalOidId parent, 
            IntervalOidId* first, IntervalOidId* last);
    
    void* detail;
};

typedef struct {
    void* (*malloc)(size_t);
    void (*free)(void*);
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

struct TimelineTopologyDetail {
    TimelineAllocator* alloc;
    IntervalOid* timeline_data;
    int next_available;
    int last_available;
};

static void topo_deinit(TimelineTopology* self) {
    if (!self || !self->detail)
        return;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail->alloc)
        return;

    void (*freeFn)(void*) = detail->alloc->free;
    freeFn(detail->timeline_data);
    freeFn(self->detail);
    freeFn(self);
}

static IntervalOidId topo_new_oid(TimelineTopology* self) {
    if (!self)
        return IntervalOidId_default;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail || (detail->next_available == detail->last_available))
        return IntervalOidId_default;

    IntervalOidId result = { detail->next_available++ };
    return result;
}

static void topo_add_sync(TimelineTopology* self, 
        IntervalOidId parent, IntervalOidId child) {
    if (!self)
        return;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail)
        return;

    detail->timeline_data[parent.id].sync = child;
}

static void topo_add_seq(TimelineTopology* self, 
        IntervalOidId parent, IntervalOidId child) {
    if (!self)
        return;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail)
        return;

    detail->timeline_data[parent.id].seq = child;
}

static void topo_add_seqs(TimelineTopology* self, 
        IntervalOidId parent, IntervalOidId* first, IntervalOidId* last) {
    if (!self || !first || !last)
        return;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail)
        return;

    int root = parent.id;
    for (IntervalOidId* i = first; i != last; ++i) {
        detail->timeline_data[root].seq = *i;
        root = i->id;
    }
}

static void topo_add_syncs(TimelineTopology* self, 
        IntervalOidId parent, IntervalOidId* first, IntervalOidId* last) {
    if (!self || !first || !last)
        return;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail)
        return;

    int root = parent.id;
    for (IntervalOidId* i = first; i != last; ++i) {
        detail->timeline_data[root].sync = *i;
        root = i->id;
    }
}

TimelineTopology* 
timeline_topology_create(
        int initial_capacity,
        TimelineAllocator* alloc) {

    if (!alloc)
        return NULL;

    TimelineTopology* topo = 
        (TimelineTopology*) alloc->malloc(sizeof(TimelineTopology));
    if (!topo)
        return NULL;

    topo->timeline_root = IntervalOid_default;
    TimelineTopologyDetail* detail = 
        (TimelineTopologyDetail*) alloc->malloc(sizeof(TimelineTopologyDetail));
    detail->alloc = (void*) alloc;
    detail->timeline_data = 
        (IntervalOid*) alloc->malloc(sizeof(IntervalOid) * (1 + initial_capacity));
    memset(detail->timeline_data, 0, sizeof(IntervalOid) * (1 + initial_capacity));
    for (int i = 0; i < initial_capacity; ++i)
        detail->timeline_data[i].self.id = i;

    topo->detail = (void*) detail;
    detail->next_available = 1;
    detail->last_available = initial_capacity;
    topo->deinit = topo_deinit;
    topo->new_oid = topo_new_oid;
    topo->add_sync = topo_add_sync;
    topo->add_seq = topo_add_seq;
    topo->add_seqs = topo_add_seqs;
    topo->add_syncs = topo_add_syncs;
    return topo;
}


#ifdef TESTING

void test_creation() {
    TimelineAllocator alloc = { .malloc = malloc, .free = free };
    TimelineTopology* topo = timeline_topology_create(100, &alloc);
    
    IntervalOidId audio_track = topo->new_oid(topo);
    IntervalOidId video_track = topo->new_oid(topo);
    topo->add_sync(topo, topo->timeline_root.self, audio_track);
    topo->add_sync(topo, topo->timeline_root.self, video_track);

    IntervalOidId v_clips[3] = { 
        topo->new_oid(topo), topo->new_oid(topo), topo->new_oid(topo) };

    topo->add_seqs(topo, video_track, &v_clips[0], &v_clips[2]);

    if (topo) {
        topo->deinit(topo);
    }
}

#endif


int main(int argc, char** argv) {
    test_creation();
    return 0;
}

#endif //OPENTIMELINE_IMPL



