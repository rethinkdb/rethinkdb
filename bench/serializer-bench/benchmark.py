# Copyright 2010-2012 RethinkDB, all rights reserved.
#!/usr/bin/env python
from itertools import product
from time import sleep
from threading import Lock
import threading, subprocess
import sys

# Define some constants
KILOBYTE = 1024
MEGABYTE = KILOBYTE * 1024
MINUTE = 60

# Define run arguments
duration = 45 * MINUTE
tps_file_prefix = "tps_"
drives = ["sdb", "sdc", "sdd", "sde"]

# Define possible serializer parameters
block_sizes = [4 * KILOBYTE, 8 * KILOBYTE]
extent_size = [256 * KILOBYTE, 512 * KILOBYTE, 1 * MEGABYTE, 2 * MEGABYTE, 4 * MEGABYTE, 8 * MEGABYTE]
active_extents = [1, 4, 8]

# Some more arguments
bench_exec_path = "./serializer-bench"

# Runs a process and calls back a function when it's done
def run_process_and_callback_on_done(cmds, callback_fn):
	print "Starting run %s" % str(cmds[len(cmds) - 1])
	def run_in_thread(cmds, callback_fn):
		for cmd in cmds:
			proc = subprocess.Popen(cmd)
			proc.wait()
		callback_fn(cmd)
		return
	thread = threading.Thread(target=run_in_thread, args=(cmds, callback_fn))
	thread.start()

# Returns an available drive path, or blocks if all are busy
def get_available_drive():
	while True:
		try:
			with runs_lock:
				drive = available_drives.pop()
			return drive
		except KeyError:
			try:
				sleep(1)
			except KeyboardInterrupt:
				sys.exit(-1)

# Sets the linux io scheduler
def set_io_scheduler(drive_name, scheduler):
	f = open('/sys/block/' + drive_name + '/queue/scheduler', 'w')
	f.write(scheduler + '\n')
	f.close()

# Runs a specific profile on a specific drive
def run_benchmark(profile, drive_name):
	drive = "/dev/" + drive_name
	# Grab the parameters for the run
	block_size = profile[0]
	extent_size = profile[1]
	active_extents = profile[2]
	# Define the function to be called on run completion
	def on_run_complete(cmd):
		global runs_completed
		with runs_lock:
			available_drives.add(drive_name)
			runs_completed += 1
		print "Finished run %s" % cmd
	# Set io scheduler to noop
	set_io_scheduler(drive_name, "noop")
        # Print run name as first run in the tps log file
        run_header = str(block_size) + "_" + str(extent_size) + "_" + str(active_extents)
        tps_file_path = tps_file_prefix + run_header
        f = open(tps_file_path, 'w')
        f.write('#' + run_header + '\n')
        f.close()
	# Do the run
	run_process_and_callback_on_done([
					  #["hdparm", "--user-master", "u", "--security-set-pass", "test", drive],
					  #["hdparm", "--user-master", "u", "--security-erase", "test", drive],
					  ["sg_format", "--wait", "--format", drive],
					  [bench_exec_path,
					  "--block-size", str(block_size),
					  "--extent-size", str(extent_size),
					  "--active-data-extents", str(active_extents),
					  "--duration", str(duration),
					  "-f", drive,
                                          "--tps-log", tps_file_path]
					 ],
					 on_run_complete)

# Driver function
def run_benchmarks():
	global runs_started
	# Create a lazy list of all possible workload combinations
	profiles = product(block_sizes, extent_size, active_extents)
	# For each profile, grab a drive and run it
	for profile in profiles:
		drive = get_available_drive()
		runs_started += 1
		run_benchmark(profile, drive)

# Let's rock and roll
if __name__ == "__main__":
	# Make all drives available initially
	global available_drives
	available_drives = set(drives)
	# We still have more runs to do...
	global runs_started, runs_completed, runs_lock
	runs_lock = Lock()
	runs_started = 0
	runs_completed = 0
	# Run the benchmarks
	run_benchmarks()
	# Now we wait
	while True:
		with runs_lock:
			if runs_started == runs_completed:
				break
		try:
			sleep(1)
		except KeyboardInterrupt:
			sys.exit(-1)
	
