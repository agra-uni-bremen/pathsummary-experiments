#include <klee_conf.h>
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

template<unsigned Width, unsigned Height, unsigned TestSize>
struct functional_test_basic : public sc_core::sc_module {
	systolic<Width, Height> &dut;

	sc_core::sc_vector<sc_core::sc_signal<uint32_t>> ins_top;
	sc_core::sc_vector<sc_core::sc_signal<uint32_t>> ins_left;
	sc_core::sc_vector<sc_core::sc_signal<uint32_t>> outs_bottom;
	sc_core::sc_vector<sc_core::sc_signal<uint32_t>> outs_right;

	SC_HAS_PROCESS(functional_test_basic);
	functional_test_basic(sc_core::sc_module_name name, sc_clock& clock, systolic<Width, Height> &dut_in) : sc_module(name), dut(dut_in),
	ins_top("signals_top", Width), ins_left("signals_left", Height), outs_bottom("signals_bottom", Width),
	outs_right("signals_right", Height)
	{
		dut.clk(clock);
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
		uint32_t data_left[Height][TestSize];
		uint32_t data_top[TestSize][Width];

#ifndef USE_KLEE
		unsigned dc=0;
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<TestSize;++j) {
				data_left[i][j]=++dc;
			}
		}
		for(unsigned i=0;i<TestSize;++i) {
			for(unsigned j=0;j<Width;++j) {
				data_top[i][j]=++dc;
			}
		}
		// print
		std::cout << "data left" << std::endl;
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<TestSize;++j) {
				std::cout << (unsigned)data_left[i][j] << "  ";
			}
			std::cout << std::endl;
		}
		std::cout << "data top" << std::endl;
		for(unsigned i=0;i<TestSize;++i) {
			for(unsigned j=0;j<Width;++j) {
				std::cout << (unsigned)data_top[i][j] << "  ";
			}
			std::cout << std::endl;
		}
#else
//		klee_make_symbolic(data_left, sizeof(data_left), "indata left");
//		klee_make_symbolic(data_top, sizeof(data_top), "indata top");
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<TestSize;++j) {
				uint32_t temp = klee_int("dleft");
				//klee_assume(temp>0);
				data_left[i][j] = temp;
			}
		}
		for(unsigned i=0;i<TestSize;++i) {
			for(unsigned j=0;j<Width;++j) {
				uint32_t temp = klee_int("dtop");
				//klee_assume(temp>0);
				data_top[i][j] = temp;
			}
		}
#endif

		// padded
		uint32_t pad_left = Height-1;
		uint32_t total_left = TestSize+pad_left;
		uint32_t padded_left[Height][total_left];
		uint32_t pad_top = Width-1;
		uint32_t total_top = TestSize+pad_top;
		uint32_t padded_top[total_top][Width];

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
				std::cout << (unsigned)padded_left[i][j] << "  ";
			}
			std::cout << std::endl;
		}
		std::cout << "padded top" << std::endl;
		for(unsigned i=0;i<total_top;++i) {
			for(unsigned j=0;j<Width;++j) {
				std::cout << (unsigned)padded_top[i][j] << "  ";
			}
			std::cout << std::endl;
		}
#endif

		INFO(std::cout << "---------------------" << std::endl;)
		wait();
		int top_i=total_top-1;
		int left_i=total_left-1;
		while(top_i>=0||left_i>=0) {
			if(left_i>=0) {
				for (unsigned j = 0; j < Height; ++j) {
					ins_left[j] = padded_left[j][left_i];
				}
				--left_i;
			}
			if(top_i>=0) {
				for (unsigned j = 0; j < Width; ++j) {
					ins_top[j] = padded_top[top_i][j];
				}
				--top_i;
			}
			wait();
		}

		for (unsigned j=0; j<Height; ++j)
			ins_left[j] = 0;
		for (unsigned j=0; j<Width; ++j)
			ins_top[j] = 0;

		for(unsigned i=0;i<(Width>Height?Width-1:Height-1);++i) {
#ifdef FAULT_SYSTOLIC
			if(i==(Width>Height?Width-2:Height-2)) {
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

		INFO(std::cout << "check!" << std::endl;)
		for(unsigned i=0;i<Height;++i) {
			for(unsigned j=0;j<Width;++j) {
				uint32_t expected=0;
				for(unsigned x=0;x<TestSize;++x) {
					expected += data_left[i][x] * data_top[x][j];
				}
				INFO(std::cout << "exp: " << (unsigned)expected << ", recv: " << (unsigned)dut.dpus[i][j].sum << std::endl;)
				assert(dut.dpus[i][j].sum == expected);
			}
		}
		sc_stop();
	}
};

int sc_main(int argc, char* argv[])
{
	sc_report_handler::set_actions(SC_ID_INSTANCE_EXISTS_,
								   SC_DO_NOTHING);
	sc_clock clk{"clk", sc_core::sc_time(20, sc_core::SC_NS)};

	systolic<WIDTH, HEIGHT> d("systolic");
	functional_test_basic<WIDTH, HEIGHT, TESTSIZE> tb("basic", clk, d);
	sc_start();

	INFO(std::cout << "finished at " << sc_core::sc_time_stamp() << std::endl);
	return 0;
}
