#include <linux/init.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/inetdevice.h>
#include <linux/timer.h>
#include "smews.h"
#include "input.h"
#include "output.h"
#include "connections.h"

int ksmews_recv_eth(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev);

static struct packet_type ksmews_ip_packet_type = {
	.type = __constant_htons(ETH_P_IP),
	.func = ksmews_recv_eth,
};
static struct list_head initial_pt_handlers;
static struct list_head *pt_handlers_head = NULL;
static struct net_device *smews_dev = NULL;
static char *curr_in_data = NULL;
static char *curr_out_data = NULL;
static struct sk_buff *out_skb = NULL;
static unsigned char hw_src[6];
static unsigned char hw_dst[6];
static char hw_dst_set = 0;

void ksmews_callback(unsigned long arg);

//~ static struct timer_list ksmews_timer;
DECLARE_TASKLET(ksmews_tasklet,ksmews_callback,0);

//~ #define KSMEWS_DBG

#define ksmews_print(...) printk(KERN_ALERT "[ksmews] " __VA_ARGS__ )
#ifdef KSMEWS_DBG
	#define ksmews_dbg(...) printk(KERN_ALERT "[ksmews] " __VA_ARGS__ )
#else
	#define ksmews_dbg(...) 
#endif


/*-----------------------------------------------------------------------------------*/
void smews_put(unsigned char c) {
	if(out_skb)
		*curr_out_data++ = c;
	return;
}

/*-----------------------------------------------------------------------------------*/
void smews_putn_const(unsigned char *ptr,uint16_t n) {
	memcpy(curr_out_data,ptr,n);
	curr_out_data+=n;
}

/*-----------------------------------------------------------------------------------*/
void smews_prepare_output(uint16_t length) {
	ksmews_dbg(">prepare output %d\n",length);
	/* create skbuf with the requested size */
	out_skb = alloc_skb(length + LL_ALLOCATED_SPACE(smews_dev), GFP_ATOMIC);
	if (out_skb == NULL)
		return;
	/* set the skbuf */
	out_skb->dev = smews_dev;
	out_skb->protocol = ETH_P_IP;
	out_skb->network_header = out_skb->data+20;
	/* reserve space for both ethernet header and data (size: length) */
	skb_reserve(out_skb, LL_RESERVED_SPACE(smews_dev));
	curr_out_data = skb_put(out_skb,length);
	/* fill ethernet header */
	if (dev_hard_header(out_skb, smews_dev, ETH_P_IP, hw_dst, hw_src, out_skb->len) < 0) {
		out_skb = NULL;
		return;
	}
	ksmews_dbg("<prepare output\n");
}

/*-----------------------------------------------------------------------------------*/
void smews_output_done(void) {
	ksmews_dbg(">send\n");
	/* transmit the skbuf */
	if(out_skb) {
		NF_HOOK(NFPROTO_UNSPEC, 0, out_skb, NULL, out_skb->dev, dev_queue_xmit);
	}
	ksmews_dbg("<send\n");
}

/*-----------------------------------------------------------------------------------*/
int16_t smews_get(void) {
	if(curr_in_data)
		return 0x00ff & (*curr_in_data++);
	else
		return -1;
}

/*-----------------------------------------------------------------------------------*/
unsigned char smews_data_to_read(void) {
	return curr_in_data != NULL;
}

/*-----------------------------------------------------------------------------------*/
static inline int deliver_skb(struct sk_buff *skb, struct packet_type *pt_prev, struct net_device *orig_dev) {
	atomic_inc(&skb->users);
	return pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
}

/*-----------------------------------------------------------------------------------*/
void ksmews_callback(unsigned long arg) {
	ksmews_dbg(">tasklet\n");
	if(smews_main_loop_step(1)) {
		tasklet_hi_schedule(&ksmews_tasklet);
	}
	ksmews_dbg("<tasklet\n");
}

/*-----------------------------------------------------------------------------------*/
int ksmews_recv_eth(struct sk_buff *skb, struct net_device *dev, struct packet_type *pt, struct net_device *orig_dev) {
	ksmews_dbg(">recv IP/Eth\n");
	/* target the beginning of the IP data */
	/* first check if this packet is for smews */
	if(ntohl(*(unsigned int*)(skb->network_header + 16)) != UI32(local_ip_addr) || // check IP dst address
		ntohs(*(unsigned short*)(skb->network_header + 2)) < 40 || // check IP packet size
		*(unsigned char*)(skb->network_header + 9) != 6 || // check IP proto == TCP
		ntohs(*(unsigned short*)(skb->network_header + 22)) != 80 // check TCP dst port == 80
	) { /* forward the packet to initial kernel handlers (this code is mainly copied from linux net/core/dev.c) */
		ksmews_dbg("*forward\n");
		struct packet_type *ptype, *pt_prev = NULL;
		struct net_device *null_or_orig = NULL;
		if(orig_dev->master) {
			if(skb_bond_should_drop(skb))
				null_or_orig = orig_dev; /* deliver only exact match */
		}

		list_for_each_entry(ptype, &initial_pt_handlers, list) {
		if(ptype->dev == null_or_orig ||
			ptype->dev == skb->dev ||
			ptype->dev == orig_dev) {
				if(pt_prev)
					deliver_skb(skb, pt_prev, orig_dev);
			pt_prev = ptype;
			}
		}
		if(pt_prev) {
			pt_prev->func(skb, skb->dev, pt_prev, orig_dev);
		} else {
			kfree_skb(skb);
		}
	} else { /* process the packet in smews */
		ksmews_dbg("*process\n");
		if(!hw_dst_set) { /* store the source address as a future dst address */
			memcpy(hw_dst,skb->mac_header+6,6);
			hw_dst_set = 1;
		}
		/* run smews */
		curr_in_data = skb->network_header;
		//~ if(smews_receive()) {
			//~ tasklet_hi_schedule(&ksmews_tasklet);
		//~ }
		//~ curr_in_data = NULL;
		if(smews_receive()) {
			curr_in_data = NULL;
			while(smews_main_loop_step(1));
			//~ tasklet_hi_schedule(&ksmews_tasklet);
		}
		
		/* free the skbuf and give back the hand */
		kfree_skb(skb);
	}
	ksmews_dbg("<recv IP/Eth\n");
	return 0;
}

/*-----------------------------------------------------------------------------------*/
void ksmews_override_eth_handler(struct packet_type *pt) {
	struct packet_type *pt1;
	/* insert the new handler */
	dev_add_pack(pt);
	/* access the handlers list */
	pt_handlers_head = pt->list.prev;
	/* create a list for the initial handlers */
	INIT_LIST_HEAD(&initial_pt_handlers);
	/* fill the list with inirial handlers */
	list_for_each_entry(pt1, pt_handlers_head, list) {
		if (pt1 && pt != pt1 && pt->type == pt1->type) {
			struct packet_type *pt2 = list_entry(pt1->list.next, typeof(*pt1), list);
			list_del(&pt1->list);
			list_add(&pt1->list,&initial_pt_handlers);
			pt1 = pt2;
		}
	}
}

/*-----------------------------------------------------------------------------------*/
void ksmews_restore_eth_handler(struct packet_type *pt) {
	/* remove the current handler */
	dev_remove_pack(pt);
	/* restore the inirial handlers */
	list_splice(&initial_pt_handlers,pt_handlers_head);
}

/*-----------------------------------------------------------------------------------*/
static int ksmews_init(void) {
	ksmews_print("*initializing\n");
	/* init smews */
	smews_init();
	/* remove the kernel handlers for all IP/ethernet packets, replace with the smews handler */
	ksmews_override_eth_handler(&ksmews_ip_packet_type);	
	/* search the ethernet device structure */
	smews_dev = __dev_get_by_name(&init_net,"eth0");
	if(smews_dev == NULL) {
		smews_dev = __dev_get_by_name(&init_net,"eth1");
		if(smews_dev == NULL) {
			return 0;
		}
	}
	/* get the mac and IP addresses from the device */
	memcpy(hw_src,smews_dev->perm_addr,6);
	UI32(local_ip_addr) = ntohl(inet_select_addr(smews_dev, 0, 0));
	/* print status */
	ksmews_print("*network device: %s\n",smews_dev->name);
	ksmews_print("*mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",hw_src[0],hw_src[1],hw_src[2],hw_src[3],hw_src[4],hw_src[5]);
	ksmews_print("*IP address: %d.%d.%d.%d\n",local_ip_addr[3],local_ip_addr[2],local_ip_addr[1],local_ip_addr[0]);
	
//~ init_timer(&ksmews_timer);
//~ ksmews_timer.function=ksmews_callback;
//~ ksmews_timer.expires=jiffies;
//~ add_timer(&ksmews_timer);

	return 0;
}

/*-----------------------------------------------------------------------------------*/
static void __exit ksmews_exit(void) {
	/* restore the initial handlers for IP/ethernet packets */
	ksmews_restore_eth_handler(&ksmews_ip_packet_type);
	/* remove the timer */
	tasklet_kill(&ksmews_tasklet);
	//~ del_timer_sync(&ksmews_timer);
	ksmews_print("exiting\n");
}

/*-----------------------------------------------------------------------------------*/
module_init(ksmews_init);
module_exit(ksmews_exit);

MODULE_LICENSE("GPL");
