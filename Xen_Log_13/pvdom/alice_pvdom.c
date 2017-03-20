/* Demo: XenStore
 * Post: http://silentming.net/blog/2017/03/02/xen-log-13-xenstore/
 * This is kernel module under GPL License * Environment: Debian 8, Linux 4.10.2, Xen 4.5.1
 *
 * Compile:
 * make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 *
 * Run:
 * insmod alice_dom0.ko
 *
 * This Module is running in dom0 to write/read info to/from xenstore
 */

#include <linux/module.h>           /* Needed by all modules */
//#include <linux/moduleparam.h>      /* Param, dont need here indeed */
//#include <linux/kernel.h>           /* KERN_WARN etc */

//#include <xen/xen.h>                /* Included by events.h */
#include <xen/events.h>             /* Event channel */
#include <xen/interface/io/xs_wire.h>   /* interface */

#include <asm/xen/page.h>               /* gfn_to_virt & mfn_to_gfn */
//#include <asm/xen/hypercall.h>        /* Included by events */

/* Notify backend */
#define NOTIFY()\
    do {\
        struct evtchn_send event;\
        event.port = xenstore_evtchn;\
        HYPERVISOR_event_channel_op(EVTCHNOP_send, &event);\
    } while ( 0 )

/* Ignore some msg */
#define IGNORE(n)\
    do {\
        char buffer[XENSTORE_RING_SIZE];\
        read_response(buffer, n);\
    } while ( 0 )

/* Interface */
int alice_xs_write(char *key, char *value);
int alice_xs_read(char *key, char *value, int value_len);

/* Shared interface */
static struct xenstore_domain_interface *xenstore;
/* guest frame number */
static unsigned long xenstore_gfn;
/* Init req_id is 0 */
static int req_id = 0;
/* Event Channel of shared xenstore interface */
static evtchn_port_t xenstore_evtchn;

/* Fill request */
static int fill_request(char *msg, int len)
{
    int i;
    int ring_index;
    /* len must < XENSTORE_RING_SIZE */
    if ( len > XENSTORE_RING_SIZE )
        return -1;
    
    /* Wait for back end to clear enough space in buffer */
    /* Message may be long, we will clean byte by byte */
    for ( i = xenstore->req_prod; len > 0; i++, len-- ) {
        XENSTORE_RING_IDX data;
        do {
            data = i - xenstore->req_cons;
            mb();
        } while ( data >= sizeof(xenstore->req) ); /* Wait back end */

        /* Copy a byte */
        ring_index = MASK_XENSTORE_IDX(i);
        xenstore->req[ring_index] = *msg;
        msg ++;
    }

    /* Ensure data is into ring */
    wmb();
    xenstore->req_prod = i;
    return 0;
}

/* Read msg from response */
static int read_response(char *msg, int len)
{
    int i;
    int ring_index;
    for ( i = xenstore->rsp_cons; len > 0; i++, len-- ) {
        /* Wait back end fill data into response */
        XENSTORE_RING_IDX data;
        do {
            data = xenstore->rsp_prod - i;
            mb();
        } while ( data == 0 ); /* No res in ring */

        /* Copy this byte */
        ring_index = MASK_XENSTORE_IDX(i);
        *msg = xenstore->rsp[ring_index];
        msg++;
    }
    xenstore->rsp_cons = i;
    return 0;
}

int init_alice(void)
{
    /* Get shared xenstore interface */
    char buffer[1024];
    xenstore_gfn = xen_start_info->store_mfn;
    xenstore = gfn_to_virt(xenstore_gfn);

    xenstore_evtchn = xen_start_info->store_evtchn;

    /* We should setup event channel here.
     * But I will just add entry to xenstore in this demo
     * */
    pr_info("Alice: Begin xenstore test\n");
    alice_xs_read("name", buffer, 1023);
    pr_info("Alice: Name: %s\n", buffer);
    alice_xs_write("alice", "test");
    alice_xs_read("alice", buffer, 1023);
    pr_info("Alice: read alice:%s\n", buffer);
    return 0;
}

/* Write a value to store */
int alice_xs_write(char *key, char *value)
{
	int key_length = strlen(key);
	int value_length = strlen(value);
	struct xsd_sockmsg msg;

    /* Fill Message */
	msg.type = XS_WRITE;
	msg.req_id = req_id;
	msg.tx_id = 0; 
	msg.len = 2 + key_length + value_length;

	/* Write the message */
	fill_request((char*)&msg, sizeof(msg));
	fill_request(key, key_length + 1);
	fill_request(value, value_length + 1);

	/* Notify the back end */
    NOTIFY();
    
    /* Read response */
	read_response((char*)&msg, sizeof(msg));

    /* We dont need this info, just discard it */
    IGNORE(msg.len);

    /* Res is not right ? */
    if ( msg.req_id != req_id++ ) {
        pr_err("%s: Alice: res not right!\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

int alice_xs_read(char *key, char *value, int value_length)
{
	int key_length = strlen(key);
	struct xsd_sockmsg msg;

	msg.type = XS_READ;
	msg.req_id = req_id;
	msg.tx_id = 0; 
	msg.len = 1 + key_length;

	/* Write the message */
	fill_request((char*)&msg, sizeof(msg));
	fill_request(key, key_length + 1);

	/* Notify the back end */
	NOTIFY();

    /* Read value */
	read_response((char*)&msg, sizeof(msg));

    /* Res is not right */
	if(msg.req_id != req_id++)
	{
		IGNORE(msg.len);
		return -1;
	}

	/* If we have enough space in the buffer */
	if(value_length >= msg.len)
	{
		read_response(value, msg.len);
		return 0;
	}

	/* Truncate */
	read_response(value, value_length);
	IGNORE(msg.len - value_length);
	return -2;
}

void exit_alice(void)
{
    pr_info("Alice: Exit Successfully\n");
}


module_init(init_alice);
module_exit(exit_alice);

MODULE_LICENSE("GPL");
