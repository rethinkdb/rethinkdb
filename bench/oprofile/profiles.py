# Copyright 2010-2012 RethinkDB, all rights reserved.
from oprofile import Event, Ratio, Profile

#Events
CPU_CLK_UNHALTED = Event('CPU_CLK_UNHALTED', 90000, 0x00, 0, 1)
INST_RETIRED = Event('INST_RETIRED', 90000, 0x1, 0, 1)
BR_INST_EXEC = Event('BR_INST_EXEC', 90000, 0x04, 0, 1)
L1I = Event('L1I', 90000, 0x04, 0, 1) #instruct load stall
ITLB_MISS_RETIRED = Event('ITLB_MISS_RETIRED', 90000, 0x20, 0, 1)
RESOURCE_STALLS = Event('RESOURCE_STALLS', 90000, 0x01, 0, 1)
L2_RQSTS = Event('L2_RQSTS', 90000, 0x30, 0, 1)
L1D_CACHE_LD = Event('L1D_CACHE_LD', 90000, 0x01, 0, 1)
STORE_BLOCKS = Event('STORE_BLOCKS', 90000, 0x0f, 0, 1)
L1D = Event('L1D', 90000, 0x01, 0, 1) #L1 cache miss
L2_LINES_IN = Event('L2_LINES_IN', 90000, 0x07, 0, 1)
DTLB_LOAD_MISSES = Event('DTLB_LOAD_MISSES', 90000, 0x01, 0, 1) #page walks
DTLB_MISSES = Event('DTLB_MISSES', 90000, 0x01, 0, 1)
L1D_CACHE_ST = Event('L1D_CACHE_ST', 90000, 0x1, 0, 1)


small_packet_profiles = [Profile([CPU_CLK_UNHALTED, INST_RETIRED, BR_INST_EXEC, L1I], 
                                 [Ratio(CPU_CLK_UNHALTED, INST_RETIRED),
                                  Ratio(L1I, CPU_CLK_UNHALTED),
                                  Ratio(BR_INST_EXEC, INST_RETIRED)]),
                         Profile([CPU_CLK_UNHALTED, L1D_CACHE_LD, STORE_BLOCKS, DTLB_LOAD_MISSES],
                                 [Ratio(L1D_CACHE_LD, CPU_CLK_UNHALTED),
                                  Ratio(STORE_BLOCKS, CPU_CLK_UNHALTED),
                                  Ratio(DTLB_LOAD_MISSES, CPU_CLK_UNHALTED)]),
                         Profile([INST_RETIRED, L1D, L2_LINES_IN, DTLB_MISSES],
                                 [Ratio(L1D, INST_RETIRED),
                                  Ratio(L2_LINES_IN, INST_RETIRED),
                                  Ratio(DTLB_MISSES, INST_RETIRED)]),
                         Profile([INST_RETIRED, L1D_CACHE_ST, L2_RQSTS, CPU_CLK_UNHALTED],
                                 [Ratio(L1D_CACHE_ST, INST_RETIRED),
                                  Ratio(L2_RQSTS, INST_RETIRED)]),
                         Profile([CPU_CLK_UNHALTED, RESOURCE_STALLS, INST_RETIRED, L1D],
                                 [Ratio(RESOURCE_STALLS, CPU_CLK_UNHALTED)]),]
