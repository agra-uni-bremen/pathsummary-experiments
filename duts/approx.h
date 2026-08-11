#pragma once
#include <systemc>
#include <tlm_utils/simple_target_socket.h>
#include "core/common/irq_if.h"
#include "util/tlm_map.h"

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/math/statistics/linear_regression.hpp>

// should be defined in the testbench, depends on the range of input/output numbers (smaller numbers: larger factor)
#ifndef BASE
#define BASE 100
#endif
#ifndef FACTOR
#define FACTOR 1
#endif

// must define accuracy (=amount of segments) here, bc I can't use template argument for #reps in BOOST_PP_REPEAT...
#define DUMBHACK BUILD_ACC
// hack for float point representation in integer (basically fixed point)
static constexpr int skalierung = BASE*FACTOR;

#define FIXEDPOINT(I) ((I)*(skalierung))
#define REAL(I) ((I)/(double)(skalierung))
#define REAL_INTERNAL(I) ((I)/(skalierung))

#if FAULT_APPROX==2
#define OUTER(z, n, data) \
    if(x1_int < (lower+((n+data*256)+1)*secsize) && ((n+data*256)<accuracy-1||approx_rounds!=BUILD_IT)) { \
        res = approximated_internal##n##_##data(x1_int);                  \
    } else
#else
#define OUTER(z, n, data) \
    if(x1_int < (lower+((n+data*256)+1)*secsize)) { \
        res = approximated_internal##n##_##data(x1_int);                  \
    } else
#endif

#if FAULT_APPROX==3
#define GEN(z,n,data) \
    uint32_t approximated_internal##n##_##data(int x1) { \
        static int m;                              \
        static int x0 = (n+data*256)==0 ? lower : (lower + (n+data*256)*secsize)-step; \
        static int b;              \
        if(init) {    \
            double l = REAL(lower) + (n+data*256)*REAL(secsize);    \
            double u = REAL(lower) + ((n+data*256)+1)*REAL(secsize);        \
            std::vector<double> xs; \
            std::vector<double> ys; \
            for(double i=l;i<u;i=i+REAL(step)) {                \
                xs.push_back(i);                                \
                ys.push_back(F(i));                            \
            }                                                        \
            auto [b1,m1] = simple_ordinary_least_squares(xs,ys); \
            m=FIXEDPOINT(m1);                         \
            b = (n+data*256)==0 ? FIXEDPOINT(F(l)) : init_current;          \
            init_current = REAL(m*(FIXEDPOINT(u)-x0))+b;          \
            return 0;              \
        }                   \
        int temp1 = 0; \
        if((n+data*256)>=accuracy-1&&approx_rounds==BUILD_IT)              \
        	temp1 = REAL_INTERNAL(m*(x1-x0))-b;    \
        else              \
        	temp1 = REAL_INTERNAL(m*(x1-x0))+b;    \
        return temp1;                  \
    }
#elif FAULT_APPROX==4
#define GEN(z,n,data) \
    uint32_t approximated_internal##n##_##data(int x1) { \
        static int m;                              \
        static int x0 = (n+data*256)==0 ? lower : (lower + (n+data*256)*secsize)-step; \
        static int b;              \
        if(init) {    \
            double l = REAL(lower) + (n+data*256)*REAL(secsize);    \
            double u = REAL(lower) + ((n+data*256)+1)*REAL(secsize);        \
            std::vector<double> xs; \
            std::vector<double> ys; \
            for(double i=l;i<u;i=i+REAL(step)) {                \
                xs.push_back(i);                                \
                ys.push_back(F(i));                            \
            }                                                        \
            auto [b1,m1] = simple_ordinary_least_squares(xs,ys); \
            m=FIXEDPOINT(m1);                         \
            b = (n+data*256)==0 ? FIXEDPOINT(F(l)) : init_current;          \
            init_current = REAL(m*(FIXEDPOINT(u)-x0))+b;          \
            return 0;              \
        }                   \
        int temp1 = 0; \
        if((n+data*256)>=accuracy-1&&approx_rounds==BUILD_IT)              \
        	temp1 = REAL_INTERNAL(m*(x1))+b;    \
        else              \
        	temp1 = REAL_INTERNAL(m*(x1-x0))+b;    \
        return temp1;                  \
    }
#else
#define GEN(z,n,data) \
    uint32_t approximated_internal##n##_##data(int x1) { \
        static int m;                              \
        static int x0 = (n+data*256)==0 ? lower : (lower + (n+data*256)*secsize)-step; \
        static int b;              \
        if(init) {    \
            double l = REAL(lower) + (n+data*256)*REAL(secsize);    \
            double u = REAL(lower) + ((n+data*256)+1)*REAL(secsize);        \
            std::vector<double> xs; \
            std::vector<double> ys; \
            for(double i=l;i<u;i=i+REAL(step)) {                \
                xs.push_back(i);                                \
                ys.push_back(F(i));                            \
            }                                                        \
            auto [b1,m1] = simple_ordinary_least_squares(xs,ys); \
            m=FIXEDPOINT(m1);                         \
            b = (n+data*256)==0 ? FIXEDPOINT(F(l)) : init_current;          \
            init_current = REAL(m*(FIXEDPOINT(u)-x0))+b;          \
            return 0;              \
        }                   \
        int temp1 = REAL_INTERNAL(m*(x1-x0))+b;        \
        return temp1;                  \
    }
#endif

#define INIT(z,n,data) \
    approximated_internal##n##_##data(0);

#if DUMBHACK <= 256
#define REPEATWRAPPER_INIT(count) BOOST_PP_REPEAT(count,INIT,0)
#define REPEATWRAPPER_GEN(count) BOOST_PP_REPEAT(count,GEN,0)
#define REPEATWRAPPER_IF(count) BOOST_PP_REPEAT(count,OUTER,0)
#elif DUMBHACK == 512
#define REPEAT_INNER_INIT(z,n,data) BOOST_PP_REPEAT(256,INIT,n)
#define REPEAT_INNER_GEN(z,n,data) BOOST_PP_REPEAT(256,GEN,n)
#define REPEAT_INNER_IF(z,n,data) BOOST_PP_REPEAT(256,OUTER,n)
#define REPEATWRAPPER_INIT(count) BOOST_PP_REPEAT(2,REPEAT_INNER_INIT,0)
#define REPEATWRAPPER_GEN(count) BOOST_PP_REPEAT(2,REPEAT_INNER_GEN,0)
#define REPEATWRAPPER_IF(count) BOOST_PP_REPEAT(2,REPEAT_INNER_IF,0)
#elif DUMBHACK == 1024
#define REPEAT_INNER_INIT(z,n,data) BOOST_PP_REPEAT(256,INIT,n)
#define REPEAT_INNER_GEN(z,n,data) BOOST_PP_REPEAT(256,GEN,n)
#define REPEAT_INNER_IF(z,n,data) BOOST_PP_REPEAT(256,OUTER,n)
#define REPEATWRAPPER_INIT(count) BOOST_PP_REPEAT(4,REPEAT_INNER_INIT,0)
#define REPEATWRAPPER_GEN(count) BOOST_PP_REPEAT(4,REPEAT_INNER_GEN,0)
#define REPEATWRAPPER_IF(count) BOOST_PP_REPEAT(4,REPEAT_INNER_IF,0)
#elif DUMBHACK == 2048
#define REPEAT_INNER_INIT(z,n,data) BOOST_PP_REPEAT(256,INIT,n)
#define REPEAT_INNER_GEN(z,n,data) BOOST_PP_REPEAT(256,GEN,n)
#define REPEAT_INNER_IF(z,n,data) BOOST_PP_REPEAT(256,OUTER,n)
#define REPEATWRAPPER_INIT(count) BOOST_PP_REPEAT(8,REPEAT_INNER_INIT,0)
#define REPEATWRAPPER_GEN(count) BOOST_PP_REPEAT(8,REPEAT_INNER_GEN,0)
#define REPEATWRAPPER_IF(count) BOOST_PP_REPEAT(8,REPEAT_INNER_IF,0)
#else
#warning "INVALID SEGMENT AMOUNT!"
#endif

using boost::math::statistics::simple_ordinary_least_squares;

/// piecewise linear approximation
/// input and template parameters have to be scaled using FIXEDPOINT macro
/// output can be turned into double using REAL macro
template<double (*F)(double), uint32_t accuracy, int lower, int upper, int step>
struct approx : public sc_core::sc_module {
	//static constexpr double accuracy_sk = REAL(accuracy);
	static_assert(lower < upper, "lower bound has to be smaller than upper bound!");
	//static_assert(upper-lower >= accuracy_sk, "not enough integer values in interval for desired amount of sections");
	//static_assert((upper-lower)%accuracy==0, "cannot divide intervall into the desired amount of sections. pick divisible numbers");
	static constexpr int secsize = (upper-lower)/accuracy;
	uint32_t x=0; // input value x
	uint32_t result=0; // result
	uint32_t ready=0; // is result ready to read?

	tlm_utils::simple_target_socket<approx> tsock;
	vp::map::LocalRouter router = {"approx"};

	sc_core::sc_event e_run;
	sc_core::sc_event irq;
	sc_core::sc_time clock_cycle;

	bool init = true;
	int init_current = 0;

#ifdef FAULT_APPROX
	unsigned approx_rounds=0;
#endif

	SC_HAS_PROCESS(approx);
	approx(sc_core::sc_module_name name) : sc_module(name) {
		clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
		tsock.register_b_transport(this, &approx::transport);

		auto &regs = router
				.add_register_bank({})
				.register_handler(this, &approx::register_access_callback);

		regs.add_register({0x0,&x});
		regs.add_register({0x4,&result, vp::map::read_only});

		SC_THREAD(run);

		// initialize approximation functions
		REPEATWRAPPER_INIT(DUMBHACK)
		init = false;
		init_current = 0;
	}

	REPEATWRAPPER_GEN(DUMBHACK)

	uint32_t approximated(uint32_t x1) {
		uint32_t res=0;
		int x1_int = (int)x1;
		if(x1_int>=upper||x1_int<lower) {
			res = 0;
		} else {
			ready = 1;
			REPEATWRAPPER_IF(DUMBHACK)
			{} // trailing {} for last, empty, "else"
		}
		return res;
	}

	void register_access_callback(const vp::map::register_access_t &r) {
		if(r.write && r.vptr == &result) {
			assert(false && "result register is read-only");
			return;
		}

#if FAULT_APPROX==1
		if(approx_rounds<BUILD_IT-1)
#endif
			r.fn();

		if (r.write && (r.vptr == &x)) { // if new input is written, calculation is called
			result = 0;
			ready = 0;
			e_run.notify(clock_cycle);
		}
		if(r.read && (r.vptr == &result)) { // is result is read, reset ready
			ready = 0;
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		router.transport(trans, delay);
	}


	void run() {
		while(true) {
			sc_core::wait(e_run);
#ifdef FAULT_APPROX
			++approx_rounds;
#endif
			result = approximated(x);
			irq.notify(clock_cycle);
		}
	}
};

