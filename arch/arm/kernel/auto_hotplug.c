/* Copyright (c) 2012, Will Tisdale <willtisdale@gmail.com>. All rights reserved.
 *
 * Modified for Mako and Grouper, Francisco Franco <franciscofranco.1990@gmail.com>. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*
 * Generic auto hotplug driver for ARM SoCs. Targeted at current generation
 * SoCs with dual and quad core applications processors.
 * Automatically hotplugs online and offline CPUs based on system load.
 * It is also capable of immediately onlining a core based on an external
 * event by calling void hotplug_boostpulse(void)
 *
 * Not recommended for use with OMAP4460 due to the potential for lockups
 * whilst hotplugging - locks up because the SoC requires the hardware to	
 * be hotpluged in a certain special order otherwise it will probably	
 * deadlock or simply trigger the watchdog and reboot. 
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/sched.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

struct work_struct hotplug_online_all_work;
struct work_struct hotplug_offline_all_work;
<<<<<<< HEAD
struct work_struct hotplug_boost_online_work;

static unsigned int history[SAMPLING_PERIODS];
static unsigned int index;

static void hotplug_decision_work_fn(struct work_struct *work)
{
	unsigned int running, disable_load, enable_load, avg_running = 0;
	unsigned int online_cpus, available_cpus, i, j;
	int cpu;
#if DEBUG
	unsigned int k;
#endif

	online_cpus = num_online_cpus();
	available_cpus = 4;
	disable_load = disable_load_threshold * online_cpus;
	enable_load = enable_load_threshold * online_cpus;
<<<<<<< HEAD
<<<<<<< HEAD
	/*
	 * Multiply nr_running() by 100 so we don't have to
	 * use fp division to get the average.
	 */
	running = nr_running() * 100;

	history[index] = running;
=======
>>>>>>> fd10053... auto_hotplug.c: we're now scaling the cores based on the NVIDIA's avg_running code so now the driver will be less hard on the system because we got rid of the history cycle and multiplication before deciding when to online/offline cores.
=======
	running = nr_running() * 100;
>>>>>>> fb09b91... auto_hotplug.c: calling nr_running() for each online cpu is stupid because the value will be the same or almost the same, so to save few ms we can only call it once and fill the array(depending on num_online_cpus of course) with that same value.

	for_each_online_cpu(cpu) {
		history[index] = running;
		if (unlikely(index++ == INDEX_MAX_VALUE))
			index = 0;
	}

#if DEBUG
	pr_info("online_cpus is: %d\n", online_cpus);
	pr_info("enable_load is: %d\n", enable_load);
	pr_info("disable_load is: %d\n", disable_load);
	pr_info("index is: %d\n", index);
	pr_info("running is: %d\n", running);
#endif

	/*
	 * Use a circular buffer to calculate the average load
	 * over the sampling periods.
	 * This will absorb load spikes of short duration where
	 * we don't want additional cores to be onlined because
	 * the cpufreq driver should take care of those load spikes.
	 */
	for (i = 0, j = index; i < SAMPLING_PERIODS; i++, j--) {
		avg_running += history[j];
		if (unlikely(j == 0))
			j = INDEX_MAX_VALUE;
	}

	/*
	 * If we are at the end of the buffer, return to the beginning.
	 */
	if (unlikely(index++ == INDEX_MAX_VALUE))
		index = 0;

#if DEBUG
	pr_info("array contents: ");
	for (k = 0; k < SAMPLING_PERIODS; k++) {
		 pr_info("%d: %d\t",k, history[k]);
	}
	pr_info("\n");
	pr_info("avg_running before division: %d\n", avg_running);
#endif

	avg_running = avg_running / SAMPLING_PERIODS;

#if DEBUG
	pr_info("average_running is: %d\n", avg_running);
#endif

	if (!(flags & HOTPLUG_DISABLED)) {
		if (avg_running > enable_all_load_threshold && online_cpus < available_cpus) {
			//pr_info("auto_hotplug: Onlining all CPUs, avg running: %d\n", avg_running);
			/*
			 * Flush any delayed offlining work from the workqueue.
			 * No point in having expensive unnecessary hotplug transitions.
			 * We still online after flushing, because load is high enough to
			 * warrant it.
			 * We set the paused flag so the sampling can continue but no more
			 * hotplug events will occur.
			 */
			flags |= HOTPLUG_PAUSED;
			if (delayed_work_pending(&hotplug_offline_work))
				cancel_delayed_work(&hotplug_offline_work);
			schedule_work(&hotplug_online_all_work);
			return;
		} else if (flags & HOTPLUG_PAUSED) {
			schedule_delayed_work_on(0, &hotplug_decision_work, SAMPLING_RATE);
			return;
		} else if ((avg_running >= enable_load) && (online_cpus < available_cpus)) {
			if (delayed_work_pending(&hotplug_offline_work))
				cancel_delayed_work(&hotplug_offline_work);
			schedule_work(&hotplug_online_single_work);
			return;
<<<<<<< HEAD
<<<<<<< HEAD
		} else if (avg_running <= disable_load) {
=======
		} else if (avg_running < disable_load && online_cpus > 1) {//(disable_load/2) && online_cpus > 1) {
>>>>>>> f617c29... auto_hotplug.c: after boostpulse is done instead of waiting 5 seconds to offline the core, now it offlines after 2 seconds. Average 15 sample times instead of 12. A little change on when to disable a core when the boostpulse flag is not on because since we now average more times now when the avg_running is less than the disable_load and online_cpus > 1 we offline a core.
			/* Only queue a cpu_down() if there isn't one already pending */
			if (!(delayed_work_pending(&hotplug_offline_work))) {
				if (online_cpus == 2 && avg_running < (disable_load/2)) {
					pr_info("auto_hotplug: Online CPUs = 2; Offlining CPU, avg running: %d\n", avg_running);
					flags |= HOTPLUG_PAUSED;
					schedule_delayed_work_on(0, &hotplug_offline_work, MIN_SAMPLING_RATE);
				} else if (online_cpus > 2) {
					pr_info("auto_hotplug: Offlining CPU, avg running: %d\n", avg_running);
					schedule_delayed_work_on(0, &hotplug_offline_work, HZ);
				}
				/* If boostpulse is active, clear the flags */
				else if(flags & BOOSTPULSE_ACTIVE) {
					flags &= ~BOOSTPULSE_ACTIVE;
					pr_info("auto_hotplug: Clearing boostpulse flags\n");
				}
=======
		} else if (avg_running < disable_load && online_cpus > 1) {
			/* Only queue a cpu_down() if there isn't one already pending */
			if(flags & BOOSTPULSE_ACTIVE) {
				flags &= ~BOOSTPULSE_ACTIVE;
			} else if (!(delayed_work_pending(&hotplug_offline_work)) && !(flags & BOOSTPULSE_ACTIVE)) {
				schedule_delayed_work_on(0, &hotplug_offline_work, HZ);
>>>>>>> 1761839... auto_hotplug.c: remove dynamic sampling time based on online cores and schedule the works in a 100ms sampling rate. Also cleanup how check for boostpulse flag to reduce reduntant function calls. Also reduced enable_all_threshold from 500 to 425 otherwise it would be really hard to have them all enabled at a certain point of time. Maybe it easier to stay on all cores if the load is above the threshold - there was a small bug here that almost prevented all the cores to stay up until the load gets down.
			}
		}
	}
	
#if DEBUG
	pr_info("sampling_rate is: %d\n", jiffies_to_msecs(SAMPLING_RATE));
#endif
	schedule_delayed_work_on(0, &hotplug_decision_work, SAMPLING_RATE);
}
=======
>>>>>>> 58f28a6... auto_hotplug.c: remove hotplugging on screen on for testing purposes.

static void hotplug_online_all_work_fn(struct work_struct *work)
{
	int cpu;
	for_each_possible_cpu(cpu) {
		if (!cpu_online(cpu)) {
			cpu_up(cpu);
			pr_info("auto_hotplug: CPU%d up.\n", cpu);
		}
	}
}

static void hotplug_offline_all_work_fn(struct work_struct *work)
{
	int cpu;
	for_each_possible_cpu(cpu) {
		if (likely(cpu_online(cpu) && (cpu))) {
			cpu_down(cpu);
			pr_info("auto_hotplug: CPU%d down.\n", cpu);
		}
	}
}

<<<<<<< HEAD
static void hotplug_online_single_work_fn(struct work_struct *work)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		if (cpu) {
			if (!cpu_online(cpu)) {
				cpu_up(cpu);
				pr_info("auto_hotplug: CPU%d up.\n", cpu);
				break;
			}
		}
	}
	schedule_delayed_work_on(0, &hotplug_decision_work, (HZ/2));
}

static void hotplug_offline_single_work_fn(struct work_struct *work)
{
	int cpu;
	for_each_online_cpu(cpu) {
		if (cpu) {
			cpu_down(cpu);
			pr_info("auto_hotplug: CPU%d down.\n", cpu);
			break;
		}
	}
	schedule_delayed_work_on(0, &hotplug_decision_work, HZ);
}

static void hotplug_unpause_work_fn(struct work_struct *work)
{
	flags &= ~HOTPLUG_PAUSED;
}

inline void hotplug_boostpulse(void)
{
	if (unlikely(flags & (EARLYSUSPEND_ACTIVE
		| HOTPLUG_DISABLED)))
		return;

	if (!(flags & BOOSTPULSE_ACTIVE)) {
		flags |= BOOSTPULSE_ACTIVE;
		/*
		 * If there are less than 2 CPUs online, then online
		 * an additional CPU, otherwise check for any pending
		 * offlines, cancel them and pause for 2 seconds.
		 * Either way, we don't allow any cpu_down()
		 * whilst the user is interacting with the device.
		 */
		if (likely(num_online_cpus() < 2)) {
<<<<<<< HEAD
<<<<<<< HEAD
=======
			//pr_info("User is interacting with the device, make sure 2 CPU's are active.\n");
>>>>>>> 1761839... auto_hotplug.c: remove dynamic sampling time based on online cores and schedule the works in a 100ms sampling rate. Also cleanup how check for boostpulse flag to reduce reduntant function calls. Also reduced enable_all_threshold from 500 to 425 otherwise it would be really hard to have them all enabled at a certain point of time. Maybe it easier to stay on all cores if the load is above the threshold - there was a small bug here that almost prevented all the cores to stay up until the load gets down.
=======
>>>>>>> f504990... auto_hotplug.c: since the sampling rate was adjusted a few patches ago I never got to update the timers in the delayed works to be according the new rate and the more sampling periods. Should be more snappy now when the device needs the extra processing power, without sacrificing battery. Also added contact info in the top of the file and more info regarding OMAP4460.
			cancel_delayed_work_sync(&hotplug_offline_work);
			flags |= HOTPLUG_PAUSED;
			schedule_work(&hotplug_online_single_work);
			schedule_delayed_work(&hotplug_unpause_work, HZ * 5);
		} else {
			if (delayed_work_pending(&hotplug_offline_work)) {
				cancel_delayed_work(&hotplug_offline_work);
				flags |= HOTPLUG_PAUSED;
				schedule_delayed_work(&hotplug_unpause_work, HZ);
				schedule_delayed_work_on(0, &hotplug_decision_work, HZ);
			}
		}
	}
}

=======
>>>>>>> 58f28a6... auto_hotplug.c: remove hotplugging on screen on for testing purposes.
#ifdef CONFIG_HAS_EARLYSUSPEND
static void auto_hotplug_early_suspend(struct early_suspend *handler)
{
    if (num_online_cpus() > 1) {
    	pr_info("auto_hotplug: Offlining CPUs for early suspend\n");
        schedule_work_on(0, &hotplug_offline_all_work);
	}
}

static void auto_hotplug_late_resume(struct early_suspend *handler)
{
	schedule_work_on(0, &hotplug_online_all_work);
}

static struct early_suspend auto_hotplug_suspend = {
	.suspend = auto_hotplug_early_suspend,
	.resume = auto_hotplug_late_resume,
};
#endif /* CONFIG_HAS_EARLYSUSPEND */

int __init auto_hotplug_init(void)
{
	pr_info("auto_hotplug: v0.220 by _thalamus\n");
	pr_info("auto_hotplug: %d CPUs detected\n", 4);

	INIT_WORK(&hotplug_online_all_work, hotplug_online_all_work_fn);
	INIT_WORK(&hotplug_offline_all_work, hotplug_offline_all_work_fn);
<<<<<<< HEAD
	INIT_DELAYED_WORK_DEFERRABLE(&hotplug_offline_work, hotplug_offline_single_work_fn);

	/*
	 * Give the system time to boot before fiddling with hotplugging.
	 */
	flags |= HOTPLUG_PAUSED;
	schedule_delayed_work_on(0, &hotplug_decision_work, HZ * 5);
	schedule_delayed_work(&hotplug_unpause_work, HZ * 6);
=======
>>>>>>> 58f28a6... auto_hotplug.c: remove hotplugging on screen on for testing purposes.

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&auto_hotplug_suspend);
#endif
	return 0;
}
late_initcall(auto_hotplug_init);