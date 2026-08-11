#include <klee_conf.h>

#ifndef BUILD_ACC
#define BUILD_ACC 64
#endif

#define BASE 100
#ifdef NEED_FP
#define FACTOR 100
#define STEP 0.001
#else
#define FACTOR 1
#define STEP 0.5
#endif

#include <approx.h>
#include <systemc.h>
#include <tlm_utils/simple_initiator_socket.h>

#include <iostream>
#include <fstream>

#ifndef __UCLIBC__
#warning "Not using uclibc...."
#endif

#ifndef BUILD_U
#define BUILD_U 1024
#endif
#ifndef BUILD_L
#define BUILD_L 0
#endif
#ifndef BUILD_IT
#define BUILD_IT 2
#endif


static constexpr uint32_t addrX = 0x00;
static constexpr uint32_t addrRes = 0x04;

static constexpr uint32_t acc = DUMBHACK; // can't use template argument for boost-repeat?
static constexpr double lower_rl = BUILD_L;
static constexpr int lower = FIXEDPOINT(BUILD_L);
static constexpr double upper_rl = BUILD_U;
static constexpr int upper = FIXEDPOINT(BUILD_U);
static constexpr double step_rl = STEP;
static constexpr int step = FIXEDPOINT(STEP);

double fun1(double x) {
	return 128*sin(x/128)*std::exp(-x/512);
}

double fun2(double x) {
	return 128 * sin(x/32) * cos(x/512);
}

double sigmoid(double x) {
	return 1/(1+std::exp(-x));
}

double tanh(double x) {
	return (std::exp(x)-std::exp(-x))/(std::exp(x)+std::exp(-x));
}

double softsign(double x) {
	return x/1+std::abs(x);
}

double gaussian(double x) {
	return std::exp(-std::pow(x,2));
}

double silu(double x) {
	return x/(1+std::exp(-x));
}

double sinc(double x) {
	if(x<0+step_rl && x>0-step_rl) return 1;
	return sin(x)/x;
}

double cosFun(double x) {
	return 2*cos(2*M_PI*2*x);
}

double expImpulse(double x) {
	return (1/(double)(2*2))*std::exp(-std::abs(x)/2);
}


template<double (*F)(double)>
void tlm_write(approx<F,acc,lower,upper,step> &tlm, uint32_t address, uint8_t *val) {
	sc_core::sc_time delay;
	tlm::tlm_generic_payload pl;
	pl.set_write();
	pl.set_address(address); // valid
	pl.set_data_length(sizeof(uint32_t));
	pl.set_data_ptr(val);
	tlm.transport(pl, delay);
}

template<double (*F)(double)>
struct functional_tlm_basic : public sc_core::sc_module {
	approx<F,acc,lower,upper,step> tlm;

	tlm_utils::simple_initiator_socket<functional_tlm_basic> isock;

	bool test_done = false;

	SC_HAS_PROCESS(functional_tlm_basic);
	functional_tlm_basic(sc_core::sc_module_name name, bool t=false) : sc_module(name), tlm("approx") {
		isock.bind(tlm.tsock);
		if(t) {
			SC_THREAD(test);
		}
		else {
			SC_THREAD(run);
		}
	}

	void run() {
		int xcur;
		int xlast;
		uint32_t ylast;
		for(unsigned i=0;i<BUILD_IT;++i) {
			xcur = klee_int("x");
			// for concrete values, FIXEDPOINT has to be applied. however, xcur can already be every possible int
			// value, so that should not matter here
			tlm_write<F>(tlm, addrX, reinterpret_cast<uint8_t *>(&xcur));
			auto before = sc_core::sc_time_stamp();
			assert(tlm.ready == 0);
			wait(tlm.irq);
			auto after = sc_core::sc_time_stamp();
			assert(after-before == sc_core::sc_time(20, sc_core::SC_NS));

			assert(tlm.ready == (xcur < lower || xcur >= upper) ? 0 : 1);
			if (i > 0 && xcur == xlast) {
				assert(tlm.result == ylast);
			}
			xlast = xcur;
			ylast = tlm.result;
		}

		test_done = true;
	}

	// dummy function for debugging
	void test() {
#ifdef USE_KLEE
		int x = klee_int("x");
//		x = FIXEDPOINT(x);
#else
		unsigned amount = (unsigned)((upper_rl-lower_rl)*(1/step_rl));
		uint32_t ress[amount];
		unsigned counter=0;
		for(int x=lower;x<upper;x=x+step) {
#endif
		tlm_write<F>(tlm, addrX, reinterpret_cast<uint8_t *>(&x));
		auto before = sc_core::sc_time_stamp();
		wait(tlm.irq);
		auto after = sc_core::sc_time_stamp();
		assert(after-before == sc_core::sc_time(20, sc_core::SC_NS));

#ifndef USE_KLEE
		ress[counter] = tlm.result;
		++counter;
		}
		std::ofstream file;
		std::string filename = std::to_string(lower)+"-"+std::to_string(upper)+"_"+std::to_string(acc)+".csv";
		file.open(filename);
		for(unsigned x=0;x<amount;++x) {
			file << REAL((int)ress[x]);
			if(x!=amount-1) file << ",";
		}
		file.close();
#endif

		// sin kann ich in symex nicht ausführen: silently concretizing
		test_done = true;
	}
};


int sc_main(int argc, char* argv[])
{
	if(argc >= 2) {
		bool test = argc == 3 && strcmp(argv[2], "test") == 0;
		if(strcmp(argv[1], "f1") == 0) {
			functional_tlm_basic<fun1> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "f2") == 0) {
			functional_tlm_basic<fun2> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "sigmoid") == 0) {
			functional_tlm_basic<sigmoid> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "tanh") == 0) {
			functional_tlm_basic<tanh> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "softsign") == 0) {
			functional_tlm_basic<softsign> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "silu") == 0) {
			functional_tlm_basic<silu> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "gaussian") == 0) {
			functional_tlm_basic<gaussian> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "cos") == 0) {
			functional_tlm_basic<cosFun> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "exp") == 0) {
			functional_tlm_basic<expImpulse> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else if(strcmp(argv[1], "sinc") == 0) {
			functional_tlm_basic<sinc> testbench("name", test);
			sc_start();
			assert(testbench.test_done && "test case unexpectedly did not finish!");
		} else
			INFO(std::cout << "Invalid test " << argv[1] << " given." << std::endl);
	} else {
		INFO(std::cout << "No test given." << std::endl);
		//Test info output
		sc_core::sc_time a(1002, sc_core::SC_MS), b (1, sc_core::SC_SEC), c (1, sc_core::SC_FS);
		INFO(std::cout << a.to_string() << " + " << b.to_string() << " = " << (a+b).to_string() << std::endl);
		//INFO(std::cout << c.to_string() << " in default units is " << c.to_default_time_units() << " " << sc_core::unit_to_string(sc_core::default_time_unit) << std::endl);
		//INFO(std::cout << "Number of registered transports: " << transports.size() << std::endl);
		//sc_core::Simcontext::get().printInfo();
	}

	INFO(std::cout << "finished at " << sc_core::sc_time_stamp() << std::endl);
	return 0;
}
