/*++
/* NAME
/*	qmgr 8
/* SUMMARY
/*	Postfix queue manager
/* SYNOPSIS
/*	\fBqmgr\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBqmgr\fR daemon awaits the arrival of incoming mail
/*	and arranges for its delivery via Postfix delivery processes.
/*	The actual mail routing strategy is delegated to the
/*	\fBtrivial-rewrite\fR(8) daemon.
/*	This program expects to be run from the \fBmaster\fR(8) process
/*	manager.
/*
/*	Mail addressed to the local \fBdouble-bounce\fR address is silently
/*	discarded.  This stops potential loops caused by undeliverable
/*	bounce notifications.
/*
/*	Mail addressed to a user listed in the optional \fBrelocated\fR
/*	database is bounced with a "user has moved to \fInew_location\fR"
/*	message. See \fBrelocated\fR(5) for a precise description.
/* MAIL QUEUES
/* .ad
/* .fi
/*	The \fBqmgr\fR daemon maintains the following queues:
/* .IP \fBincoming\fR
/*	Inbound mail from the network, or mail picked up by the
/*	local \fBpickup\fR agent from the \fBmaildrop\fR directory.
/* .IP \fBactive\fR
/*	Messages that the queue manager has opened for delivery. Only
/*	a limited number of messages is allowed to enter the \fBactive\fR
/*	queue (leaky bucket strategy, for a fixed delivery rate).
/* .IP \fBdeferred\fR
/*	Mail that could not be delivered upon the first attempt. The queue
/*	manager implements exponential backoff by doubling the time between
/*	delivery attempts.
/* .IP \fBcorrupt\fR
/*	Unreadable or damaged queue files are moved here for inspection.
/* DELIVERY STATUS REPORTS
/* .ad
/* .fi
/*	The \fBqmgr\fR daemon keeps an eye on per-message delivery status
/*	reports in the following directories. Each status report file has
/*	the same name as the corresponding message file:
/* .IP \fBbounce\fR
/*	Per-recipient status information about why mail is bounced.
/*	These files are maintained by the \fBbounce\fR(8) daemon.
/* .IP \fBdefer\fR
/*	Per-recipient status information about why mail is delayed.
/*	These files are maintained by the \fBdefer\fR(8) daemon.
/* .PP
/*	The \fBqmgr\fR daemon is responsible for asking the
/*	\fBbounce\fR(8) or \fBdefer\fR(8) daemons to send non-delivery
/*	reports.
/* STRATEGIES
/* .ad
/* .fi
/*	The queue manager implements a variety of strategies for
/*	either opening queue files (input) or for message delivery (output).
/* .IP "\fBleaky bucket\fR"
/*	This strategy limits the number of messages in the \fBactive\fR queue
/*	and prevents the queue manager from running out of memory under
/*	heavy load.
/* .IP \fBfairness\fR
/*	When the \fBactive\fR queue has room, the queue manager takes one
/*	message from the \fBincoming\fR queue and one from the \fBdeferred\fR
/*	queue. This prevents a large mail backlog from blocking the delivery
/*	of new mail.
/* .IP "\fBslow start\fR"
/*	This strategy eliminates "thundering herd" problems by slowly
/*	adjusting the number of parallel deliveries to the same destination.
/* .IP "\fBround robin\fR
/*	The queue manager sorts delivery requests by destination.
/*	Round-robin selection prevents one destination from dominating
/*	deliveries to other destinations.
/* .IP "\fBexponential backoff\fR"
/*	Mail that cannot be delivered upon the first attempt is deferred.
/*	The time interval between delivery attempts is doubled after each
/*	attempt.
/* .IP "\fBdestination status cache\fR"
/*	The queue manager avoids unnecessary delivery attempts by
/*	maintaining a short-term, in-memory list of unreachable destinations.
/* .IP "\fBpreemptive message scheduling\fR"
/*	The queue manager attempts to minimize the average per-recipient delay
/*	while still preserving the correct per-message delays, using
/*	a sophisticated preemptive message scheduling.
/* TRIGGERS
/* .ad
/* .fi
/*	On an idle system, the queue manager waits for the arrival of
/*	trigger events, or it waits for a timer to go off. A trigger
/*	is a one-byte message.
/*	Depending on the message received, the queue manager performs
/*	one of the following actions (the message is followed by the
/*	symbolic constant used internally by the software):
/* .IP "\fBD (QMGR_REQ_SCAN_DEFERRED)\fR"
/*	Start a deferred queue scan.  If a deferred queue scan is already
/*	in progress, that scan will be restarted as soon as it finishes.
/* .IP "\fBI (QMGR_REQ_SCAN_INCOMING)\fR"
/*	Start an incoming queue scan. If an incoming queue scan is already
/*	in progress, that scan will be restarted as soon as it finishes.
/* .IP "\fBA (QMGR_REQ_SCAN_ALL)\fR"
/*	Ignore deferred queue file time stamps. The request affects
/*	the next deferred queue scan.
/* .IP "\fBF (QMGR_REQ_FLUSH_DEAD)\fR"
/*	Purge all information about dead transports and destinations.
/* .IP "\fBW (TRIGGER_REQ_WAKEUP)\fR"
/*	Wakeup call, This is used by the master server to instantiate
/*	servers that should not go away forever. The action is to start
/*	an incoming queue scan.
/* .PP
/*	The \fBqmgr\fR daemon reads an entire buffer worth of triggers.
/*	Multiple identical trigger requests are collapsed into one, and
/*	trigger requests are sorted so that \fBA\fR and \fBF\fR precede
/*	\fBD\fR and \fBI\fR. Thus, in order to force a deferred queue run,
/*	one would request \fBA F D\fR; in order to notify the queue manager
/*	of the arrival of new mail one would request \fBI\fR.
/* STANDARDS
/* .ad
/* .fi
/*	None. The \fBqmgr\fR daemon does not interact with the outside world.
/* SECURITY
/* .ad
/* .fi
/*	The \fBqmgr\fR daemon is not security sensitive. It reads
/*	single-character messages from untrusted local users, and thus may
/*	be susceptible to denial of service attacks. The \fBqmgr\fR daemon
/*	does not talk to the outside world, and it can be run at fixed low
/*	privilege in a chrooted environment.
/* DIAGNOSTICS
/*	Problems and transactions are logged to the syslog daemon.
/*	Corrupted message files are saved to the \fBcorrupt\fR queue
/*	for further inspection.
/*
/*	Depending on the setting of the \fBnotify_classes\fR parameter,
/*	the postmaster is notified of bounces and of other trouble.
/* BUGS
/*	A single queue manager process has to compete for disk access with
/*	multiple front-end processes such as \fBsmtpd\fR. A sudden burst of
/*	inbound mail can negatively impact outbound delivery rates.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*      The following \fBmain.cf\fR parameters are especially relevant to
/*      this program. See the Postfix \fBmain.cf\fR file for syntax details
/*      and for default values. Use the \fBpostfix reload\fR command after
/*      a configuration change.
/* .SH Miscellaneous
/* .ad
/* .fi
/* .IP \fBallow_min_user\fR
/*	Do not bounce recipient addresses that begin with '-'.
/* .IP \fBrelocated_maps\fR
/*	Tables with contact information for users, hosts or domains
/*	that no longer exist. See \fBrelocated\fR(5).
/* .IP \fBqueue_directory\fR
/*	Top-level directory of the Postfix queue.
/* .SH "Active queue controls"
/* .ad
/* .fi
/*	In the text below, \fItransport\fR is the first field in a
/*	\fBmaster.cf\fR entry.
/* .IP \fBqmgr_message_active_limit\fR
/*	Limit the number of messages in the active queue.
/* .IP \fBqmgr_message_recipient_limit\fR
/*	Limit the number of in-memory recipients.
/* .sp
/*	This parameter also limits the size of the short-term, in-memory
/*	destination cache.
/* .IP \fBqmgr_message_recipient_minimum\fR
/*	Per message minimum of in-memory recipients.
/* .IP \fBdefault_recipient_limit\fR
/*	Default limit on the number of in-memory recipients per transport.
/* .IP \fItransport\fB_recipient_limit\fR
/*	Limit on the number of in-memory recipients, for the named
/*	message \fItransport\fR.
/* .IP \fBdefault_extra_recipient_limit\fR
/*	Default limit on the total number of per transport in-memory
/*	recipients that the preempting messages can have.
/* .IP \fItransport\fB_extra_recipient_limit\fR
/*	Limit on the number of in-memory recipients which all preempting
/*	messages delivered by the transport \fItransport\fR can have.
/* .SH "Timing controls"
/* .ad
/* .fi
/* .IP \fBmin_backoff\fR
/*	Minimal time in seconds between delivery attempts
/*	of a deferred message.
/* .sp
/*	This parameter also limits the time an unreachable destination
/*	is kept in the short-term, in-memory destination status cache.
/* .IP \fBmax_backoff\fR
/*	Maximal time in seconds between delivery attempts
/*	of a deferred message.
/* .IP \fBmaximal_queue_lifetime\fR
/*	Maximal time in days a message is queued
/*	before it is sent back as undeliverable.
/* .IP \fBqueue_run_delay\fR
/*	Time in seconds between deferred queue scans. Queue scans do
/*	not overlap.
/* .IP \fBtransport_retry_time\fR
/*	Time in seconds between attempts to contact a broken
/*	delivery transport.
/* .SH "Concurrency controls"
/* .ad
/* .fi
/* .IP \fBinitial_destination_concurrency\fR
/*	Initial per-destination concurrency level for parallel delivery
/*	to the same destination.
/* .IP \fBdefault_destination_concurrency_limit\fR
/*	Default limit on the number of parallel deliveries to the same
/*	destination.
/* .IP \fItransport\fB_destination_concurrency_limit\fR
/*	Limit on the number of parallel deliveries to the same destination,
/*	for delivery via the named message \fItransport\fR.
/* .SH "Recipient controls"
/* .ad
/* .fi
/* .IP \fBdefault_destination_recipient_limit\fR
/*	Default limit on the number of recipients per message transfer.
/* .IP \fItransport\fB_destination_recipient_limit\fR
/*	Limit on the number of recipients per message transfer, for the
/*	named message \fItransport\fR.
/* .SH "Message scheduling"
/* .ad
/* .fi
/* .IP "\fItransport\fB_delivery_slot_cost\fR (valid range: 0,2,3...)
/*	This parameter basically controls how often a message
/*	delivered by \fItransport\fR can be preempted by another
/*	message.
/*	An internal per-message/transport counter is incremented by one
/*	for each \fItransport\fB_delivery_slot_cost\fR
/*	deliveries handled by \fItransport\fR. This counter represents
/*	the number of "available delivery slots" for use by other messages.
/*	Current message can be preempted by another message when that
/*	other message can be delivered using less \fItransport\fR agents
/*	than the value of the "available delivery slots" counter.
/* .sp
/*	Value equal to 0 disables the message preemption for \fItransport\fR.
/* .IP \fItransport\fB_minimum_delivery_slots\fR
/*	Message preemption is not attempted at all whenever a message
/*	that can't ever accumulate at least \fItransport\fB_minimum_delivery_slots\fR
/*	available delivery slots is being delivered by \fItransport\fR.
/* .IP "\fItransport\fB_delivery_slot_discount\fR (valid range: 0..100)"
/* .IP \fItransport\fB_delivery_slot_loan\fR
/*	These parameters speed up the moment when a message preemption can happen.
/*	Instead of waiting until the full amount of delivery slots
/*	required is available, the preemption can happen when
/*	\fItransport\fB_delivery_slot_discount\fR percent of the required
/*	amount plus \fItransport\fB_delivery_slot_loan\fR still remains to
/*	be accumulated. Note that the full amount will still have to be
/*	accumulated before another preemption can take place later.
/* .IP \fBdefault_delivery_slot_cost\fR
/* .IP \fBdefault_minimum_delivery_slots\fR
/* .IP \fBdefault_delivery_slot_discount\fR
/* .IP \fBdefault_delivery_slot_loan\fR
/*	Default values for the transport specific parameters described above.
/* SEE ALSO
/*	master(8), process manager
/*	relocated(5), format of the "user has moved" table
/*	syslogd(8) system logging
/*	trivial-rewrite(8), address routing
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Scheduler enhancements:
/*	Patrik Rak
/*	Modra 6
/*	155 00, Prague, Czech Republic
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <events.h>
#include <vstream.h>
#include <dict.h>

/* Global library. */

#include <mail_queue.h>
#include <recipient_list.h>
#include <mail_conf.h>
#include <mail_params.h>
#include <mail_proto.h>			/* QMGR_SCAN constants */

/* Master process interface */

#include <master_proto.h>
#include <mail_server.h>

/* Application-specific. */

#include "qmgr.h"

 /*
  * Tunables.
  */
int     var_queue_run_delay;
int     var_min_backoff_time;
int     var_max_backoff_time;
int     var_max_queue_time;
int     var_qmgr_active_limit;
int     var_qmgr_rcpt_limit;
int     var_qmgr_msg_rcpt_limit;
int     var_xport_rcpt_limit;
int     var_stack_rcpt_limit;
int     var_delivery_slot_cost;
int     var_delivery_slot_loan;
int     var_delivery_slot_discount;
int     var_min_delivery_slots;
int     var_init_dest_concurrency;
int     var_transport_retry_time;
int     var_dest_con_limit;
int     var_dest_rcpt_limit;
char   *var_relocated_maps;
char   *var_virtual_maps;
char   *var_defer_xports;
bool    var_allow_min_user;
char   *var_fflush_maps;

static QMGR_SCAN *qmgr_incoming;
static QMGR_SCAN *qmgr_deferred;

MAPS   *qmgr_relocated;
MAPS   *qmgr_virtual;
MAPS   *qmgr_fflush;

/* qmgr_deferred_run_event - queue manager heartbeat */

static void qmgr_deferred_run_event(int unused_event, char *dummy)
{

    /*
     * This routine runs when it is time for another deferred queue scan.
     * Make sure this routine gets called again in the future.
     */
    qmgr_scan_request(qmgr_deferred, QMGR_SCAN_START);
    event_request_timer(qmgr_deferred_run_event, dummy, var_queue_run_delay);
}

/* qmgr_trigger_event - respond to external trigger(s) */

static void qmgr_trigger_event(char *buf, int len,
			               char *unused_service, char **argv)
{
    int     incoming_flag = 0;
    int     deferred_flag = 0;
    int     i;

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * Collapse identical requests that have arrived since we looked last
     * time. There is no client feedback so there is no need to process each
     * request in order. And as long as we don't have conflicting requests we
     * are free to sort them into the most suitable order.
     */
    for (i = 0; i < len; i++) {
	if (msg_verbose)
	    msg_info("request: %d (%c)",
		     buf[i], ISALNUM(buf[i]) ? buf[i] : '?');
	switch (buf[i]) {
	case TRIGGER_REQ_WAKEUP:
	case QMGR_REQ_SCAN_INCOMING:
	    incoming_flag |= QMGR_SCAN_START;
	    break;
	case QMGR_REQ_SCAN_DEFERRED:
	    deferred_flag |= QMGR_SCAN_START;
	    break;
	case QMGR_REQ_FLUSH_DEAD:
	    deferred_flag |= QMGR_FLUSH_DEAD;
	    incoming_flag |= QMGR_FLUSH_DEAD;
	    break;
	case QMGR_REQ_SCAN_ALL:
	    deferred_flag |= QMGR_SCAN_ALL;
	    incoming_flag |= QMGR_SCAN_ALL;
	    break;
	default:
	    if (msg_verbose)
		msg_info("request ignored");
	    break;
	}
    }

    /*
     * Process each request type at most once. Modifiers take effect upon the
     * next queue run. If no queue run is in progress, and a queue scan is
     * requested, the request takes effect immediately.
     */
    if (incoming_flag != 0)
	qmgr_scan_request(qmgr_incoming, incoming_flag);
    if (deferred_flag != 0)
	qmgr_scan_request(qmgr_deferred, deferred_flag);
}

/* qmgr_loop - queue manager main loop */

static int qmgr_loop(char *unused_name, char **unused_argv)
{
    char   *in_path = 0;
    char   *df_path = 0;

    /*
     * This routine runs as part of the event handling loop, after the event
     * manager has delivered a timer or I/O event (including the completion
     * of a connection to a delivery process), or after it has waited for a
     * specified amount of time. The result value of qmgr_loop() specifies
     * how long the event manager should wait for the next event.
     */
#define DONT_WAIT	0
#define WAIT_FOR_EVENT	(-1)

    /*
     * Attempt to drain the active queue by allocating a suitable delivery
     * process and by delivering mail via it. Delivery process allocation and
     * mail delivery are asynchronous.
     */
    qmgr_active_drain();

    /*
     * Let some new blood into the active queue when the queue size is
     * smaller than some configurable limit. When the system is under heavy
     * load, favor new mail over old mail.
     */
    if (qmgr_message_count < var_qmgr_active_limit)
	if ((in_path = qmgr_scan_next(qmgr_incoming)) != 0)
	    qmgr_active_feed(qmgr_incoming, in_path);
    if (qmgr_message_count < var_qmgr_active_limit)
	if ((df_path = qmgr_scan_next(qmgr_deferred)) != 0)
	    qmgr_active_feed(qmgr_deferred, df_path);
    if (in_path || df_path)
	return (DONT_WAIT);
    return (WAIT_FOR_EVENT);
}

/* pre_accept - see if tables have changed */

static void pre_accept(char *unused_name, char **unused_argv)
{
    if (dict_changed()) {
	msg_info("table has changed -- exiting");
	exit(0);
    }
}

/* qmgr_pre_init - pre-jail initialization */

static void qmgr_pre_init(char *unused_name, char **unused_argv)
{
    if (*var_relocated_maps)
	qmgr_relocated = maps_create("relocated", var_relocated_maps,
				     DICT_FLAG_LOCK);
    if (*var_virtual_maps)
	qmgr_virtual = maps_create("virtual", var_virtual_maps,
				   DICT_FLAG_LOCK);
    if (*var_fflush_maps)
	qmgr_fflush = maps_create(VAR_FFLUSH_MAPS, var_fflush_maps,
				  DICT_FLAG_LOCK);
}

/* qmgr_post_init - post-jail initialization */

static void qmgr_post_init(char *unused_name, char **unused_argv)
{

    /*
     * This routine runs after the skeleton code has entered the chroot jail.
     * Prevent automatic process suicide after a limited number of client
     * requests or after a limited amount of idle time. Move any left-over
     * entries from the active queue to the incoming queue, and give them a
     * time stamp into the future, in order to allow ongoing deliveries to
     * finish first. Start scanning the incoming and deferred queues.
     * Left-over active queue entries are moved to the incoming queue because
     * the incoming queue has priority; moving left-overs to the deferred
     * queue could cause anomalous delays when "postfix reload/start" are
     * issued often.
     */
    var_use_limit = 0;
    var_idle_limit = 0;
    qmgr_move(MAIL_QUEUE_ACTIVE, MAIL_QUEUE_INCOMING,
	      event_time() + var_min_backoff_time);
    qmgr_incoming = qmgr_scan_create(MAIL_QUEUE_INCOMING);
    qmgr_deferred = qmgr_scan_create(MAIL_QUEUE_DEFERRED);
    qmgr_scan_request(qmgr_incoming, QMGR_SCAN_START);
    qmgr_deferred_run_event(0, (char *) 0);
}

/* main - the main program */

int     main(int argc, char **argv)
{
    static CONFIG_STR_TABLE str_table[] = {
	VAR_RELOCATED_MAPS, DEF_RELOCATED_MAPS, &var_relocated_maps, 0, 0,
	VAR_VIRTUAL_MAPS, DEF_VIRTUAL_MAPS, &var_virtual_maps, 0, 0,
	VAR_DEFER_XPORTS, DEF_DEFER_XPORTS, &var_defer_xports, 0, 0,
	VAR_FFLUSH_MAPS, DEF_FFLUSH_MAPS, &var_fflush_maps, 0, 0,
	0,
    };
    static CONFIG_INT_TABLE int_table[] = {
	VAR_QUEUE_RUN_DELAY, DEF_QUEUE_RUN_DELAY, &var_queue_run_delay, 1, 0,
	VAR_MIN_BACKOFF_TIME, DEF_MIN_BACKOFF_TIME, &var_min_backoff_time, 1, 0,
	VAR_MAX_BACKOFF_TIME, DEF_MAX_BACKOFF_TIME, &var_max_backoff_time, 1, 0,
	VAR_MAX_QUEUE_TIME, DEF_MAX_QUEUE_TIME, &var_max_queue_time, 1, 1000,
	VAR_QMGR_ACT_LIMIT, DEF_QMGR_ACT_LIMIT, &var_qmgr_active_limit, 1, 0,
	VAR_QMGR_RCPT_LIMIT, DEF_QMGR_RCPT_LIMIT, &var_qmgr_rcpt_limit, 0, 0,
	VAR_QMGR_MSG_RCPT_LIMIT, DEF_QMGR_MSG_RCPT_LIMIT, &var_qmgr_msg_rcpt_limit, 1, 0,
	VAR_XPORT_RCPT_LIMIT, DEF_XPORT_RCPT_LIMIT, &var_xport_rcpt_limit, 0, 0,
	VAR_STACK_RCPT_LIMIT, DEF_STACK_RCPT_LIMIT, &var_stack_rcpt_limit, 0, 0,
	VAR_DELIVERY_SLOT_COST, DEF_DELIVERY_SLOT_COST, &var_delivery_slot_cost, 0, 0,
	VAR_DELIVERY_SLOT_LOAN, DEF_DELIVERY_SLOT_LOAN, &var_delivery_slot_loan, 0, 0,
	VAR_DELIVERY_SLOT_DISCOUNT, DEF_DELIVERY_SLOT_DISCOUNT, &var_delivery_slot_discount, 0, 100,
	VAR_MIN_DELIVERY_SLOTS, DEF_MIN_DELIVERY_SLOTS, &var_min_delivery_slots, 0, 0,
	VAR_INIT_DEST_CON, DEF_INIT_DEST_CON, &var_init_dest_concurrency, 1, 0,
	VAR_XPORT_RETRY_TIME, DEF_XPORT_RETRY_TIME, &var_transport_retry_time, 1, 0,
	VAR_DEST_CON_LIMIT, DEF_DEST_CON_LIMIT, &var_dest_con_limit, 0, 0,
	VAR_DEST_RCPT_LIMIT, DEF_DEST_RCPT_LIMIT, &var_dest_rcpt_limit, 0, 0,
	0,
    };
    static CONFIG_BOOL_TABLE bool_table[] = {
	VAR_ALLOW_MIN_USER, DEF_ALLOW_MIN_USER, &var_allow_min_user,
	0,
    };

    /*
     * Use the trigger service skeleton, because no-one else should be
     * monitoring our service port while this process runs, and because we do
     * not talk back to the client.
     */
    trigger_server_main(argc, argv, qmgr_trigger_event,
			MAIL_SERVER_INT_TABLE, int_table,
			MAIL_SERVER_STR_TABLE, str_table,
			MAIL_SERVER_BOOL_TABLE, bool_table,
			MAIL_SERVER_PRE_INIT, qmgr_pre_init,
			MAIL_SERVER_POST_INIT, qmgr_post_init,
			MAIL_SERVER_LOOP, qmgr_loop,
			MAIL_SERVER_PRE_ACCEPT, pre_accept,
			0);
}