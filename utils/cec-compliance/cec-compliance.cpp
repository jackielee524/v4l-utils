// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2016 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 */

#include <sstream>

#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include "cec-compliance.h"
#include "compiler.h"

/* Short option list

   Please keep in alphabetical order.
   That makes it easier to see which short options are still free.

   In general the lower case is used to set something and the upper
   case is used to retrieve a setting. */
enum Option {
	OptSetAdapter = 'a',
	OptTestAdapter = 'A',
	OptColor = 'C',
	OptSetDevice = 'd',
	OptSetDriver = 'D',
	OptExpect = 'e',
	OptExitOnFail = 'E',
	OptTestFuzzing = 'F',
	OptHelp = 'h',
	OptInteractive = 'i',
	OptListTests = 'l',
	OptExpectWithNoWarnings = 'n',
	OptNoWarnings = 'N',
	OptRemote = 'r',
	OptReplyThreshold = 'R',
	OptSkipInfo = 's',
	OptShowTimestamp = 'S',
	OptTimeout = 't',
	OptTrace = 'T',
	OptVerbose = 'v',
	OptWallClock = 'w',
	OptExitOnWarn = 'W',

	OptTestCore = 128,
	OptTestAudioRateControl,
	OptTestARCControl,
	OptTestCapDiscoveryControl,
	OptTestDeckControl,
	OptTestDeviceMenuControl,
	OptTestDeviceOSDTransfer,
	OptTestDynamicAutoLipsync,
	OptTestOSDDisplay,
	OptTestOneTouchPlay,
	OptTestOneTouchRecord,
	OptTestPowerStatus,
	OptTestRemoteControlPassthrough,
	OptTestRoutingControl,
	OptTestSystemAudioControl,
	OptTestSystemInformation,
	OptTestTimerProgramming,
	OptTestTunerControl,
	OptTestVendorSpecificCommands,
	OptTestStandbyResume,

	OptSkipTestAudioRateControl,
	OptSkipTestARCControl,
	OptSkipTestCapDiscoveryControl,
	OptSkipTestDeckControl,
	OptSkipTestDeviceMenuControl,
	OptSkipTestDeviceOSDTransfer,
	OptSkipTestDynamicAutoLipsync,
	OptSkipTestOSDDisplay,
	OptSkipTestOneTouchPlay,
	OptSkipTestOneTouchRecord,
	OptSkipTestPowerStatus,
	OptSkipTestRemoteControlPassthrough,
	OptSkipTestRoutingControl,
	OptSkipTestSystemAudioControl,
	OptSkipTestSystemInformation,
	OptSkipTestTimerProgramming,
	OptSkipTestTunerControl,
	OptSkipTestVendorSpecificCommands,
	OptSkipTestStandbyResume,

	OptVersion,
	OptLast = 256
};


static char options[OptLast];

static int app_result;
static int tests_total, tests_ok;

bool show_info;
bool show_colors;
bool show_warnings = true;
bool exit_on_fail;
bool exit_on_warn;
unsigned warnings;
unsigned reply_threshold = 1000;
time_t long_timeout = 60;

static struct option long_options[] = {
	{"device", required_argument, nullptr, OptSetDevice},
	{"adapter", required_argument, nullptr, OptSetAdapter},
	{"driver", required_argument, nullptr, OptSetDriver},
	{"help", no_argument, nullptr, OptHelp},
	{"no-warnings", no_argument, nullptr, OptNoWarnings},
	{"exit-on-fail", no_argument, nullptr, OptExitOnFail},
	{"exit-on-warn", no_argument, nullptr, OptExitOnWarn},
	{"remote", optional_argument, nullptr, OptRemote},
	{"list-tests", no_argument, nullptr, OptListTests},
	{"expect", required_argument, nullptr, OptExpect},
	{"expect-with-no-warnings", required_argument, nullptr, OptExpectWithNoWarnings},
	{"timeout", required_argument, nullptr, OptTimeout},
	{"trace", no_argument, nullptr, OptTrace},
	{"verbose", no_argument, nullptr, OptVerbose},
	{"color", required_argument, nullptr, OptColor},
	{"skip-info", no_argument, nullptr, OptSkipInfo},
	{"wall-clock", no_argument, nullptr, OptWallClock},
	{"show-timestamp", no_argument, nullptr, OptShowTimestamp},
	{"interactive", no_argument, nullptr, OptInteractive},
	{"reply-threshold", required_argument, nullptr, OptReplyThreshold},

	{"test-adapter", no_argument, nullptr, OptTestAdapter},
	{"test-fuzzing", no_argument, nullptr, OptTestFuzzing},
	{"test-core", no_argument, nullptr, OptTestCore},
	{"test-audio-rate-control", no_argument, nullptr, OptTestAudioRateControl},
	{"test-audio-return-channel-control", no_argument, nullptr, OptTestARCControl},
	{"test-capability-discovery-and-control", no_argument, nullptr, OptTestCapDiscoveryControl},
	{"test-deck-control", no_argument, nullptr, OptTestDeckControl},
	{"test-device-menu-control", no_argument, nullptr, OptTestDeviceMenuControl},
	{"test-device-osd-transfer", no_argument, nullptr, OptTestDeviceOSDTransfer},
	{"test-dynamic-auto-lipsync", no_argument, nullptr, OptTestDynamicAutoLipsync},
	{"test-osd-display", no_argument, nullptr, OptTestOSDDisplay},
	{"test-one-touch-play", no_argument, nullptr, OptTestOneTouchPlay},
	{"test-one-touch-record", no_argument, nullptr, OptTestOneTouchRecord},
	{"test-power-status", no_argument, nullptr, OptTestPowerStatus},
	{"test-remote-control-passthrough", no_argument, nullptr, OptTestRemoteControlPassthrough},
	{"test-routing-control", no_argument, nullptr, OptTestRoutingControl},
	{"test-system-audio-control", no_argument, nullptr, OptTestSystemAudioControl},
	{"test-system-information", no_argument, nullptr, OptTestSystemInformation},
	{"test-timer-programming", no_argument, nullptr, OptTestTimerProgramming},
	{"test-tuner-control", no_argument, nullptr, OptTestTunerControl},
	{"test-vendor-specific-commands", no_argument, nullptr, OptTestVendorSpecificCommands},
	{"test-standby-resume", no_argument, nullptr, OptTestStandbyResume},

	{"skip-test-audio-rate-control", no_argument, nullptr, OptSkipTestAudioRateControl},
	{"skip-test-audio-return-channel-control", no_argument, nullptr, OptSkipTestARCControl},
	{"skip-test-capability-discovery-and-control", no_argument, nullptr, OptSkipTestCapDiscoveryControl},
	{"skip-test-deck-control", no_argument, nullptr, OptSkipTestDeckControl},
	{"skip-test-device-menu-control", no_argument, nullptr, OptSkipTestDeviceMenuControl},
	{"skip-test-device-osd-transfer", no_argument, nullptr, OptSkipTestDeviceOSDTransfer},
	{"skip-test-dynamic-auto-lipsync", no_argument, nullptr, OptSkipTestDynamicAutoLipsync},
	{"skip-test-osd-display", no_argument, nullptr, OptSkipTestOSDDisplay},
	{"skip-test-one-touch-play", no_argument, nullptr, OptSkipTestOneTouchPlay},
	{"skip-test-one-touch-record", no_argument, nullptr, OptSkipTestOneTouchRecord},
	{"skip-test-power-status", no_argument, nullptr, OptSkipTestPowerStatus},
	{"skip-test-remote-control-passthrough", no_argument, nullptr, OptSkipTestRemoteControlPassthrough},
	{"skip-test-routing-control", no_argument, nullptr, OptSkipTestRoutingControl},
	{"skip-test-system-audio-control", no_argument, nullptr, OptSkipTestSystemAudioControl},
	{"skip-test-system-information", no_argument, nullptr, OptSkipTestSystemInformation},
	{"skip-test-timer-programming", no_argument, nullptr, OptSkipTestTimerProgramming},
	{"skip-test-tuner-control", no_argument, nullptr, OptSkipTestTunerControl},
	{"skip-test-vendor-specific-commands", no_argument, nullptr, OptSkipTestVendorSpecificCommands},
	{"skip-test-standby-resume", no_argument, nullptr, OptSkipTestStandbyResume},
	{"version", no_argument, nullptr, OptVersion},
	{nullptr, 0, nullptr, 0}
};

#define STR(x) #x
#define STRING(x) STR(x)

static void usage()
{
	printf("Usage:\n"
	       "  -d, --device <dev>   Use device <dev> instead of /dev/cec0\n"
	       "                       If <dev> starts with a digit, then /dev/cec<dev> is used.\n"
	       "  -D, --driver <driver>    Use a cec device with this driver name\n"
	       "  -a, --adapter <adapter>  Use a cec device with this adapter name\n"
	       "  -r, --remote [<la>]  As initiator test the remote logical address or all LAs if no LA was given\n"
	       "  -R, --reply-threshold <timeout>\n"
	       "                       Warn if replies take longer than this threshold (default 1000ms)\n"
	       "  -i, --interactive    Interactive mode when doing remote tests\n"
	       "  -t, --timeout <secs> Set the standby/resume timeout to <secs>. Default is 60s.\n"
	       "\n"
	       "  -A, --test-adapter                  Test the CEC adapter API\n"
	       "  -F, --test-fuzzing                  Test by fuzzing CEC messages\n"
	       "  --test-core                         Test the core functionality\n"
	       "\n"
	       "By changing --test to --skip-test in the following options you can skip tests\n"
	       "instead of enabling them.\n"
	       "\n"
	       "  --test-audio-rate-control           Test the Audio Rate Control feature\n"
	       "  --test-audio-return-channel-control Test the Audio Return Channel Control feature\n"
	       "  --test-capability-discovery-and-control Test the Capability Discovery and Control feature\n"
	       "  --test-deck-control                 Test the Deck Control feature\n"
	       "  --test-device-menu-control          Test the Device Menu Control feature\n"
	       "  --test-device-osd-transfer          Test the Device OSD Transfer feature\n"
	       "  --test-dynamic-auto-lipsync         Test the Dynamic Auto Lipsync feature\n"
	       "  --test-osd-display                  Test the OSD Display feature\n"
	       "  --test-one-touch-play               Test the One Touch Play feature\n"
	       "  --test-one-touch-record             Test the One Touch Record feature\n"
	       "  --test-power-status                 Test the Power Status feature\n"
	       "  --test-remote-control-passthrough   Test the Remote Control Passthrough feature\n"
	       "  --test-routing-control              Test the Routing Control feature\n"
	       "  --test-system-audio-control         Test the System Audio Control feature\n"
	       "  --test-system-information           Test the System Information feature\n"
	       "  --test-timer-programming            Test the Timer Programming feature\n"
	       "  --test-tuner-control                Test the Tuner Control feature\n"
	       "  --test-vendor-specific-commands     Test the Vendor Specific Commands feature\n"
	       "  --test-standby-resume               Test standby and resume functionality. This will activate\n"
	       "                                      testing of Standby, Give Device Power Status and One Touch Play.\n"
	       "\n"
	       "  -E, --exit-on-fail Exit on the first fail.\n"
	       "  -l, --list-tests   List all tests.\n"
	       "  -e, --expect <test>=<result>\n"
	       "                     Fail if the test gave a different result.\n"
	       "  -n, --expect-with-no-warnings <test>=<result>\n"
	       "                     Fail if the test gave a different result or if the test generated warnings.\n"
	       "  -h, --help         Display this help message\n"
	       "  -C, --color <when> Highlight OK/warn/fail/FAIL strings with colors\n"
	       "                     <when> can be set to always, never, or auto (the default)\n"
	       "  -N, --no-warnings  Turn off warning messages\n"
	       "  -s, --skip-info    Skip Driver Info output\n"
	       "  -T, --trace        Trace all called ioctls\n"
	       "  -v, --verbose      Turn on verbose reporting\n"
	       "  --version          Show version information\n"
	       "  -w, --wall-clock   Show timestamps as wall-clock time\n"
	       "  -S, --show-timestamp Show timestamp of the start of each test\n"
	       "  -W, --exit-on-warn Exit on the first warning.\n"
	       );
}

std::string safename(const char *name)
{
	std::string s;
	bool not_alnum = false;

	while (*name) {
		if (isalnum(*name)) {
			if (not_alnum && !s.empty())
				s += '-';
			s += tolower(*name);
			not_alnum = false;
		} else if (!not_alnum)
			not_alnum = true;
		name++;
	}
	return s;
}

static std::string ts2s(__u64 ts)
{
	std::string s;
	struct timespec now;
	struct timeval tv;
	struct timeval sub;
	struct timeval res;
	__u64 diff;
	char buf[64];
	time_t t;

	if (!options[OptWallClock]) {
		sprintf(buf, "%llu.%03llus", ts / 1000000000, (ts % 1000000000) / 1000000);
		return buf;
	}
	clock_gettime(CLOCK_MONOTONIC, &now);
	gettimeofday(&tv, nullptr);
	diff = now.tv_sec * 1000000000ULL + now.tv_nsec - ts;
	sub.tv_sec = diff / 1000000000ULL;
	sub.tv_usec = (diff % 1000000000ULL) / 1000;
	timersub(&tv, &sub, &res);
	t = res.tv_sec;
	s = ctime(&t);
	s = s.substr(0, s.length() - 6);
	sprintf(buf, "%03llu", (__u64)res.tv_usec / 1000);
	return s + "." + buf;
}

std::string current_ts()
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts2s(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

const char *power_status2s(__u8 power_status)
{
	switch (power_status) {
	case CEC_OP_POWER_STATUS_ON:
		return "On";
	case CEC_OP_POWER_STATUS_STANDBY:
		return "Standby";
	case CEC_OP_POWER_STATUS_TO_ON:
		return "In transition Standby to On";
	case CEC_OP_POWER_STATUS_TO_STANDBY:
		return "In transition On to Standby";
	default:
		return "Unknown";
	}
}

const char *bcast_system2s(__u8 bcast_system)
{
	switch (bcast_system) {
	case CEC_OP_BCAST_SYSTEM_PAL_BG:
		return "PAL B/G";
	case CEC_OP_BCAST_SYSTEM_SECAM_LQ:
		return "SECAM L'";
	case CEC_OP_BCAST_SYSTEM_PAL_M:
		return "PAL M";
	case CEC_OP_BCAST_SYSTEM_NTSC_M:
		return "NTSC M";
	case CEC_OP_BCAST_SYSTEM_PAL_I:
		return "PAL I";
	case CEC_OP_BCAST_SYSTEM_SECAM_DK:
		return "SECAM DK";
	case CEC_OP_BCAST_SYSTEM_SECAM_BG:
		return "SECAM B/G";
	case CEC_OP_BCAST_SYSTEM_SECAM_L:
		return "SECAM L";
	case CEC_OP_BCAST_SYSTEM_PAL_DK:
		return "PAL DK";
	case 31:
		return "Other System";
	default:
		return "Future use";
	}
}

const char *dig_bcast_system2s(__u8 bcast_system)
{
	switch (bcast_system) {
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_ARIB_GEN:
		return "ARIB generic";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_ATSC_GEN:
		return "ATSC generic";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_DVB_GEN:
		return "DVB generic";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_ARIB_BS:
		return "ARIB-BS";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_ARIB_CS:
		return "ARIB-CS";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_ARIB_T:
		return "ARIB-T";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_ATSC_CABLE:
		return "ATSC Cable";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_ATSC_SAT:
		return "ATSC Satellite";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_ATSC_T:
		return "ATSC Terrestrial";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_DVB_C:
		return "DVB-C";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_DVB_S:
		return "DVB-S";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_DVB_S2:
		return "DVB S2";
	case CEC_OP_DIG_SERVICE_BCAST_SYSTEM_DVB_T:
		return "DVB-T";
	default:
		return "Invalid";
	}
}

std::string opcode2s(const struct cec_msg *msg)
{
	std::stringstream oss;
	__u8 opcode = msg->msg[1];
	const char *name;

	if (msg->len == 1)
		return "MSG_POLL";

	if (opcode == CEC_MSG_CDC_MESSAGE) {
		__u8 cdc_opcode = msg->msg[4];
		name = cec_cdc_opcode2s(cdc_opcode);

		if (name)
			return name;
		oss << "CDC: 0x" << std::hex << static_cast<unsigned>(cdc_opcode);
		return oss.str();
	}

	name = cec_opcode2s(opcode);

	if (name)
		return name;
	oss << "0x" << std::hex << static_cast<unsigned>(opcode);
	return oss.str();
}

int cec_named_ioctl(struct node *node, const char *name,
		    unsigned long int request, void *parm)
{
	int retval;
	int e;
	auto msg = static_cast<struct cec_msg *>(parm);
	__u8 opcode = 0;
	std::string opname;

	if (request == CEC_TRANSMIT) {
		opcode = cec_msg_opcode(msg);
		opname = opcode2s(msg);
	}

	retval = ioctl(node->fd, request, parm);

	if (request == CEC_RECEIVE) {
		opcode = cec_msg_opcode(msg);
		opname = opcode2s(msg);
	}

	e = retval == 0 ? 0 : errno;
	if (options[OptTrace] && (e || !show_info ||
			(request != CEC_TRANSMIT && request != CEC_RECEIVE))) {
		if (request == CEC_TRANSMIT)
			printf("\t\t%s: %s returned %d (%s)\n",
				opname.c_str(), name, retval, strerror(e));
		else
			printf("\t\t%s returned %d (%s)\n",
				name, retval, strerror(e));
	}

	if (!retval && request == CEC_TRANSMIT &&
	    (msg->tx_status & CEC_TX_STATUS_OK) && ((msg->tx_status & CEC_TX_STATUS_MAX_RETRIES))) {
		/*
		 * Workaround this bug in the CEC framework. This bug was solved
		 * in kernel 4.18 but older versions still can produce this incorrect
		 * combination of TX flags. If this occurs, then this really means
		 * that the transmit went OK, but the wait for the reply was
		 * cancelled (e.g. due to the HPD doing down).
		 */
		msg->tx_status &= ~(CEC_TX_STATUS_MAX_RETRIES | CEC_TX_STATUS_ERROR);
		if (msg->tx_error_cnt)
			msg->tx_error_cnt--;
		msg->rx_status = CEC_RX_STATUS_TIMEOUT;
		msg->rx_ts = msg->tx_ts;
		warn("Both OK and MAX_RETRIES were set in tx_status! Applied workaround.\n");
	}

	if (!retval && show_info &&
	    (request == CEC_TRANSMIT || request == CEC_RECEIVE)) {
		printf("\t\t%s: Sequence: %u Length: %u\n",
		       opname.c_str(), msg->sequence, msg->len);
		if (msg->tx_ts || msg->rx_ts) {
			printf("\t\t\t");
			if (msg->tx_ts)
				printf("Tx Timestamp: %s ", ts2s(msg->tx_ts).c_str());
			if (msg->rx_ts)
				printf("Rx Timestamp: %s", ts2s(msg->rx_ts).c_str());
			printf("\n");
			if (msg->tx_ts && msg->rx_ts)
				printf("\t\t\tApproximate response time: %u ms\n",
				       response_time_ms(msg));
		}
		if ((msg->tx_status & ~CEC_TX_STATUS_OK) ||
		    (msg->rx_status & ~CEC_RX_STATUS_OK))
			printf("\t\t\tStatus: %s\n", cec_status2s(*msg).c_str());
		if (msg->tx_status & CEC_TX_STATUS_TIMEOUT)
			warn("CEC_TX_STATUS_TIMEOUT was set, should not happen.\n");
	}

	if (!retval) {
		__u8 la = cec_msg_initiator(msg);

		/*
		 * TODO: The logic here might need to be re-evaluated.
		 *
		 * Currently a message is registered as recognized if
		 *     - We receive a reply that is not Feature Abort with
		 *       [Unrecognized Opcode] or [Undetermined]
		 *     - We manually receive (CEC_RECEIVE) and get a Feature Abort
		 *       with reason different than [Unrecognized Opcode] or
		 *       [Undetermined]
		 */
		if (request == CEC_TRANSMIT && msg->timeout > 0 &&
		    cec_msg_initiator(msg) != CEC_LOG_ADDR_UNREGISTERED &&
		    cec_msg_destination(msg) != CEC_LOG_ADDR_BROADCAST &&
		    (msg->tx_status & CEC_TX_STATUS_OK) &&
		    (msg->rx_status & CEC_RX_STATUS_OK)) {
			if (cec_msg_status_is_abort(msg) &&
			    (abort_reason(msg) == CEC_OP_ABORT_UNRECOGNIZED_OP ||
			     abort_reason(msg) == CEC_OP_ABORT_UNDETERMINED))
				node->remote[la].unrecognized_op[opcode] = true;
			else
				node->remote[la].recognized_op[opcode] = true;
		}

		if (request == CEC_RECEIVE &&
		    cec_msg_initiator(msg) != CEC_LOG_ADDR_UNREGISTERED &&
		    cec_msg_opcode(msg) == CEC_MSG_FEATURE_ABORT) {
			__u8 abort_msg = msg->msg[2];

			if (abort_reason(msg) == CEC_OP_ABORT_UNRECOGNIZED_OP ||
			    abort_reason(msg) == CEC_OP_ABORT_UNDETERMINED)
				node->remote[la].unrecognized_op[abort_msg] = true;
			else
				node->remote[la].recognized_op[abort_msg] = true;
		}
	}

	return retval == -1 ? e : (retval ? -1 : 0);
}

const char *result_name(int res, bool show_colors)
{
	switch (res) {
	case OK_NOT_SUPPORTED:
		return show_colors ? COLOR_GREEN("OK") " (Not Supported)" : "OK (Not Supported)";
	case OK_PRESUMED:
		return show_colors ? COLOR_GREEN("OK") " (Presumed)" : "OK (Presumed)";
	case OK_REFUSED:
		return show_colors ? COLOR_GREEN("OK") " (Refused)" : "OK (Refused)";
	case OK_UNEXPECTED:
		return show_colors ? COLOR_GREEN("OK") " (Unexpected)" : "OK (Unexpected)";
	case OK_EXPECTED_FAIL:
		return show_colors ? COLOR_GREEN("OK") " (Expected Failure)" : "OK (Expected Failure)";
	case OK:
		return show_colors ? COLOR_GREEN("OK") : "OK";
	default:
		return show_colors ? COLOR_RED("FAIL") : "FAIL";
	}
}

const char *ok(int res)
{
	const char *res_name = result_name(res, show_colors);

	switch (res) {
	case OK_NOT_SUPPORTED:
	case OK_PRESUMED:
	case OK_REFUSED:
	case OK_UNEXPECTED:
	case OK_EXPECTED_FAIL:
	case OK:
		res = OK;
		break;
	default:
		break;
	}
	tests_total++;
	if (res)
		app_result = res;
	else
		tests_ok++;
	return res_name;
}

int check_0(const void *p, int len)
{
	const __u8 *q = static_cast<const __u8 *>(p);

	while (len--)
		if (*q++)
			return 1;
	return 0;
}

#define TX_WAIT_FOR_HPD		10
#define TX_WAIT_FOR_HPD_RETURN	30

static bool wait_for_hpd(struct node *node, bool send_image_view_on)
{
	int fd = node->fd;
	int flags = fcntl(node->fd, F_GETFL);
	time_t t = time(nullptr);

	fcntl(node->fd, F_SETFL, flags | O_NONBLOCK);
	for (;;) {
		struct timeval tv = { 1, 0 };
		fd_set ex_fds;
		int res;

		FD_ZERO(&ex_fds);
		FD_SET(fd, &ex_fds);
		res = select(fd + 1, nullptr, nullptr, &ex_fds, &tv);
		if (res < 0) {
			fail("select failed with error %d\n", errno);
			return false;
		}
		if (FD_ISSET(fd, &ex_fds)) {
			struct cec_event ev;

			res = doioctl(node, CEC_DQEVENT, &ev);
			if (!res && ev.event == CEC_EVENT_STATE_CHANGE &&
			    ev.state_change.log_addr_mask)
				break;
		}

		if (send_image_view_on && time(nullptr) - t > TX_WAIT_FOR_HPD) {
			struct cec_msg image_view_on_msg;

			// So the HPD is gone (possibly due to a standby), but
			// some TVs still have a working CEC bus, so send Image
			// View On to attempt to wake it up again.
			cec_msg_init(&image_view_on_msg, CEC_LOG_ADDR_UNREGISTERED,
				     CEC_LOG_ADDR_TV);
			cec_msg_image_view_on(&image_view_on_msg);
			doioctl(node, CEC_TRANSMIT, &image_view_on_msg);
			send_image_view_on = false;
		}

		if (time(nullptr) - t > TX_WAIT_FOR_HPD + TX_WAIT_FOR_HPD_RETURN) {
			fail("timed out after %d s waiting for HPD to return\n",
			     TX_WAIT_FOR_HPD + TX_WAIT_FOR_HPD_RETURN);
			return false;
		}
	}
	fcntl(node->fd, F_SETFL, flags);
	return true;
}

bool transmit_timeout(struct node *node, struct cec_msg *msg, unsigned timeout)
{
	struct cec_msg original_msg = *msg;
	bool retried = false;
	int res;

	msg->timeout = timeout;
retry:
	res = doioctl(node, CEC_TRANSMIT, msg);
	if (res == ENODEV) {
		printf("Device was disconnected.\n");
		std::exit(EXIT_FAILURE);
	}
	if (res == ENONET) {
		if (retried) {
			fail("HPD was lost twice, that can't be right\n");
			return false;
		}
		warn("HPD was lost, wait for it to come up again.\n");

		if (!wait_for_hpd(node, !(node->caps & CEC_CAP_NEEDS_HPD) &&
				  cec_msg_destination(msg) == CEC_LOG_ADDR_TV))
			return false;

		retried = true;
		goto retry;
	}

	if (res || !(msg->tx_status & CEC_TX_STATUS_OK))
		return false;

	if (((msg->rx_status & CEC_RX_STATUS_OK) || (msg->rx_status & CEC_RX_STATUS_FEATURE_ABORT))
	    && response_time_ms(msg) > reply_threshold)
		warn("Waited %4ums for %s to msg %s.\n",
		     response_time_ms(msg),
		     (msg->rx_status & CEC_RX_STATUS_OK) ? "reply" : "Feature Abort",
		     opcode2s(&original_msg).c_str());

	if (!cec_msg_status_is_abort(msg))
		return true;

	if (cec_msg_is_broadcast(&original_msg)) {
		fail("Received Feature Abort in reply to broadcast message\n");
		return false;
	}

	const char *reason;

	switch (abort_reason(msg)) {
	case CEC_OP_ABORT_UNRECOGNIZED_OP:
	case CEC_OP_ABORT_UNDETERMINED:
		return true;
	case CEC_OP_ABORT_INVALID_OP:
		reason = "Invalid operand";
		break;
	case CEC_OP_ABORT_NO_SOURCE:
		reason = "Cannot provide source";
		break;
	case CEC_OP_ABORT_REFUSED:
		reason = "Refused";
		break;
	case CEC_OP_ABORT_INCORRECT_MODE:
		reason = "Incorrect mode";
		break;
	default:
		reason = "Unknown";
		fail_once("Unknown Feature Abort reason (0x%02x)\n", abort_reason(msg));
		break;
	}
	info("Opcode %s was replied to with Feature Abort [%s]\n",
	     opcode2s(&original_msg).c_str(), reason);

	return true;
}

int util_receive(struct node *node, unsigned la, unsigned timeout,
		 struct cec_msg *msg, __u8 sent_msg, __u8 reply1, __u8 reply2)
{
	unsigned ts_start = get_ts_ms();

	while (get_ts_ms() - ts_start < timeout) {
		memset(msg, 0, sizeof(*msg));
		msg->timeout = 20;
		if (doioctl(node, CEC_RECEIVE, msg))
			continue;
		if (cec_msg_initiator(msg) != la)
			continue;

		if (msg->msg[1] == CEC_MSG_FEATURE_ABORT) {
			__u8 reason, abort_msg;

			cec_ops_feature_abort(msg, &abort_msg, &reason);
			if (abort_msg != sent_msg)
				continue;
			return 0;
		}

		if (msg->msg[1] == reply1 || (reply2 && msg->msg[1] == reply2))
			return msg->msg[1];
	}

	return -1;
}

static int poll_remote_devs(struct node *node)
{
	unsigned retries = 0;

	node->remote_la_mask = 0;
	if (!(node->caps & CEC_CAP_TRANSMIT))
		return 0;

	for (unsigned i = 0; i < 15; i++) {
		struct cec_msg msg;

		cec_msg_init(&msg, node->log_addr[0], i);
		fail_on_test(doioctl(node, CEC_TRANSMIT, &msg));

		if (msg.tx_status & CEC_TX_STATUS_OK) {
			node->remote_la_mask |= 1 << i;
			retries = 0;
		} else if (msg.tx_status & CEC_TX_STATUS_NACK) {
			retries = 0;
		} else {
			if (!(msg.tx_status & CEC_TX_STATUS_ARB_LOST))
				warn("retry poll due to unexpected status: %s\n",
				     cec_status2s(msg).c_str());
			retries++;
			fail_on_test(retries > 10);
			i--;
		}
	}
	return 0;
}

static void topology_probe_device(struct node *node, unsigned i, unsigned la)
{
	struct cec_msg msg;
	bool unknown;

	printf("\tSystem Information for device %d (%s) from device %d (%s):\n",
	       i, cec_la2s(i), la, cec_la2s(la));

	cec_msg_init(&msg, la, i);
	cec_msg_get_cec_version(&msg, true);
	unknown = !transmit_timeout(node, &msg) || timed_out_or_abort(&msg);
	printf("\t\tCEC Version                : ");
	if (unknown) {
		printf("%s\n", cec_status2s(msg).c_str());
		node->remote[i].cec_version = CEC_OP_CEC_VERSION_1_4;
	}
	/* This needs to be kept in sync with newer CEC versions */
	else {
		node->remote[i].cec_version = msg.msg[2];
		if (msg.msg[2] < CEC_OP_CEC_VERSION_1_3A) {
			printf("< 1.3a (%x)\n", msg.msg[2]);
			warn("The reported CEC version is less than 1.3a. The device will be tested as a CEC 1.3a compliant device.\n");
		}
		else if (msg.msg[2] > CEC_OP_CEC_VERSION_2_0) {
			printf("> 2.0 (%x)\n", msg.msg[2]);
			warn("The reported CEC version is greater than 2.0. The device will be tested as a CEC 2.0 compliant device.\n");
		}
		else
			printf("%s\n", cec_version2s(msg.msg[2]));
	}

	cec_msg_init(&msg, la, i);
	cec_msg_give_physical_addr(&msg, true);
	unknown = !transmit_timeout(node, &msg) || timed_out_or_abort(&msg);
	printf("\t\tPhysical Address           : ");
	if (unknown) {
		printf("%s\n", cec_status2s(msg).c_str());
		node->remote[i].phys_addr = CEC_PHYS_ADDR_INVALID;
	}
	else {
		node->remote[i].phys_addr = (msg.msg[2] << 8) | msg.msg[3];
		printf("%x.%x.%x.%x\n",
		       cec_phys_addr_exp(node->remote[i].phys_addr));
		node->remote[i].prim_type = msg.msg[4];
		printf("\t\tPrimary Device Type        : %s\n",
		       cec_prim_type2s(node->remote[i].prim_type));
	}

	cec_msg_init(&msg, la, i);
	cec_msg_give_device_vendor_id(&msg, true);
	unknown = !transmit_timeout(node, &msg) || timed_out_or_abort(&msg);
	printf("\t\tVendor ID                  : ");
	if (unknown) {
		printf("%s\n", cec_status2s(msg).c_str());
		node->remote[i].vendor_id = CEC_VENDOR_ID_NONE;
	} else {
		__u32 vendor_id = (msg.msg[2] << 16) | (msg.msg[3] << 8) | msg.msg[4];
		node->remote[i].vendor_id = vendor_id;

		const char *vendor = cec_vendor2s(vendor_id);

		if (vendor)
			printf("0x%06x (%s)\n", node->remote[i].vendor_id, vendor);
		else
			printf("0x%06x, %u\n", vendor_id, vendor_id);
	}

	cec_msg_init(&msg, la, i);
	cec_msg_give_osd_name(&msg, true);
	unknown = !transmit_timeout(node, &msg) || timed_out_or_abort(&msg);
	printf("\t\tOSD Name                   : ");
	if (unknown) {
		printf("%s\n", cec_status2s(msg).c_str());
	} else {
		cec_ops_set_osd_name(&msg, node->remote[i].osd_name);
		printf("'%s'\n", node->remote[i].osd_name);
	}

	cec_msg_init(&msg, la, i);
	cec_msg_get_menu_language(&msg, true);
	if (transmit_timeout(node, &msg) && !timed_out_or_abort(&msg)) {
		cec_ops_set_menu_language(&msg, node->remote[i].language);
		printf("\t\tMenu Language              : %s\n",
		       node->remote[i].language);
	}

	cec_msg_init(&msg, la, i);
	cec_msg_give_device_power_status(&msg, true);
	unknown = !transmit_timeout(node, &msg) || timed_out_or_abort(&msg);
	printf("\t\tPower Status               : ");
	if (unknown) {
		printf("%s\n", cec_status2s(msg).c_str());
	} else {
		__u8 pwr;

		cec_ops_report_power_status(&msg, &pwr);
		if (pwr >= 4)
			printf("Invalid\n");
		else {
			node->remote[i].has_power_status = true;
			node->remote[i].in_standby = pwr != CEC_OP_POWER_STATUS_ON;
			printf("%s\n", power_status2s(pwr));
		}
	}

	if (node->remote[i].cec_version < CEC_OP_CEC_VERSION_2_0)
		return;
	cec_msg_init(&msg, la, i);
	cec_msg_give_features(&msg, true);
	if (transmit_timeout(node, &msg) && !timed_out_or_abort(&msg)) {
		/* RC Profile and Device Features are assumed to be 1 byte. As of CEC 2.0 only
		   1 byte is used, but this might be extended in future versions. */
		__u8 cec_version, all_device_types;
		const __u8 *rc_profile = nullptr, *dev_features = nullptr;

		cec_ops_report_features(&msg, &cec_version, &all_device_types,
					&rc_profile, &dev_features);
		if (rc_profile == nullptr || dev_features == nullptr)
			return;
		node->remote[i].rc_profile = *rc_profile;
		node->remote[i].dev_features = *dev_features;
		node->remote[i].all_device_types = all_device_types;
		node->remote[i].source_has_arc_rx =
			(*dev_features & CEC_OP_FEAT_DEV_SOURCE_HAS_ARC_RX) != 0;
		node->remote[i].sink_has_arc_tx =
			(*dev_features & CEC_OP_FEAT_DEV_SINK_HAS_ARC_TX) != 0;
		node->remote[i].has_aud_rate =
			(*dev_features & CEC_OP_FEAT_DEV_HAS_SET_AUDIO_RATE) != 0;
		node->remote[i].has_deck_ctl =
			(*dev_features & CEC_OP_FEAT_DEV_HAS_DECK_CONTROL) != 0;
		node->remote[i].has_rec_tv =
			(*dev_features & CEC_OP_FEAT_DEV_HAS_RECORD_TV_SCREEN) != 0;
	}
}

int main(int argc, char **argv)
{
	std::string device;
	const char *driver = nullptr;
	const char *adapter = nullptr;
	char short_options[26 * 2 * 3 + 1];
	int remote_la = -1;
	bool test_remote = false;
	unsigned test_tags = 0;
	int idx = 0;
	int fd = -1;
	int ch;
	int i;
	const char *env_media_apps_color = getenv("MEDIA_APPS_COLOR");

	srandom(time(nullptr));
	if (!env_media_apps_color || !strcmp(env_media_apps_color, "auto"))
		show_colors = isatty(STDOUT_FILENO);
	else if (!strcmp(env_media_apps_color, "always"))
		show_colors = true;
	else if (!strcmp(env_media_apps_color, "never"))
		show_colors = false;
	else {
		fprintf(stderr,
			"cec-compliance: invalid value for MEDIA_APPS_COLOR environment variable\n");
	}

	collectTests();

	for (i = 0; long_options[i].name; i++) {
		if (!isalpha(long_options[i].val))
			continue;
		short_options[idx++] = long_options[i].val;
		if (long_options[i].has_arg == required_argument) {
			short_options[idx++] = ':';
		} else if (long_options[i].has_arg == optional_argument) {
			short_options[idx++] = ':';
			short_options[idx++] = ':';
		}
	}
	while (true) {
		int option_index = 0;

		short_options[idx] = 0;
		ch = getopt_long(argc, argv, short_options,
				 long_options, &option_index);
		if (ch == -1)
			break;

		options[ch] = 1;
		if (!option_index) {
			for (i = 0; long_options[i].val; i++) {
				if (long_options[i].val == ch) {
					option_index = i;
					break;
				}
			}
		}
		if (long_options[option_index].has_arg == optional_argument &&
		    !optarg && argv[optind] && argv[optind][0] != '-')
			optarg = argv[optind++];

		switch (ch) {
		case OptHelp:
			usage();
			return 0;
		case OptSetDevice:
			device = optarg;
			if (device[0] >= '0' && device[0] <= '9' && device.length() <= 3) {
				static char newdev[20];

				sprintf(newdev, "/dev/cec%s", optarg);
				device = newdev;
			}
			break;
		case OptSetDriver:
			driver = optarg;
			break;
		case OptSetAdapter:
			adapter = optarg;
			break;
		case OptReplyThreshold:
			reply_threshold = strtoul(optarg, nullptr, 0);
			break;
		case OptTimeout:
			long_timeout = strtoul(optarg, nullptr, 0);
			break;
		case OptColor:
			if (!strcmp(optarg, "always"))
				show_colors = true;
			else if (!strcmp(optarg, "never"))
				show_colors = false;
			else if (!strcmp(optarg, "auto"))
				show_colors = isatty(STDOUT_FILENO);
			else {
				usage();
				std::exit(EXIT_FAILURE);
			}
			break;
		case OptNoWarnings:
			show_warnings = false;
			break;
		case OptExitOnFail:
			exit_on_fail = true;
			break;
		case OptExitOnWarn:
			exit_on_warn = true;
			break;
		case OptExpect:
		case OptExpectWithNoWarnings:
			if (setExpectedResult(optarg, ch == OptExpectWithNoWarnings)) {
				fprintf(stderr, "invalid -e argument %s\n", optarg);
				usage();
				return 1;
			}
			break;
		case OptRemote:
			if (optarg) {
				remote_la = strtoul(optarg, nullptr, 0);
				if (remote_la < 0 || remote_la > 15) {
					fprintf(stderr, "--test: invalid remote logical address\n");
					usage();
					return 1;
				}
			}
			test_remote = true;
			break;
		case OptWallClock:
			break;
		case OptVerbose:
			show_info = true;
			break;
		case OptVersion:
			printf("cec-compliance %s%s\n",
			       PACKAGE_VERSION, STRING(GIT_COMMIT_CNT));
			if (strlen(STRING(GIT_SHA)))
				printf("cec-compliance SHA: %s %s\n",
				       STRING(GIT_SHA), STRING(GIT_COMMIT_DATE));
			std::exit(EXIT_SUCCESS);
		case ':':
			fprintf(stderr, "Option '%s' requires a value\n",
				argv[optind]);
			usage();
			return 1;
		case '?':
			if (argv[optind])
				fprintf(stderr, "Unknown argument '%s'\n", argv[optind]);
			usage();
			return 1;
		}
	}
	if (optind < argc) {
		printf("unknown arguments: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);
		printf("\n");
		usage();
		return 1;
	}

	if (device.empty() && (driver || adapter)) {
		device = cec_device_find(driver, adapter);
		if (device.empty()) {
			fprintf(stderr,
				"Could not find a CEC device for the given driver/adapter combination\n");
			std::exit(EXIT_FAILURE);
		}
	}
	if (device.empty())
		device = "/dev/cec0";

	if ((fd = open(device.c_str(), O_RDWR)) < 0) {
		fprintf(stderr, "Failed to open %s: %s\n", device.c_str(),
			strerror(errno));
		std::exit(EXIT_FAILURE);
	}

	struct node node = { };
	struct cec_caps caps = { };

	node.fd = fd;
	node.device = device.c_str();
	doioctl(&node, CEC_ADAP_G_CAPS, &caps);
	node.caps = caps.capabilities;
	node.msg_fl_mask = CEC_MSG_FL_REPLY_TO_FOLLOWERS | CEC_MSG_FL_RAW;
	if (node.caps & CEC_CAP_REPLY_VENDOR_ID)
		node.msg_fl_mask |= CEC_MSG_FL_REPLY_VENDOR_ID;
	node.available_log_addrs = caps.available_log_addrs;
	node.is_vivid = !strcmp(caps.driver, "vivid");

	if (options[OptTestAudioRateControl])
		test_tags |= TAG_AUDIO_RATE_CONTROL;
	if (options[OptTestARCControl])
		test_tags |= TAG_ARC_CONTROL;
	if (options[OptTestCapDiscoveryControl])
		test_tags |= TAG_CAP_DISCOVERY_CONTROL;
	if (options[OptTestDeckControl])
		test_tags |= TAG_DECK_CONTROL;
	if (options[OptTestDeviceMenuControl])
		test_tags |= TAG_DEVICE_MENU_CONTROL;
	if (options[OptTestDeviceOSDTransfer])
		test_tags |= TAG_DEVICE_OSD_TRANSFER;
	if (options[OptTestDynamicAutoLipsync])
		test_tags |= TAG_DYNAMIC_AUTO_LIPSYNC;
	if (options[OptTestOSDDisplay])
		test_tags |= TAG_OSD_DISPLAY;
	if (options[OptTestOneTouchPlay])
		test_tags |= TAG_ONE_TOUCH_PLAY;
	if (options[OptTestOneTouchRecord])
		test_tags |= TAG_ONE_TOUCH_RECORD;
	if (options[OptTestPowerStatus])
		test_tags |= TAG_POWER_STATUS;
	if (options[OptTestRemoteControlPassthrough])
		test_tags |= TAG_REMOTE_CONTROL_PASSTHROUGH;
	if (options[OptTestRoutingControl])
		test_tags |= TAG_ROUTING_CONTROL;
	if (options[OptTestSystemAudioControl])
		test_tags |= TAG_SYSTEM_AUDIO_CONTROL;
	if (options[OptTestSystemInformation])
		test_tags |= TAG_SYSTEM_INFORMATION;
	if (options[OptTestTimerProgramming])
		test_tags |= TAG_TIMER_PROGRAMMING;
	if (options[OptTestTunerControl])
		test_tags |= TAG_TUNER_CONTROL;
	if (options[OptTestVendorSpecificCommands])
		test_tags |= TAG_VENDOR_SPECIFIC_COMMANDS;
	/* When code is added to the Standby/Resume test for waking up
	   other devices than TVs, the necessary tags should be added
	   here (probably Routing Control and/or RC Passthrough) */
	if (options[OptTestStandbyResume])
		test_tags |= TAG_POWER_STATUS | TAG_STANDBY_RESUME;

	if (!test_tags && !options[OptTestCore])
		test_tags = TAG_ALL;

	if (options[OptSkipTestAudioRateControl])
		test_tags &= ~TAG_AUDIO_RATE_CONTROL;
	if (options[OptSkipTestARCControl])
		test_tags &= ~TAG_ARC_CONTROL;
	if (options[OptSkipTestCapDiscoveryControl])
		test_tags &= ~TAG_CAP_DISCOVERY_CONTROL;
	if (options[OptSkipTestDeckControl])
		test_tags &= ~TAG_DECK_CONTROL;
	if (options[OptSkipTestDeviceMenuControl])
		test_tags &= ~TAG_DEVICE_MENU_CONTROL;
	if (options[OptSkipTestDeviceOSDTransfer])
		test_tags &= ~TAG_DEVICE_OSD_TRANSFER;
	if (options[OptSkipTestDynamicAutoLipsync])
		test_tags &= ~TAG_DYNAMIC_AUTO_LIPSYNC;
	if (options[OptSkipTestOSDDisplay])
		test_tags &= ~TAG_OSD_DISPLAY;
	if (options[OptSkipTestOneTouchPlay])
		test_tags &= ~TAG_ONE_TOUCH_PLAY;
	if (options[OptSkipTestOneTouchRecord])
		test_tags &= ~TAG_ONE_TOUCH_RECORD;
	if (options[OptSkipTestPowerStatus])
		test_tags &= ~TAG_POWER_STATUS;
	if (options[OptSkipTestRemoteControlPassthrough])
		test_tags &= ~TAG_REMOTE_CONTROL_PASSTHROUGH;
	if (options[OptSkipTestRoutingControl])
		test_tags &= ~TAG_ROUTING_CONTROL;
	if (options[OptSkipTestSystemAudioControl])
		test_tags &= ~TAG_SYSTEM_AUDIO_CONTROL;
	if (options[OptSkipTestSystemInformation])
		test_tags &= ~TAG_SYSTEM_INFORMATION;
	if (options[OptSkipTestTimerProgramming])
		test_tags &= ~TAG_TIMER_PROGRAMMING;
	if (options[OptSkipTestTunerControl])
		test_tags &= ~TAG_TUNER_CONTROL;
	if (options[OptSkipTestVendorSpecificCommands])
		test_tags &= ~TAG_VENDOR_SPECIFIC_COMMANDS;
	if (options[OptSkipTestStandbyResume])
		test_tags &= ~(TAG_POWER_STATUS | TAG_STANDBY_RESUME);

	if (options[OptInteractive])
		test_tags |= TAG_INTERACTIVE;

	if (strlen(STRING(GIT_SHA)))
		printf("cec-compliance SHA                 : %s %s\n",
		       STRING(GIT_SHA), STRING(GIT_COMMIT_DATE));

	node.phys_addr = CEC_PHYS_ADDR_INVALID;
	doioctl(&node, CEC_ADAP_G_PHYS_ADDR, &node.phys_addr);

	struct cec_log_addrs laddrs = { };
	doioctl(&node, CEC_ADAP_G_LOG_ADDRS, &laddrs);
	node.vendor_id = laddrs.vendor_id;

	if (node.phys_addr == CEC_PHYS_ADDR_INVALID &&
	    !(node.caps & (CEC_CAP_PHYS_ADDR | CEC_CAP_NEEDS_HPD)) &&
	    laddrs.num_log_addrs) {
		struct cec_msg msg;

		/*
		 * Special corner case: if PA is invalid, then you can still try
		 * to wake up a TV.
		 */
		cec_msg_init(&msg, CEC_LOG_ADDR_UNREGISTERED, CEC_LOG_ADDR_TV);

		cec_msg_image_view_on(&msg);
		fail_on_test(doioctl(&node, CEC_TRANSMIT, &msg));
		if (msg.tx_status & CEC_TX_STATUS_OK) {
			time_t cnt = 0;

			while (cnt++ <= long_timeout) {
				fail_on_test(doioctl(&node, CEC_ADAP_G_PHYS_ADDR, &node.phys_addr));
				if (node.phys_addr != CEC_PHYS_ADDR_INVALID) {
					doioctl(&node, CEC_ADAP_G_LOG_ADDRS, &laddrs);
					break;
				}
				sleep(1);
			}
		}

	}

	if (options[OptSkipInfo]) {
		printf("\n");
	} else {
		struct cec_connector_info conn_info = {};

		doioctl(&node, CEC_ADAP_G_CONNECTOR_INFO, &conn_info);

		cec_driver_info(caps, laddrs, node.phys_addr, conn_info);
	}

	if (options[OptListTests]) {
		printf("\nAvailable Tests:\n\n");
		listTests();
		printf("\n");
		printf("Possible test results:\n"
		       "\t0 = OK                  Supported correctly by the device.\n"
		       "\t1 = FAIL                Failed and was expected to be supported by this device.\n"
		       "\t2 = OK (Presumed)       Presumably supported.  Manually check to confirm.\n"
		       "\t3 = OK (Not Supported)  Not supported and not mandatory for the device.\n"
		       "\t4 = OK (Refused)        Supported by the device, but was refused.\n"
		       "\t5 = OK (Unexpected)     Supported correctly but is not expected to be supported for this device.\n");
	}

	bool missing_pa = node.phys_addr == CEC_PHYS_ADDR_INVALID && (node.caps & CEC_CAP_PHYS_ADDR);
	bool missing_la = laddrs.num_log_addrs == 0 && (node.caps & CEC_CAP_LOG_ADDRS);

	if (missing_la || missing_pa)
		printf("\n");
	if (missing_pa)
		fprintf(stderr, "FAIL: missing physical address, use cec-ctl to configure this\n");
	if (missing_la)
		fprintf(stderr, "FAIL: missing logical address(es), use cec-ctl to configure this\n");
	if (missing_la || missing_pa)
		std::exit(EXIT_FAILURE);

	if (!options[OptSkipInfo]) {
		printf("\nCompliance test for %s device %s:\n\n",
		       caps.driver, device.c_str());
		printf("    The test results mean the following:\n"
		       "        OK                    Supported correctly by the device.\n"
		       "        OK (Not Supported)    Not supported and not mandatory for the device.\n"
		       "        OK (Presumed)         Presumably supported.  Manually check to confirm.\n"
		       "        OK (Unexpected)       Supported correctly but is not expected to be supported for this device.\n"
		       "        OK (Refused)          Supported by the device, but was refused.\n"
		       "        OK (Expected Failure) Failed but this was expected (see -e option).\n"
		       "        FAIL                  Failed and was expected to be supported by this device.\n\n");
	}

	node.has_cec20 = laddrs.cec_version >= CEC_OP_CEC_VERSION_2_0;
	node.num_log_addrs = laddrs.num_log_addrs;
	memcpy(node.log_addr, laddrs.log_addr, laddrs.num_log_addrs);
	node.adap_la_mask = laddrs.log_addr_mask;
	node.current_time = time(nullptr);

	printf("Find remote devices:\n");
	printf("\tPolling: %s\n", ok(poll_remote_devs(&node)));

	if (!node.remote_la_mask) {
		printf("\nFAIL: No remote devices found, exiting.\n");
		std::exit(EXIT_FAILURE);
	}

	if (options[OptTestAdapter])
		testAdapter(node, laddrs, device.c_str());
	printf("\n");

	printf("Network topology:\n");
	for (unsigned i = 0; i < 15; i++)
		if (node.remote_la_mask & (1 << i))
			topology_probe_device(&node, i, node.log_addr[0]);
	printf("\n");

	if (options[OptTestFuzzing] && remote_la >= 0)
		std::exit(testFuzzing(node, laddrs.log_addr[0], remote_la));

	unsigned remote_la_mask = node.remote_la_mask;

	if (remote_la >= 0)
		remote_la_mask = 1 << remote_la;

	if (test_remote) {
		for (unsigned i = 0; i < node.num_log_addrs; i++) {
			unsigned from = node.log_addr[i];
			node.prim_devtype = laddrs.primary_device_type[i];

			for (unsigned to = 0; to <= 15; to++)
				if (!(node.adap_la_mask & (1 << to)) &&
				    (remote_la_mask & (1 << to)))
					testRemote(&node, from, to, test_tags,
						   options[OptInteractive], options[OptShowTimestamp]);
		}
	}

	/* Final test report */

	close(fd);

	printf("Total for %s device %s: %d, Succeeded: %d, Failed: %d, Warnings: %d\n",
	       caps.driver, device.c_str(),
	       tests_total, tests_ok, tests_total - tests_ok, warnings);
	std::exit(app_result);
}
