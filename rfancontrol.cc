#include <stdio.h>
#include <time.h>
#include <nvml.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <algorithm>

static void write_int(const char* const path, const int val)
{
	FILE* f = fopen(path, "w+");
	if (!f)
		return;
	char str[16];
	snprintf(str, 16, "%d", val);
	fwrite(str, strlen(str), 1, f);
	fclose(f);
}

static int read_int(const char* const path)
{
	FILE* f = fopen(path, "r");
	int val;
	const int r = fscanf(f, "%d", &val);
	fclose(f);
	return r > 0 ? val : 0;
}

static int running = 1;

static void signal_handler(int sig)
{
	switch (sig) {
	case SIGTERM:
	case SIGINT:
	case SIGABRT:
		running = 0;
		break;
	case SIGHUP:
		// Don't know how to handle yet.
		break;
	}
}

int main(const int argc, const char* argv[])
{
	struct sigaction act;
	act.sa_handler = &signal_handler;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, 0);
	sigaction(SIGINT, &act, 0);
	sigaction(SIGABRT, &act, 0);
	sigaction(SIGHUP, &act, 0);
	const pid_t pid = getpid();
	write_int("/var/run/rfancontrol.pid", (int)pid);
	nvmlInit();
	struct timespec req, rem;
	printf("Override to Manual, pid %d\n", (int)pid);
	// See https://github.com/torvalds/linux/blob/master/drivers/hwmon/it87.c
	// The new auto_pwm is supported on sTRX40 Aorus Xtreme. Use that instead.
	// First, read previous mode for GPU fans, and auto setup.
	const int pwm1_enable = read_int("/sys/class/hwmon/hwmon1/pwm1_enable");
	write_int("/sys/class/hwmon/hwmon1/pwm1_enable", 1);
	do {
		// Sleep 0.5 second.
		req.tv_sec = 0;
		req.tv_nsec = 500000000;
		nanosleep(&req, &rem);
		// Read GPU fan speed.
		unsigned int device_count = 0;
		if (NVML_SUCCESS != nvmlDeviceGetCount(&device_count))
			continue;
		unsigned int max_temperature = 30;
		for (unsigned int i = 0; i < device_count; i++)
		{
			nvmlDevice_t device;
			if (NVML_SUCCESS != nvmlDeviceGetHandleByIndex(i, &device))
				continue;
			unsigned int temperature;
			if (NVML_SUCCESS != nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temperature))
				continue;
			if (temperature > max_temperature)
				max_temperature = temperature;
		}
		const int pwm1 = std::min(255, std::max(0, (int)(max_temperature * max_temperature / 3600.0 * 256))); // Quadratic curve such that 25% maps to 30C, 100% maps to 60C.
		write_int("/sys/class/hwmon/hwmon1/pwm1", pwm1);
	} while (running);
	printf("Exiting, Reset to Automatic\n");
	write_int("/sys/class/hwmon/hwmon1/pwm1_enable", pwm1_enable);
	nvmlShutdown();
	return 0;
}
