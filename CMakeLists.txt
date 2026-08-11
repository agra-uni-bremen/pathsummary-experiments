cmake_minimum_required(VERSION 3.10)

# currently, five different faults can be activated (in build: xyz_FAULT1 if fault 1 is active, xyz_FAULT2 if fault 2, ...)
# only one is active at a time, this macro just defines the number of faults that should be built
# (e.g. 2: FAULT0, FAULT1, FAULT2, but each only has the corresponding fault)
set(AMOUNT_APPROXFAULTS 0) # 4
# how large is the interval? results in [-limit,limit) => e.g. [-2048,2048)
set(APPROX_INTERVALS 2048) # 1024 4096 8192 16384 32768)
# how many segments?
# !! any number <= is possible w/o problems. For >256, only 512, 1024, and 2048 are supported (boost library issue)
set(APPROX_SEGMENTS 64) # 32 128 256 512 1024 2048)
# how often do we transfer new input into the DUV?
set(APPROX_ITERATIONS 1) # 4 8 16)

# floating point: for functions with small values.
# attributes see above
set(APPROX_FP_INTERVALS 1) # 2 4 8)
set(APPROX_FP_SEGMENTS 16) #2 4 8 32 64 128 256 512 1024)
set(APPROX_FP_ITERATIONS 1 2) # 4 8 16)

# systolic array dimensions: width/height of systolic array, amount of inputs
set(SYSTOLIC_WIDTHS 2) # 4 6)
set(SYSTOLIC_HEIGHTS 2) # 4 6)
set(SYSTOLIC_TESTSIZES 2) # 4 6)

set(AMOUNT_SYSTOLICFAULTS 0) #4

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
# "-fsanitize=memory -fsanitize-memory-track-origins=2 -fno-omit-frame-pointer"
set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fno-omit-frame-pointer -g -O1")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(PathSummary_Experiments NONE)	#NONE Important for wllvm and custom Bytecode

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include(config/DetermineLanguage.cmake)
find_package( SystemC REQUIRED)

add_subdirectory(config)
add_subdirectory(duts)

foreach(INTERVAL IN LISTS APPROX_INTERVALS)
	foreach(SEGMENT IN LISTS APPROX_SEGMENTS)
		foreach (ITERATION IN LISTS APPROX_ITERATIONS)
			# Approx + Systolic SysC
			foreach(HEIGHT IN LISTS SYSTOLIC_HEIGHTS)
				foreach(WIDTH IN LISTS SYSTOLIC_WIDTHS)
					foreach(TESTSIZE IN LISTS SYSTOLIC_TESTSIZES)
						add_executable(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}
								benches/bench_systapprox_sysc.cpp
						)
						target_include_directories(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
						target_link_libraries(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE DPU_APPROX=1)
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE HEIGHT=${HEIGHT})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE WIDTH=${WIDTH})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE TESTSIZE=${TESTSIZE})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_L=-${INTERVAL})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_U=${INTERVAL})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_ACC=${SEGMENT})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_IT=${ITERATION})
						if(AMOUNT_APPROXFAULTS GREATER 0)
							foreach(FAULT RANGE 1 ${AMOUNT_APPROXFAULTS})
								add_executable(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT}
										benches/bench_systapprox_sysc.cpp
								)
								target_include_directories(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
								target_link_libraries(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE DPU_APPROX=1)
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE HEIGHT=${HEIGHT})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE WIDTH=${WIDTH})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE TESTSIZE=${TESTSIZE})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_L=-${INTERVAL})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_U=${INTERVAL})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_ACC=${SEGMENT})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_IT=${ITERATION})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE FAULT_APPROX=${FAULT})
							endforeach()
						endif ()
						if(AMOUNT_SYSTOLICFAULTS GREATER 0)
							foreach(FAULT RANGE 1 ${AMOUNT_SYSTOLICFAULTS})
								add_executable(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION}
										benches/bench_systapprox_sysc.cpp
								)
								target_include_directories(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
								target_link_libraries(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE DPU_APPROX=1)
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE HEIGHT=${HEIGHT})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE WIDTH=${WIDTH})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE TESTSIZE=${TESTSIZE})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_L=-${INTERVAL})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_U=${INTERVAL})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_ACC=${SEGMENT})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_IT=${ITERATION})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE FAULT_SYSTOLIC=${FAULT})
							endforeach()
						endif ()
					endforeach ()
				endforeach ()
			endforeach ()
			# Approx SysC
			add_executable(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}
					benches/bench_approx_sysc.cpp
			)
			target_include_directories(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
			target_link_libraries(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
			target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_L=-${INTERVAL})
			target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_U=${INTERVAL})
			target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_ACC=${SEGMENT})
			target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_IT=${ITERATION})
			if(AMOUNT_APPROXFAULTS GREATER 0)
				foreach(FAULT RANGE 1 ${AMOUNT_APPROXFAULTS})
					add_executable(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT}
							benches/bench_approx_sysc.cpp
					)
					target_include_directories(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
					target_link_libraries(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
					target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_L=-${INTERVAL})
					target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_U=${INTERVAL})
					target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_ACC=${SEGMENT})
					target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_IT=${ITERATION})
					target_compile_definitions(testbench_sysc_approx_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE FAULT_APPROX=${FAULT})
				endforeach()
			endif ()
		endforeach ()
	endforeach ()
endforeach ()

foreach(INTERVAL IN LISTS APPROX_FP_INTERVALS)
	foreach(SEGMENT IN LISTS APPROX_FP_SEGMENTS)
		foreach (ITERATION IN LISTS APPROX_FP_ITERATIONS)
			# Approx + Systolic SysC
			foreach(HEIGHT IN LISTS SYSTOLIC_HEIGHTS)
				foreach(WIDTH IN LISTS SYSTOLIC_WIDTHS)
					foreach(TESTSIZE IN LISTS SYSTOLIC_TESTSIZES)
						add_executable(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}
								benches/bench_systapprox_sysc.cpp
						)
						target_include_directories(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
						target_link_libraries(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE DPU_APPROX=1)
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE HEIGHT=${HEIGHT})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE WIDTH=${WIDTH})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE TESTSIZE=${TESTSIZE})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_L=-${INTERVAL})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_U=${INTERVAL})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_ACC=${SEGMENT})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_IT=${ITERATION})
						target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE NEED_FP=1)
						if(AMOUNT_APPROXFAULTS GREATER 0)
							foreach(FAULT RANGE 1 ${AMOUNT_APPROXFAULTS})
								add_executable(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT}
										benches/bench_systapprox_sysc.cpp
								)
								target_include_directories(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
								target_link_libraries(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE DPU_APPROX=1)
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE HEIGHT=${HEIGHT})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE WIDTH=${WIDTH})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE TESTSIZE=${TESTSIZE})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_L=-${INTERVAL})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_U=${INTERVAL})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_ACC=${SEGMENT})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_IT=${ITERATION})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE FAULT_APPROX=${FAULT})
								target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE NEED_FP=1)
							endforeach()
						endif ()
						if(AMOUNT_SYSTOLICFAULTS GREATER 0)
						foreach(FAULT RANGE 1 ${AMOUNT_SYSTOLICFAULTS})
							add_executable(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}
									benches/bench_systapprox_sysc.cpp
							)
							target_include_directories(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
							target_link_libraries(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE DPU_APPROX=1)
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE HEIGHT=${HEIGHT})
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE WIDTH=${WIDTH})
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE TESTSIZE=${TESTSIZE})
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_L=-${INTERVAL})
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_U=${INTERVAL})
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_ACC=${SEGMENT})
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_IT=${ITERATION})
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE FAULT_SYSTOLIC=${FAULT})
							target_compile_definitions(testbench_sysc_syst_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE NEED_FP=1)
						endforeach()
					endif ()
					endforeach ()
				endforeach ()
			endforeach ()

			# Approx SysC
			add_executable(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}
					benches/bench_approx_sysc.cpp
			)
			target_include_directories(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
			target_link_libraries(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
			target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_L=-${INTERVAL})
			target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_U=${INTERVAL})
			target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_ACC=${SEGMENT})
			target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE BUILD_IT=${ITERATION})
			target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION} PRIVATE NEED_FP=1)
			if(AMOUNT_APPROXFAULTS GREATER 0)
				foreach(FAULT RANGE 1 ${AMOUNT_APPROXFAULTS})
					add_executable(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT}
							benches/bench_approx_sysc.cpp
					)
					target_include_directories(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
					target_link_libraries(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
					target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_L=-${INTERVAL})
					target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_U=${INTERVAL})
					target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_ACC=${SEGMENT})
					target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE BUILD_IT=${ITERATION})
					target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE FAULT_APPROX=${FAULT})
					target_compile_definitions(testbench_sysc_approx_fp_${INTERVAL}_${SEGMENT}_${ITERATION}_FAULT${FAULT} PRIVATE NEED_FP=1)
				endforeach()
			endif ()
		endforeach ()
	endforeach ()
endforeach ()

foreach(HEIGHT IN LISTS SYSTOLIC_HEIGHTS)
	foreach(WIDTH IN LISTS SYSTOLIC_WIDTHS)
		foreach(TESTSIZE IN LISTS SYSTOLIC_TESTSIZES)
			add_executable(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE}
					benches/bench_systolic_sysc.cpp
			)
			target_include_directories(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
			target_compile_definitions(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE} PRIVATE HEIGHT=${HEIGHT})
			target_compile_definitions(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE} PRIVATE WIDTH=${WIDTH})
			target_compile_definitions(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE} PRIVATE TESTSIZE=${TESTSIZE})
			target_link_libraries(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
			if(AMOUNT_SYSTOLICFAULTS GREATER 0)
				foreach(FAULT RANGE 1 ${AMOUNT_SYSTOLICFAULTS})
					add_executable(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT}
							benches/bench_systolic_sysc.cpp
					)
					target_include_directories(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT} PRIVATE ${SystemC_INCLUDE_DIR} config/klee)
					target_compile_definitions(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT} PRIVATE HEIGHT=${HEIGHT})
					target_compile_definitions(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT} PRIVATE WIDTH=${WIDTH})
					target_compile_definitions(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT} PRIVATE TESTSIZE=${TESTSIZE})
					target_link_libraries(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT} peripherals_sysc) # ${SystemC_LIBRARIES} peripherals_sysc)
					target_compile_definitions(testbench_sysc_systolic_${WIDTH}_${HEIGHT}_${TESTSIZE}_FAULT${FAULT} PRIVATE FAULT_SYSTOLIC=${FAULT})
				endforeach()
			endif ()
		endforeach ()
	endforeach ()
endforeach ()
