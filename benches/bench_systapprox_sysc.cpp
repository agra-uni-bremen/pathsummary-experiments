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

#include <systolic/systolic.h>
#include <systemc.h>

#ifndef WIDTH
#define WIDTH 2
#endif
#ifndef HEIGHT
#define HEIGHT 2
#endif
#ifndef TESTSIZE
#define TESTSIZE 2
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

template<unsigned Width, unsigned Height, unsigned TestSize, double (*F)(double)>
struct functional_test_basic : public sc_core::sc_module {
	systolic<Width, Height, F, acc, lower, upper, step> dut;
	sc_core::sc_clock &clock;

	sc_core::sc_vector<sc_core::sc_signal<uint32_t>> ins_top;
	sc_core::sc_vector<sc_core::sc_signal<uint32_t>> ins_left;
	sc_core::sc_vector<sc_core::sc_signal<uint32_t>> outs_bottom;
	sc_core::sc_vector<sc_core::sc_signal<uint32_t>> outs_right;
	sc_core::sc_signal<bool> approx_start;
	sc_core::sc_signal<bool> approx_done;

	SC_HAS_PROCESS(functional_test_basic);
	functional_test_basic(sc_core::sc_module_name name, sc_clock& clock) : sc_module(name), clock(clock), dut("systolic"),
																											ins_top("signals_top", Width), ins_left("signals_left", Height), outs_bottom("signals_bottom", Width),
																											outs_right("signals_right", Height)
	{
		dut.clk(clock);
		dut.approx_done(approx_done);
		dut.approx_start(approx_start);
		approx_start.write(false);
		for(unsigned i=0;i<Height;++i) {
			dut.ins_left[i](ins_left[i]);
			dut.outs_right[i](outs_right[i]);
		}
		for (unsigned j = 0; j < Width; ++j) {
			dut.ins_top[j](ins_top[j]);
			dut.outs_bottom[j](outs_bottom[j]);
		}

		SC_THREAD(send);
		sensitive << clock.posedge_event();
	}

	void send() {
		// originaldaten
		int data_left[Height][TestSize];
		int data_top[TestSize][Width];

		// padded
		unsigned pad_left = Height-1;
		unsigned total_left = TestSize+pad_left;
		int padded_left[Height][total_left];
		unsigned pad_top = Width-1;
		unsigned total_top = TestSize+pad_top;
		int padded_top[total_top][Width];

		uint32_t sum_prev_it = 0;
		uint32_t sum_approx_prev_it = 0;
		for(unsigned inputs=0;inputs<BUILD_IT;++inputs) {
#ifndef USE_KLEE
		int dc=0;
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<TestSize;++j) {
				++dc;
				data_left[i][j]=FIXEDPOINT(1);
			}
		}
		for(unsigned i=0;i<TestSize;++i) {
			for(unsigned j=0;j<Width;++j) {
				++dc;
				data_top[i][j]=(upper/2)-(step*dc);
			}
		}
		// print
		std::cout << "data left" << std::endl;
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<TestSize;++j) {
				std::cout << data_left[i][j] << "  ";
			}
			std::cout << std::endl;
		}
		std::cout << "data top" << std::endl;
		for(unsigned i=0;i<TestSize;++i) {
			for(unsigned j=0;j<Width;++j) {
				std::cout << data_top[i][j] << "  ";
			}
			std::cout << std::endl;
		}
#else
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<TestSize;++j) {
				data_left[i][j] = klee_int("dleft");
			}
		}
		for(unsigned i=0;i<TestSize;++i) {
			for(unsigned j=0;j<Width;++j) {
				data_top[i][j] = klee_int("dtop");
			}
		}
#endif

		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<total_left;++j) {
				if(j<(pad_left-i)||j>=(total_left-i))
					padded_left[i][j]=0;
				else
					padded_left[i][j]=data_left[i][j-(pad_left-i)];
			}
		}
		for(unsigned i=0;i<total_top;++i) {
			for(unsigned j=0;j<Width;++j) {
				if(i<(pad_top-j)||i>=(total_top-j))
					padded_top[i][j]=0;
				else
					padded_top[i][j]=data_top[i-(pad_top-j)][j];
			}
		}
#ifndef USE_KLEE
		// print
		std::cout << "padded left" << std::endl;
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<total_left;++j) {
				std::cout << padded_left[i][j] << "  ";
			}
			std::cout << std::endl;
		}
		std::cout << "padded top" << std::endl;
		for(unsigned i=0;i<total_top;++i) {
			for(unsigned j=0;j<Width;++j) {
				std::cout << padded_top[i][j] << "  ";
			}
			std::cout << std::endl;
		}
#endif

		INFO(std::cout << "---------------------" << std::endl;)
		assert(!approx_done.read());
		wait();
		int top_i=total_top-1;
		int left_i=total_left-1;
		while(top_i>=0||left_i>=0) {
			if(left_i>=0) {
				for (unsigned j = 0; j < Height; ++j) {
					ins_left[j] = padded_left[j][left_i];
				}
				--left_i;
			} else {
				for (unsigned j = 0; j < Height; ++j) {
					ins_left[j] = 0;
				}
			}
			if(top_i>=0) {
				for (unsigned j = 0; j < Width; ++j) {
					ins_top[j] = padded_top[top_i][j];
				}
				--top_i;
			} else {
				for (unsigned j = 0; j < Width; ++j) {
					ins_top[j] = 0;
				}
			}
			wait();
		}

		// reset signals so that systolic array does not read old values again
		for (unsigned j=0; j<Height; ++j)
			ins_left[j] = 0;
		for (unsigned j=0; j<Width; ++j)
			ins_top[j] = 0;

		// wait for calculation to be done
		for(unsigned i=0;i<(Width>Height?Height-1:Width-1);++i) {
#ifdef FAULT_SYSTOLIC
			if(i==(Width>Height?Height-2:Width-2)) {
				for(unsigned a=0;a<Height;++a) {
					for(unsigned b=0;b<Width;++b) {
						dut.dpus[a][b].last_sum=true;
					}
				}
			}
#endif
			wait();
		}
#ifdef FAULT_SYSTOLIC
			for(unsigned a=0;a<Height;++a) {
			for(unsigned b=0;b<Width;++b) {
				dut.dpus[a][b].last_sum=false;
			}
		}
#endif

		INFO(std::cout << "check sum!" << std::endl;)
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<Width;++j) {
				uint32_t expected=0;
				for(unsigned x=0;x<TestSize;++x) {
					int sum_int = (int)data_left[i][x] * (int)data_top[x][j];
					expected += REAL_INTERNAL(sum_int);
				}
				INFO(std::cout << "exp: " << REAL((int)expected) << ", recv: " << REAL((int)dut.dpus[i][j].sum) << std::endl;)
				assert(expected == dut.dpus[i][j].sum);
			}
		}
		approx_start.write(true);
		wait(clock.negedge_event());
		// done in all dpus?
		assert(approx_done.read());
		uint32_t sum_prev_dpu=0;
		uint32_t sum_approx_prev_dpu=0;
		for(unsigned i=0;i<Height;++i) {
			for (unsigned j = 0; j < Width; ++j) {
				assert(dut.dpus[i][j].ready == ((int)dut.dpus[i][j].sum < lower || (int)dut.dpus[i][j].sum >= upper) ? 0 : 1);
				if((i>0 || j>0) && dut.dpus[i][j].sum == sum_prev_dpu) {
					assert(dut.dpus[i][j].sum_approx == sum_approx_prev_dpu);
				}
				if(inputs>0 && dut.dpus[i][j].sum == sum_prev_it) {
					assert(dut.dpus[i][j].sum_approx == sum_approx_prev_it);
				}
				sum_prev_dpu = dut.dpus[i][j].sum;
				sum_approx_prev_dpu = dut.dpus[i][j].sum_approx;
			}
		}
		sum_prev_it = sum_prev_dpu;
		sum_approx_prev_it = sum_approx_prev_dpu;
		approx_start.write(false);
		wait();
//		assert(!approx_done.read());
//		for(unsigned i=0;i<Height;++i) {
//			for (unsigned j = 0; j < Width; ++j) {
//				assert(dut.dpus[i][j].sum == 0);
//				assert(dut.dpus[i][j].sum_approx == 0);
//			}
//		}
		}
		sc_stop();
	}
};

int sc_main(int argc, char* argv[])
{
	sc_report_handler::set_actions(SC_ID_INSTANCE_EXISTS_,
								   SC_DO_NOTHING);
	sc_clock clk{"clk", sc_core::sc_time(20, sc_core::SC_NS)};

	if(argc == 2) {
		if(strcmp(argv[1], "f1") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, fun1> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "f2") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, fun2> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "sigmoid") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, sigmoid> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "tanh") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, tanh> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "softsign") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, softsign> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "silu") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, silu> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "gaussian") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, gaussian> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "cos") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, cosFun> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "exp") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, expImpulse> tb("name", clk);
			sc_start();
		} else if(strcmp(argv[1], "sinc") == 0) {
			functional_test_basic<WIDTH, HEIGHT, TESTSIZE, sinc> tb("name", clk);
			sc_start();
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
