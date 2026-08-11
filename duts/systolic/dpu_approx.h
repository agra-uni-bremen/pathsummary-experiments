#pragma once

#include <systemc>
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
    if(x1_int < (lower+((n+data*256)+1)*secsize) && ((n+data*256)<accuracy-1||dpu_num!=(WIDTH*HEIGHT)-1)) { \
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
        if((n+data*256)>=accuracy-1&&dpu_num==(WIDTH*HEIGHT)-1)              \
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
        if((n+data*256)>=accuracy-1&&dpu_num==(WIDTH*HEIGHT)-1)              \
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

template<double (*F)(double), uint32_t accuracy, int lower, int upper, int step>
struct dpu_approx : public sc_core::sc_module {
	static_assert(lower < upper, "lower bound has to be smaller than upper bound!");
	static constexpr int secsize = (upper-lower)/accuracy;
	sc_core::sc_in<bool> clk;
	sc_core::sc_in<bool> approx_start;
	sc_core::sc_out<bool> approx_done;

	sc_core::sc_in<uint32_t> in_top;
	sc_core::sc_in<uint32_t> in_left;
	sc_core::sc_out<uint32_t> out_bottom;
	sc_core::sc_out<uint32_t> out_right;

	uint32_t sum=0;
	uint32_t sum_approx=0;
    uint32_t ready=0;

	// tmp stuff for approx
	bool init = true;
	int init_current = 0;

#if defined(FAULT_SYSTOLIC) || defined(FAULT_APPROX)
    unsigned dpu_num=0;
#endif
#ifdef FAULT_SYSTOLIC
	bool last_sum=false;
#endif

	SC_HAS_PROCESS(dpu_approx);
	dpu_approx(sc_core::sc_module_name name) : sc_module(name){
		SC_METHOD(calculate);
		sensitive << clk.pos();
		SC_METHOD(approx);
		sensitive << approx_start;
		dont_initialize();

		// initialize approximation functions
		REPEATWRAPPER_INIT(DUMBHACK)
		init = false;
		init_current = 0;
	}

	dpu_approx() : dpu_approx("dpu_approx") {} // TODO name mit counter

	REPEATWRAPPER_GEN(DUMBHACK)

	void calculate() {
#if FAULT_SYSTOLIC==2
		int sum_int;
		if(dpu_num==(WIDTH*HEIGHT)-1&&last_sum)
			sum_int = (int)in_top.read()/(int)in_left.read();
		else
			sum_int = (int)in_top.read()*(int)in_left.read();
#else
		int sum_int = (int)in_top.read()*(int)in_left.read();
#endif
#if FAULT_SYSTOLIC==1
		if(!last_sum)
#elif FAULT_SYSTOLIC==3
		if(dpu_num==(WIDTH*HEIGHT)-1&&last_sum)
			sum = REAL_INTERNAL(sum_int);
		else
#elif FAULT_SYSTOLIC==4
		if(dpu_num==(WIDTH*HEIGHT)-1&&last_sum)
			sum -= REAL_INTERNAL(sum_int);
		else
#endif
		sum += REAL_INTERNAL(sum_int);
		out_bottom.write(in_top.read());
		out_right.write(in_left.read());
	}

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

	void approx() {
		if(approx_start.read()) {
#if FAULT_APPROX==1
			if(dpu_num==(WIDTH*HEIGHT)-1)
				sum_approx = approximated(0);
			else
#endif
			sum_approx = approximated(sum);
			approx_done.write(true);
//			INFO(std::cout << "[dpu] " << sc_core::sc_time_stamp() << ", calculated for sum " << std::to_string(REAL((int)sum)) << " approx value " << std::to_string(REAL((int)sum_approx)) << std::endl;)
		} else {
			sum_approx = 0;
			sum = 0;
			ready = 0;
			approx_done.write(false);
		}
	}
};

