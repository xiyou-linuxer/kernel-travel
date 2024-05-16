#include <trap/softirq.h>
#include <trap/irq.h>
#include <linux/stdio.h>

uint32_t softirq_active;
static struct softirq_action softirq_vec[32];

static void bh_enable(void) {
	bh_count--;
}
static void bh_disable(void) {
	bh_count++;
}

void do_softirq(void)
{
	/* escape for 2 cases:
	 * first->  Nested hardirq(impossible)
	 * second-> in softirq */
	if (in_interrupt()) {
		return ;
	}

	bh_disable();
	//printk("do softirq...\n");

	/* must be done before enable interrupt
	 * just thinking about some hardirq intervene
	 * and set some new bit for softirq_active
	 * so active is not equal to the eliminated bits */
	uint32_t active = softirq_active;
	softirq_active &= ~softirq_active;
	intr_enable();

	/* do softirqs */
	struct softirq_action *h = softirq_vec;
	while (active != 0)
	{
		if (active & 1)
			h->action(h->data);

		active >>= 1;
		h++;
	}
	intr_disable();

	bh_enable();
}

void open_softirq(uint32_t nr,void (*action)(void*),void* data)
{
	softirq_vec[nr].action = action;
	softirq_vec[nr].data   = data;
}

void raise_softirq(uint32_t nr)
{
	softirq_active |= (1 << nr);
}

void __init softirq_init(void)
{
	/* wait for new features */
}


