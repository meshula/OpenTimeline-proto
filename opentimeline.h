
#define TESTING

//------- opentime.h starts here
#ifndef OPENTIME_PROTO_H
#define OPENTIME_PROTO_H

#include <float.h>
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

#endif // OPENTIME_PROTO_H

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

//------- curve.h starts here

#ifndef OPENTIME_CURVE_H
#define OPENTIME_CURVE_H

typedef struct {
    OT_seconds time;
    OT_seconds value;
} OT_ControlPoint;

OT_ControlPoint OT_mul_cp(OT_ControlPoint cp, float val);
OT_ControlPoint OT_add_cp(OT_ControlPoint lh, OT_ControlPoint rh);
OT_ControlPoint OT_sub_cp(OT_ControlPoint lh, OT_ControlPoint rh);
bool OT_cp_equal(OT_ControlPoint lh, OT_ControlPoint rh);
OT_ControlPoint OT_lerp_cp(float u, OT_ControlPoint a, OT_ControlPoint b);
float value_at_time_between(float t, OT_ControlPoint fst, OT_ControlPoint snd);

typedef struct TimeCurveLinear {
    OT_ControlPoint* knots;
} TimeCurveLinear;

typedef struct {
    void* (*malloc)(size_t);
    void (*free)(void*);
} CurveAllocator;

typedef enum {
    EvalOK, EvalOutOfBounds } EvalResult;

typedef struct {
    float val;
    EvalResult err;
} EvalFloatResult;

typedef struct CurveInterface {
    CurveAllocator* alloc;
    void (*deinit)(struct CurveInterface*);

    TimeCurveLinear* (*tcl_init_with_knots)(struct CurveInterface*, OT_ControlPoint* knots, int count);
    TimeCurveLinear* (*tcl_init_identity)(struct CurveInterface*, float* times, int count);
    void (*tcl_deinit)(struct CurveInterface*, TimeCurveLinear*);
    EvalFloatResult (*tcl_eval)(struct CurveInterface*, TimeCurveLinear*, float t);
    int (*tcl_nearest_smaller_knot_index)(TimeCurveLinear*, float t);
    OT_ControlPoint[2] (*tcl_extents)(TimeCurveLinear*);
} CurveInterface;

CurveInterface* curve_interface_create(CurveAllocator*);


#endif // OPENTIME_CURVE_H

#define IMPL_OPENTIME_CURVE
#ifdef IMPL_OPENTIME_CURVE

#include "cvector.h"

TimeCurveLinear* ot_tcl_init_with_knots(CurveInterface* ci, OT_ControlPoint* knots_in, int count) {
    if (!ci || !ci->alloc)
        return NULL;

    OT_ControlPoint* knots = NULL;
    cvector_grow(knots, count);
    for (int i = 0; i < count; ++i) {
        cvector_push_back(knots, knots_in[i]);
    }
    TimeCurveLinear* tcl = (TimeCurveLinear*) ci->alloc->malloc(sizeof(TimeCurveLinear));
    tcl->knots = knots;
    return tcl;
}

TimeCurveLinear* ot_tcl_init_identity(CurveInterface* ci, float* times_in, int count) {
    if (!ci || !ci->alloc)
        return NULL;
    
    OT_ControlPoint* knots = NULL;
    cvector_grow(knots, count);
    for (int i = 0; i < count; ++i) {
        OT_ControlPoint p = { times_in[i], times_in[i] };
        cvector_push_back(knots, p);
    }
    TimeCurveLinear* tcl = (TimeCurveLinear*) ci->alloc->malloc(sizeof(TimeCurveLinear));
    tcl->knots = knots;
    return tcl;
}

void ot_tcl_deinit(CurveInterface* ci, TimeCurveLinear* tcl) {
    if (!tcl || !ci || !ci->alloc)
        return;

    cvector_free(tcl->knots);
    ci->alloc->free(tcl);
}

OT_ControlPoint[2] ot_tcl_extents(TimeCurveLinear* self) {
    OT_ControlPoint[2] result = { self->knots[0], self->knots[0] };
    int sz = cvector_size(self->knots);
    for (int i = 0; i < sz; ++i) {
        if (self->knots[i].value.t < result[0].value.t)
            result[0].value.t = self->knots[i].value.t;
        if (self->knots[i].time.t < result[0].time.t)
            result[0].time.t = self->knots[i].time.t;
        if (self->knots[i].value.t > result[1].value.t)
            result[1].value.t = self->knots[i].value.t;
        if (self->knots[i].time.t > result[1].time.t)
            result[1].time.t = self->knots[i].time.t;
    }
    return result;
}

EvalFloatResult ot_tcl_eval(CurveInterface* ci, TimeCurveLinear* tcl, float t) {
    int idx = ci->tcl_nearest_smaller_knot_index(tcl, t);
    if (idx < 0)
        return (EvalFloatResult) { 0, EvalOutOfBounds };
    return (EvalFloatResult) {
        value_at_time_between(t, tcl->knots[idx], tcl->knots[idx+1]),
                EvalOK };
}

int ot_tcl_nearest_smaller_knot_index(TimeCurveLinear* tcl, float t) {
    int sz = cvector_size(tcl->knots);
    if ((sz == 0) ||
       (t < tcl->knots[0].time.t) ||
       (t >= tcl->knots[sz - 1].time.t)) {
        return -1;
    }

    for (int i = 0; i < sz - 1; ++i) {
        if (tcl->knots[i].time.t <= t && t < tcl->knots[i+1].time.t)
            return i;
    }
    if (tcl->knots[sz - 1].time.t == t)
        return sz - 1;
    return -1;
}

/// project another curve through this one.  A curve maps 'time' to 'value'
/// parameters.  if curve self is v_self(t_self), and curve other is 
/// v_other(t_other) and other is being projected through self, the result
/// function is v_self(v_other(t_other)).  This maps the the v_other value
/// to the t_self parameter.
///
/// curve self:
/// 
/// v_self
/// |  /
/// | /
/// |/
/// +--- t_self
///
/// curve other:
///   
///  v_other
/// |      ,-'
/// |   ,-'
/// |,-'
/// +---------- t_other
///
/// @TODO finish this doc
///
TimeCurveLinear* ot_project_curve(
        /// curve being projected _through_
        TimeCurveLinear* self,
        /// curve being projected
        TimeCurveLinear* other
        ) 
{
    const other_bounds = other.extents();
    var other_copy = TimeCurveLinear.init(other.knots) catch unreachable;

    // find all knots in self that are within the other bounds
    for (self.knots)
        |self_knot| 
        {
            if (
                    _is_between(
                        self_knot.time,
                        other_bounds[0].value,
                        other_bounds[1].value
                        )
               ) {
                // @TODO: not sure how to do this in place?
                other_copy = other_copy.split_at_each_value(self_knot.time);
            }
        }

    // split other into curves where it goes in and out of the domain of self
    var curves_to_project = std.ArrayList(TimeCurveLinear).init(ALLOCATOR);
    const self_bounds = self.extents();
    var last_index: i32 = -10;
    var current_curve = std.ArrayList(ControlPoint).init(ALLOCATOR);
    for (other_copy.knots) 
        |other_knot, index| 
        {
            if (
                    self_bounds[0].time <= other_knot.value 
                    and other_knot.value <= self_bounds[1].time
               ) 
            {
                if (index != last_index+1) {
                    // curves of less than one point are trimmed, because they
                    // have no duration, and therefore are not modelled in our
                    // system.
                    if (current_curve.items.len > 1) {
                        curves_to_project.append(
                                TimeCurveLinear.init(
                                    current_curve.items
                                    ) catch unreachable
                                ) catch unreachable;
                    }
                    current_curve = std.ArrayList(ControlPoint).init(ALLOCATOR);
                }

                current_curve.append(other_knot) catch unreachable;
                last_index = @intCast(i32, index);
            }
        }
    if (current_curve.items.len > 1) {
        curves_to_project.append(
                TimeCurveLinear.init(current_curve.items) catch unreachable
                ) catch unreachable;
    }

    if (curves_to_project.items.len == 0) {
        return &[_]TimeCurveLinear{};
    }

    for (curves_to_project.items) 
        |crv| 
        {
            // project each knot
            for (crv.knots) 
                |knot, index| 
                {
                    // 2. evaluate grows a parameter to treat endpoint as in bounds
                    // 3. check outside of evaluate if it sits on a knot and use
                    //    the value rathe rthan computing
                    // 4. catch the error and call a different function or do a
                    //    check in that case
                    const value = self.evaluate(knot.value) catch (
                            if (self.knots[self.knots.len-1].time == knot.value) 
                            self.knots[self.knots.len-1].value 
                            else unreachable
                            );
                    crv.knots[index] = .{
                        .time = knot.time,
                            // this will error out trying to project the last endpoint
                            // .value = self.evaluate(knot.value) catch unreachable
                            .value = value
                    };
                }
        }

    return curves_to_project.items;
}


CurveInterface* curve_interface_create(CurveAllocator* alloc) {
    if (!alloc)
        return NULL;
    CurveInterface* ci = (CurveInterface*) malloc(sizeof(CurveInterface));
    ci->tcl_init_with_knots = ot_tcl_init_with_knots;
    ci->tcl_init_identity = ot_tcl_init_identity;
    ci->tcl_eval = ot_tcl_eval;
    ci->tcl_deinit = ot_tcl_deinit;
    ci->tcl_nearest_smaller_knot_index = ot_tcl_nearest_smaller_knot_index;
    ci->tcl_extents = ot_tcl_extents;
    return ci;
}



OT_ControlPoint OT_mul_cp(OT_ControlPoint cp, float val) {
    return (OT_ControlPoint) { 
        (OT_seconds) { cp.time.t * val }, 
        (OT_seconds) { cp.value.t * val } }; }
OT_ControlPoint OT_add_cp(OT_ControlPoint lh, OT_ControlPoint rh) {
    return (OT_ControlPoint) { 
        (OT_seconds) { lh.time.t + rh.time.t }, 
        (OT_seconds) { lh.value.t + rh.value.t }}; }
OT_ControlPoint OT_sub_cp(OT_ControlPoint lh, OT_ControlPoint rh) {
    return (OT_ControlPoint) { 
        (OT_seconds) { lh.time.t - rh.time.t },
        (OT_seconds) { lh.value.t - rh.value.t }}; }
bool OT_cp_equal(OT_ControlPoint lh, OT_ControlPoint rh) {
    return (lh.time.t == rh.time.t) && (lh.value.t == rh.value.t); }

OT_ControlPoint OT_lerp_cp(float u, OT_ControlPoint a, OT_ControlPoint b) {
    OT_ControlPoint result = {
        { a.time.t * (1.f - u) + b.time.t * u },
        { a.value.t * (1.f - u) + b.value.t * u } };
    return result;
}
float lerp_f(float u, float a, float b) {
    return a * (1.f - u) + b * u;
}
float inv_lerp_f(float u, float a, float b) {
    if (a == b)
        return a;
    return (u - a) / (b - a);
}

float value_at_time_between(float t, OT_ControlPoint fst, OT_ControlPoint snd) {
    float u = inv_lerp_f(t, fst.time.t, snd.time.t);
    return lerp_f(u, fst.value.t, snd.value.t);
}

// evaluate a 1d bezier whose first point is 0.
float _bezier0(float unorm, float p2, float p3, float p4)
{
    const float p1 = 0.0f;
    const float z = unorm;
    const float z2 = z * z;
    const float z3 = z2 * z;

    const float zmo = z - 1.0f;
    const float zmo2 = zmo * zmo;
    const float zmo3 = zmo2 * zmo;

    return (p4 * z3) 
        - (p3 * (3.0f * z2 * zmo))
        + (p2 * (3.0f * z * zmo2))
        - (p1 * zmo3);
}

//
// Given x in the interval [0, p3], and a monotonically nondecreasing
// 1-D Bezier curve, B(u), with control points (0, p1, p2, p3), find
// u so that B(u) == x.
//

float _findU(float x, float p1, float p2, float p3)
{
    const float MAX_ABS_ERROR = FLT_EPSILON * 2.0f;
    const int MAX_ITERATIONS = 45;

    if (x <= 0.f) {
        return 0.f;
    }

    if (x >= p3) {
        return 1.f;
    }

    float _u1 = 0.f;
    float _u2 = 0.f;
    float x1 = -x; // same as: bezier0 (0, p1, p2, p3) - x;
    float x2 = p3 - x; // same as: bezier0 (1, p1, p2, p3) - x;

    {
        const float _u3 = 1.0f - x2 / (x2 - x1);
        const float x3 = _bezier0(_u3, p1, p2, p3) - x;

        if (x3 == 0.f)
            return _u3;

        if (x3 < 0.f)
        {
            if (1.0f - _u3 <= MAX_ABS_ERROR) {
                if (x2 < -x3)
                    return 1.0f;
                return _u3;
            }

            _u1 = 1.0f;
            x1 = x2;
        }
        else
        {
            _u1 = 0.0f;
            x1 = x1 * x2 / (x2 + x3);

            if (_u3 <= MAX_ABS_ERROR) {
                if (-x1 < x3)
                    return 0.0f;
                return _u3;
            }
        }
        _u2 = _u3;
        x2 = x3;
    }

    int i = MAX_ITERATIONS - 1;

    while (i > 0)
    {
        i -= 1;
        const float _u3 = _u2 - x2 * ((_u2 - _u1) / (x2 - x1));
        const float x3 = _bezier0 (_u3, p1, p2, p3) - x;

        if (x3 == 0)
            return _u3;

        if (x2 * x3 <= 0)
        {
            _u1 = _u2;
            x1 = x2;
        }
        else
        {
            x1 = x1 * x2 / (x2 + x3);
        }

        _u2 = _u3;
        x2 = x3;

        if (_u2 > _u1)
        {
            if (_u2 - _u1 <= MAX_ABS_ERROR)
                break;
        }
        else
        {
            if (_u1 - _u2 <= MAX_ABS_ERROR)
                break;
        }
    }

    if (x1 < 0)
        x1 = -x1;
    if (x2 < 0)
        x2 = -x2;

    if (x1 < x2)
        return _u1;
    return _u2;
}

//
// Given x in the interval [p0, p3], and a monotonically nondecreasing
// 1-D Bezier curve, B(u), with control points (p0, p1, p2, p3), find
// u so that B(u) == x.
//
float findU(float x, float p0, float p1, float p2, float p3)
{
    return _findU(x - p0, p1 - p0, p2 - p0, p3 - p0);
}



#ifdef TESTING

void test_control_points() {
    {
        // add
        OT_ControlPoint cp1 = { { 0 }, { 10 } };
        OT_ControlPoint cp2 = { { 20 }, { -10 } };
        OT_ControlPoint test = {{ 20 }, { 0 } };
        OT_ControlPoint result = OT_add_cp(cp1, cp2);
        bool success = OT_cp_equal(test, result);
    }
    {
        // sub
        OT_ControlPoint cp1 = { { 0 }, { 10 } };
        OT_ControlPoint cp2 = { { 20 }, { -10 } };
        OT_ControlPoint test = {{ -20 }, { 0 } };
        OT_ControlPoint result = OT_add_cp(cp1, cp2);
        bool success = OT_cp_equal(test, result);
    }
    {
        // mul 
        OT_ControlPoint cp1 = { { 0 }, { 10 } };
        float scale = -10;
        OT_ControlPoint test = {{ 0 }, { -100 } };
        OT_ControlPoint result = OT_mul_cp(cp1, scale);
        bool success = OT_cp_equal(test, result);
    }
    {
        // lerp
        OT_ControlPoint fst = {{ 0 }, { 0 }};
        OT_ControlPoint snd = {{ 1 }, { 1 }};
        bool success;
        success = OT_lerp_cp(0.00f, fst, snd).value.t == 0.00f;
        success = OT_lerp_cp(0.25f, fst, snd).value.t == 0.25f;
        success = OT_lerp_cp(0.50f, fst, snd).value.t == 0.50f;
        success = OT_lerp_cp(0.75f, fst, snd).value.t == 0.75f;
        success = OT_lerp_cp(0.00f, fst, snd).time.t == 0.00f;
        success = OT_lerp_cp(0.25f, fst, snd).time.t == 0.25f;
        success = OT_lerp_cp(0.50f, fst, snd).time.t == 0.50f;
        success = OT_lerp_cp(0.75f, fst, snd).time.t == 0.75f;
    }
    {
        bool success;
        success = findU(0.f, 0.f, 1.f, 2.f, 3.f) == 0.f;
        // out of range values are clamped
        success = findU(-1.f, 0.f, 1.f, 2.f, 3.f) == 0.f;
        success = findU(1.f, 0.f, 1.f, 2.f, 3.f) == 1.f;
    }
 }

#endif // TESTING

#endif // IMPL_OPENTIME_CURVE


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
    bool reset_transform;
    OT_TimeInterval bounds;
    //OT_TImeCurve mapping;
} IntervalOid;
const IntervalOid IntervalOid_default = {{0}, {0}, {0},
    { {0}, 1.f }, false,
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



