
#define TESTING

//------- opentime.h starts here

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    float t;
} OT_seconds;

typedef struct {
    OT_seconds start;
    OT_seconds end;
} OT_TimeInterval;
const OT_TimeInterval OT_TimeInterval_default = { {0.f}, {INFINITY} };
const OT_TimeInterval OT_TimeInterval_continuum = {{-INFINITY}, {INFINITY}};

typedef struct {
    OT_seconds t;
    float s;
} OT_TimeAffineTransform;
const OT_TimeAffineTransform OT_TimeAffineTransform_default = { {0.f}, 1.f };

struct OpenTimeInterfaceDetail;
typedef struct OpenTimeInterfaceDetail OpenTimeInterfaceDetail;
typedef struct OpenTimeInterface {
    OpenTimeInterfaceDetail* detail;
    void (*deinit)(struct OpenTimeInterface*);

    // construction
    OT_seconds (*duration)(OT_TimeInterval*);
    OT_TimeInterval (*from_start_duration)(OT_seconds start, OT_seconds duration);
    OT_TimeInterval (*from_start)(OT_seconds start);

    // transforms
    OT_seconds (*transform_seconds)(OT_TimeAffineTransform*, OT_seconds*);
    OT_TimeInterval (*transform_interval)(OT_TimeAffineTransform*, OT_TimeInterval*);
    OT_TimeAffineTransform (*compose_transform)(OT_TimeAffineTransform*, OT_TimeAffineTransform*);
    OT_TimeAffineTransform (*invert_transform)(OT_TimeAffineTransform*);

    // Interval Algebra
    bool (*interval_equals)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_precedes)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_meets)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_starts)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_overlaps)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_starts_or_overlaps)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_during)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_ends)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_disjoint)(OT_TimeInterval* a, OT_TimeInterval* b);
    bool (*interval_within)(OT_TimeInterval* a, OT_TimeInterval* b);
} OpenTimeInterface;

typedef struct {
    void* (*malloc)(size_t);
    void (*free)(void*);
} OpenTimeAllocator;

OpenTimeInterface* opentime_create(OpenTimeAllocator*);

#define IMPL_OPENTIME
#ifdef IMPL_OPENTIME

struct OpenTimeInterfaceDetail {
    OpenTimeAllocator* alloc;
};

void ot_deinit(OpenTimeInterface* ot) {
    if (!ot || !ot->detail)
        return;

    ot->detail->alloc->free(ot);
}

OT_seconds ot_duration(OT_TimeInterval* ival) {
    if (!ival)
        return (OT_seconds){ 0.f };

    if (!isfinite(ival->start.t) || !isfinite(ival->end.t))
        return (OT_seconds){INFINITY};

    return (OT_seconds){ ival->end.t - ival->start.t };
}

OT_TimeInterval ot_from_start_duration(OT_seconds start, OT_seconds duration) {
    if (duration.t <= 0)
        return (OT_TimeInterval){ { start.t + duration.t }, { -duration.t } };
    return (OT_TimeInterval){ start.t, { start.t + duration.t }};
}

OT_seconds ot_transform_seconds(OT_TimeAffineTransform* x, OT_seconds* s) {
    if (!x || !s)
        return (OT_seconds) { 0.f };

    return (OT_seconds) { s->t * x->s + x->t.t };
}

OT_TimeInterval ot_transform_interval(OT_TimeAffineTransform* x, OT_TimeInterval* ti) {
    if (!x || !ti)
        return OT_TimeInterval_default;

    return (OT_TimeInterval) { 
        ot_transform_seconds(x, &ti->start), ot_transform_seconds(x, &ti->end) };
}

OT_TimeAffineTransform ot_compose_transform(OT_TimeAffineTransform* x1,
        OT_TimeAffineTransform* x2) {
    if (!x1 || !x2)
        return OT_TimeAffineTransform_default;

    return (OT_TimeAffineTransform) {
        ot_transform_seconds(x1, &x2->t),
        x1->s * x2->s };
}

OT_TimeAffineTransform ot_invert_transform(OT_TimeAffineTransform* x) {
    return (OT_TimeAffineTransform) {
        (OT_seconds) { -x->t.t / x->s },
        1.f / x->s };
}
 
bool ot_interval_equals(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->start.t == b->start.t) && (a->end.t == b->end.t);
}

bool ot_interval_precedes(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (b->start.t > a->end.t);
}

bool ot_interval_meets(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->end.t == b->start.t);
}

bool ot_interval_disjoint(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->end.t < b->start.t) ||
        (b->end.t < a->start.t);
}

bool ot_interval_starts(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->start.t == b->start.t) &&
        (a->end.t < b->start.t);
}

bool ot_interval_ends(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->start.t > b->start.t) &&
        (a->end.t == b->start.t);
}

bool ot_interval_overlaps(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->start.t < b->start.t) &&
        (a->end.t < b->start.t);
}

bool ot_interval_starts_or_overlaps(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->start.t <= b->start.t) &&
        (a->end.t > b->start.t);
}

bool ot_interval_during(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->start.t > b->start.t) &&
        (a->end.t < b->start.t);
}

bool ot_interval_within(OT_TimeInterval* a, OT_TimeInterval* b) {
    if (!a || !b)
        return false;

    return (a->start.t >= b->start.t) &&
        (a->end.t < b->start.t) && !ot_interval_equals(a, b);
}


OT_TimeInterval ot_from_start(OT_seconds start) {
    return (OT_TimeInterval){ start, { INFINITY } };
}

OpenTimeInterface* opentime_create(OpenTimeAllocator* alloc) {
    if (!alloc)
        return NULL;

    OpenTimeInterface* ot = (OpenTimeInterface*) alloc->malloc(sizeof(OpenTimeInterface));
    ot->detail = (OpenTimeInterfaceDetail*) alloc->malloc(sizeof(OpenTimeInterfaceDetail));
    ot->detail->alloc = alloc;
    ot->deinit = ot_deinit;
    ot->duration = ot_duration;
    ot->from_start_duration = ot_from_start_duration;
    ot->from_start = ot_from_start;
    ot->transform_seconds = ot_transform_seconds;
    ot->transform_interval = ot_transform_interval;
    ot->compose_transform = ot_compose_transform;
    ot->invert_transform = ot_invert_transform;
    ot->interval_equals = ot_interval_equals;
    ot->interval_precedes = ot_interval_precedes;
    ot->interval_meets = ot_interval_meets;
    ot->interval_starts = ot_interval_starts;
    ot->interval_overlaps = ot_interval_overlaps;
    ot->interval_starts_or_overlaps = ot_interval_starts_or_overlaps;
    ot->interval_during = ot_interval_during;
    ot->interval_ends = ot_interval_ends;
    ot->interval_disjoint = ot_interval_disjoint;
    ot->interval_within = ot_interval_within;
    return ot;
}

#ifdef TESTING
#include <stdlib.h>

void test_opentime() {
    OpenTimeAllocator alloc = { .malloc = malloc, .free = free };
    OpenTimeInterface* ot = opentime_create(&alloc);
    {
        OT_TimeInterval ival = { {10.f}, {20.f} };
        OT_TimeInterval tval = ot->from_start_duration((OT_seconds){10.f}, (OT_seconds){10.f});
        bool success = ot->interval_equals(&ival, &tval) == true;
    }
    {
        OT_TimeInterval ival = { {10.f}, {20.f} };
        OT_TimeInterval tval = ot->from_start((OT_seconds){0.f});
        bool success = ot->interval_starts_or_overlaps(&ival, &tval) == false;
        tval = ot->from_start((OT_seconds){10.f});
        success = ot->interval_starts_or_overlaps(&ival, &tval) == true;
        tval = ot->from_start((OT_seconds){15.f});
        success = ot->interval_starts_or_overlaps(&ival, &tval) == true;
        tval = ot->from_start((OT_seconds){20.f});
        success = ot->interval_starts_or_overlaps(&ival, &tval) == false;
        tval = ot_from_start((OT_seconds){25.f});
        success = ot->interval_starts_or_overlaps(&ival, &tval) == false;
        tval = ot_from_start((OT_seconds){-INFINITY});
        success = ot->interval_starts_or_overlaps(&ival, &tval) == false;
        tval = ot_from_start((OT_seconds){INFINITY});
        success = ot->interval_starts_or_overlaps(&ival, &tval) == false;
    }
    {
        // offset test
        OT_TimeInterval cti = { { 10.f}, {20.f } };
        OT_TimeAffineTransform xform = { { 10.f }, 1.f };
        OT_TimeInterval result = ot->transform_interval(&xform, &cti);
        OT_TimeInterval tval = { {20.f}, {30.f} };
        bool success = ot->interval_equals(&tval, &result) == true;
        success = ot->duration(&result).t == 10.f;
        success = ot->duration(&result).t == ot->duration(&cti).t;
        OT_TimeAffineTransform xform2 = ot->compose_transform(&xform, &xform);
        success = (xform.t.t == 20.f) && (xform.s == 1.f);
    }
    {
        // scale test
        OT_TimeInterval cti = { { 10.f}, {20.f } };
        OT_TimeAffineTransform xform = { { 10.f }, 2.f };
        OT_TimeInterval result = ot->transform_interval(&xform, &cti);
        OT_TimeInterval tval = { {30.f}, {50.f} };
        bool success = ot->interval_equals(&tval, &result) == true;
        success = ot->duration(&result).t == ot->duration(&cti).t * xform.s;
        OT_TimeAffineTransform xform2 = ot->compose_transform(&xform, &xform);
        success = (xform.t.t == 30.f) && (xform.s == 4.f);
    }
    {
        // invert test
        OT_TimeAffineTransform xform = { { 10.f }, 2.f };
        OT_TimeAffineTransform inv = ot->invert_transform(&xform);
        OT_TimeAffineTransform identity = ot->compose_transform(&xform, &inv);
        bool success = (identity.t.t == 0.f) && (identity.s == 1);
        OT_seconds pt = { 10.f };
        OT_seconds result = ot->transform_seconds(&xform, &pt);
        result = ot->transform_seconds(&inv, &result);
        success = pt.t == result.t;
    }

    ot->deinit(ot);
}

#endif // TESTING


#endif // IMPL_OPENTIME

//------- opentimeline.h starts here
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct { uint32_t id; } IntervalOidId;
const IntervalOidId IntervalOidId_default = { 0 };
struct TimelineTopologyInterface;
typedef struct TimelineTopologyInterface TimelineTopologyInterface;

typedef struct {
    IntervalOidId self;
    IntervalOidId seq;
    IntervalOidId sync;
    OT_TimeAffineTransform basis;
    OT_TimeInterval bounds;
    //OT_TImeCurve mapping;
} IntervalOid;
const IntervalOid IntervalOid_default = {{0}, {0}, {0},
    { {0}, 1.f }, 
    {{0}, {INFINITY}}};

struct TimelineTopologyDetail;
typedef struct TimelineTopologyDetail TimelineTopologyDetail;

struct TimelineTopologyInterface {
    IntervalOid timeline_root;

    void (*deinit)(TimelineTopologyInterface* self);

    IntervalOidId (*new_oid)(TimelineTopologyInterface* self);
    
    void (*add_sync)(TimelineTopologyInterface* self, IntervalOidId parent, IntervalOidId);
    void (*add_seq)(TimelineTopologyInterface* self, IntervalOidId parent, IntervalOidId);
    void (*add_syncs)(TimelineTopologyInterface* self, IntervalOidId parent, 
            IntervalOidId* first, IntervalOidId* last);
    void (*add_seqs)(TimelineTopologyInterface* self, IntervalOidId parent, 
            IntervalOidId* first, IntervalOidId* last);
    
    TimelineTopologyDetail* detail;
};

typedef struct {
    void* (*malloc)(size_t);
    void (*free)(void*);
} TimelineAllocator;

TimelineTopologyInterface*
timeline_topology_create(
        int initial_capacity,
        TimelineAllocator*);

#define OPENTIMELINE_IMPL
#ifdef OPENTIMELINE_IMPL

#ifdef TESTING
#include <stdlib.h>
#endif

struct TimelineTopologyDetail {
    TimelineAllocator* alloc;
    IntervalOid* timeline_data;
    int next_available;
    int last_available;
};

static void topo_deinit(TimelineTopologyInterface* self) {
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

static IntervalOidId topo_new_oid(TimelineTopologyInterface* self) {
    if (!self)
        return IntervalOidId_default;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail || (detail->next_available == detail->last_available))
        return IntervalOidId_default;

    IntervalOidId result = { detail->next_available++ };
    return result;
}

static void topo_add_sync(TimelineTopologyInterface* self, 
        IntervalOidId parent, IntervalOidId child) {
    if (!self)
        return;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail)
        return;

    detail->timeline_data[parent.id].sync = child;
}

static void topo_add_seq(TimelineTopologyInterface* self, 
        IntervalOidId parent, IntervalOidId child) {
    if (!self)
        return;

    TimelineTopologyDetail* detail = (TimelineTopologyDetail*) self->detail;
    if (!detail)
        return;

    detail->timeline_data[parent.id].seq = child;
}

static void topo_add_seqs(TimelineTopologyInterface* self, 
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

static void topo_add_syncs(TimelineTopologyInterface* self, 
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

TimelineTopologyInterface* 
timeline_topology_create(
        int initial_capacity,
        TimelineAllocator* alloc) {

    if (!alloc)
        return NULL;

    TimelineTopologyInterface* topo = 
        (TimelineTopologyInterface*) alloc->malloc(sizeof(TimelineTopologyInterface));
    if (!topo)
        return NULL;

    topo->timeline_root = IntervalOid_default;
    TimelineTopologyDetail* detail = 
        (TimelineTopologyDetail*) alloc->malloc(sizeof(TimelineTopologyDetail));
    detail->alloc = (void*) alloc;
    detail->timeline_data = 
        (IntervalOid*) alloc->malloc(sizeof(IntervalOid) * (1 + initial_capacity));
    memset(detail->timeline_data, 0, sizeof(IntervalOid) * (1 + initial_capacity));
    for (int i = 0; i < initial_capacity; ++i) {
        detail->timeline_data[i].self.id = i;
        detail->timeline_data[i].bounds = OT_TimeInterval_default;
    }

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
    TimelineTopologyInterface* topo = timeline_topology_create(100, &alloc);
    
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

#endif // TESTING


int main(int argc, char** argv) {
    test_creation();
    return 0;
}

#endif //OPENTIMELINE_IMPL



