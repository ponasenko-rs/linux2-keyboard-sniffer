#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/types.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roman Ponasenko");
MODULE_VERSION("0.1");

const static int KB_IRQ = 1;
const static int timer_period_ms = 60000; // minute

struct timer_list timer;
static struct tasklet_struct presses_counter;
static atomic_t presses_count = ATOMIC_INIT(0);

static irqreturn_t ks_top_handler(int irq, void* device) {
	tasklet_schedule(&presses_counter);
	return IRQ_HANDLED;
}

static void ks_bottom_handler(unsigned long data) {
	atomic_inc(&presses_count);
}

static void timer_handler(struct timer_list* timerp) {
	printk(KERN_INFO "ks: you pressed keyboard buttons %d times for the last minute\n",
		atomic_read(&presses_count));

	atomic_set(&presses_count, 0);
	mod_timer(&timer, jiffies + msecs_to_jiffies(timer_period_ms));
}

static int __init ks_init(void) {
	printk(KERN_INFO "ks: init\n");

	if (request_irq(KB_IRQ, ks_top_handler, IRQF_SHARED, "ks_module", &presses_counter) < 0) {
		printk(KERN_INFO "ks: cant register top handler\n");
		return -1;
	}

	tasklet_init(&presses_counter, ks_bottom_handler, 0);

	timer_setup(&timer, timer_handler, 0);
	mod_timer(&timer, jiffies + msecs_to_jiffies(timer_period_ms));
	
	return 0;
}

static void __exit ks_exit(void) {
	printk(KERN_INFO "ks: exit\n");

	del_timer(&timer);

	tasklet_kill(&presses_counter);
  	
	disable_irq(KB_IRQ);
	free_irq(KB_IRQ, &presses_counter);
	enable_irq(KB_IRQ);
}

module_init(ks_init);
module_exit(ks_exit);

