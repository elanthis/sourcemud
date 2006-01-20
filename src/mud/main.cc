/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "server.h"
#include "settings.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pwd.h>
#include <grp.h>
#include <netdb.h>

#include <fstream>

#include "common/error.h"
#include "common/awestr.h"
#include "common/rand.h"
#include "mud/network.h"
#include "mud/player.h"
#include "mud/settings.h"
#include "mud/weather.h"
#include "common/streams.h"
#include "mud/zone.h"
#include "mud/control.h"
#include "mud/message.h"
#include "mud/telnet.h"
#include "mud/account.h"
#include "common/log.h"
#include "mud/clock.h"
#include "mud/parse.h"
#include "mud/bindings.h"
#include "mud/hooks.h"

// DECLARATIONS
namespace {
	class TelnetListener : public SocketListener
	{
		public:
		inline TelnetListener (int s_sock) : SocketListener(s_sock) {}

		virtual void in_ready ();
	};

	class ControlListener : public SocketListener
	{
		public:
		inline ControlListener (int s_sock) : SocketListener(s_sock) {}

		virtual void in_ready ();
	};

	void cleanup ();
	int write_pid_file (StringArg path);
	double count_ticks (timeval& start, timeval& cur);
}

// GLOBALS
namespace {
	// time
	struct timeval last_time;
	unsigned long int game_ticks;
	time_t start_time;

	// are we running still?
	bool running;

	// Signal flags
	volatile bool signaled_shutdown = false;
	volatile bool signaled_reload = false;
}

// FUNCTIONS
namespace {

	// termination signal handler
	void
	sigterm_handler (int)
	{
		signaled_shutdown = true;
	}

	// interrupt signal handler
	void
	sigint_handler (int)
	{
		signaled_shutdown = true;
	}

	// hangup signal handler
	void
	sighup_handler (int)
	{
		signaled_reload = true;
	}

	// write out our pid file
	int
	write_pid_file (StringArg path)
	{
		// open it up
		int fd;
		if ((fd = open(path, O_WRONLY|O_CREAT, 0640)) < 0) {
			// fatal error
			Log::Error << "Failed to create or open PID file '" << path << "': " << strerror(errno);
			return -1;
		}

		// try to lock; if it's already locked, another process
		// owns the PID lock, otherwise, it's a stale PID file and
		// we can safely over-write it.
		struct flock lockinfo;
		lockinfo.l_type = F_WRLCK;
		lockinfo.l_whence = SEEK_SET;
		lockinfo.l_start = 0;
		lockinfo.l_len = 0;
		if (fcntl(fd, F_SETLK, &lockinfo) < 0) {
			// fatal error
			Log::Error << "Failed to lock PID file '" << path << "': " << strerror(errno);
			Log::Error << "AweMUD appears to already be running.";
			return -1;
		}

		// truncate file
		if (ftruncate(fd, 0) < 0) {
			Log::Error << "Failed to truncate PID file '" << path << "': " << strerror(errno);
			unlink(path);
			close(fd);
			return -1;
		}

		// write PID
		String pidstr;
		StreamControl(pidstr) << getpid() << "\n";
		if (write(fd, pidstr.c_str(), pidstr.size()) != (int)pidstr.size()) {
			Log::Error << "Failed to write to PID file '" << path << "': " << strerror(errno);
			unlink(path);
			close(fd);
			return -1;
		}

		// all good!
		return 0;
	}

	// in a very unsafe fashion, dump text to a socket
	// FIXME: yuck, this is ugly - but how else to do this
	// without shitloads of infrastructure?
	void
	dump_to_sock (int sock, const char* text)
	{
		send(sock, text, strlen(text), 0);
	}

	void
	cleanup ()
	{
		// remember paths
		String pid_path = SettingsManager.get_pid_file();
		String ctrl_path = SettingsManager.get_control_sock();

		// remove files
		if (pid_path)
			unlink (pid_path);
		if (ctrl_path)
			unlink (ctrl_path);

		// cleanup
		GC_gcollect();
	}

	inline void
	timeval_add_ms (struct timeval& tv, long msecs)
	{
		tv.tv_sec += msecs / 1000;
		tv.tv_usec += (msecs % 1000) * 1000;
		if (tv.tv_usec >= 1000000) {
			tv.tv_usec -= 1000000;
			tv.tv_sec ++;
		}
	}

	inline long
	ms_until_timeval (struct timeval& tv)
	{
		struct timeval now;
		gettimeofday(&now, NULL);

		if (now.tv_sec > tv.tv_sec)
			return 0;
		else if (now.tv_sec == tv.tv_sec && now.tv_usec >= tv.tv_usec)
			return 0;
		else {
			long msecs = (tv.tv_sec - now.tv_sec) * 1000;
			msecs += (tv.tv_usec - now.tv_usec) / 1000;
			return msecs + 2;
			// we add two milliseconds - one to get past rounding
			// errors here, and one to get past rounding errors in the
			// syscalls that use the value
		}
	}

	void
	TelnetListener::in_ready ()
	{
		// accept client
		SockStorage addr;
		int client = Network::accept_tcp(sock, addr);
		if (client == -1) {
			Log::Error << "accept() failed: " << strerror(errno);
			return;
		}

		// deny blocked hosts
		if (NetworkManager.denies.exists(addr)) {
			dump_to_sock(client, "Your host or network has been banned from this server.\r\n");
			Log::Network << "Telnet client rejected: " << Network::get_addr_name(addr) << ": host or network banned";
			close(client);
			return;
		}

		// manage connection count
		int err = NetworkManager.connections.add(addr);
		if (err == -1) {
			dump_to_sock(client, "Too many users connected.\r\n");
			Log::Network << "Telnet client rejected: " << Network::get_addr_name(addr) << ": too many users";
			close(client);
			return;
		}
		if (err == -2) {
			dump_to_sock(client, "Too many users connected from your host.\r\n");
			Log::Network << "Telnet client rejected: " << Network::get_addr_name(addr) << ": too many users from client host";
			close(client);
			return;
		}

		// log connection
		Log::Network << "Telnet client connected: " << Network::get_addr_name(addr);
		
		// create a new telnet handler
		TelnetHandler *telnet = new TelnetHandler(client, addr);
		if (telnet == NULL) {
			dump_to_sock(client, "Internal server error.\r\n");
			Log::Error << "TelnetHandler() failed, closing connection.";
			close(client);
			NetworkManager.connections.remove(addr);
			return;
		}

		// add to poll manager
		if (NetworkManager.add_socket(telnet)) {
			dump_to_sock(client, "Internal server error.\r\n");
			Log::Error << "PollSystem::add_socket() failed, closing connection.";
			close(client);
			NetworkManager.connections.remove(addr);
			return;
		}

		// banner
		telnet->clear_scr();
		*telnet <<
			"\n ----===[ AweMUD V" VERSION " ]===----\n\n"
			"AweMUD Copyright (C) 2000-2005  AwesomePlay Productions, Inc.\n"
			"Visit http://www.awemud.net for more details.\n";

		// connect message
		*telnet << StreamParse(MessageManager.get("connect"));

		// init login
		telnet->set_mode(new TelnetModeLogin(telnet));
	}

	void
	ControlListener::in_ready ()
	{
		// accept client
		uid_t uid;
		int client = Network::accept_unix(sock, uid);
		if (client == -1) {
			Log::Error << "accept() failed: " << strerror(errno);
			return;
		}

		// deny blocked users
		if (!ControlManager.has_user_access(uid)) {
			dump_to_sock(client, "+NOACCESS Access Denied\n");
			Log::Network << "Control interface client rejected: " << uid << ": user not allowed";
			close(client);
			return;
		}

		// log connection
		Log::Network << "Control interface client connected: " << uid;

		// create a new connection
		ControlHandler* conn = new ControlHandler(client, uid);
		if (conn == NULL) {
			dump_to_sock(client, "+INTERNAL Internal server error.\n");
			Log::Warning << "ControlHandler() failed, closing connection.";
			close(client);
			return;
		}

		// add to poll manager
		if (NetworkManager.add_socket(conn)) {
			dump_to_sock(client, "Internal server error.\r\n");
			Log::Error << "PollSystem::add_socket() failed, closing connection.";
			close(client);
			conn = NULL;
			return;
		}

		// banner
		dump_to_sock(client, "+OK Welcome\n");
	}
}

void
AweMUD::shutdown ()
{
	Log::Info << "Shutting down server";
	running = false;
}

ulong
AweMUD::get_ticks ()
{
	return game_ticks;
}

ulong
AweMUD::get_rounds ()
{
	return TICKS_TO_ROUNDS(game_ticks);
}

String
AweMUD::get_uptime ()
{
	BufferSink<512> uptime;
	uint secs = (::time (NULL) - start_time); 
	uint seconds = secs % 60;
	secs /= 60;
	uint minutes = secs % 60;
	secs /= 60;
	uint hours = secs % 24;
	secs /= 24;
	uint days = secs;

	if (days != 1)
		uptime << days << " days, ";
	else
		uptime << "1 day, ";

	if (hours != 1)
		uptime << hours << " hours, ";
	else
		uptime << "1 hour, ";

	if (minutes != 1)
		uptime << minutes << " minutes, ";
	else
		uptime << "1 minute, ";

	if (seconds != 1)
		uptime << seconds << " seconds";
	else
		uptime << "1 second";

	return uptime.str();
}

int
main (int argc, char **argv)
{
	// print out information
	printf("AweMUD server V" VERSION "\n"
		"Copyright (C) 2000-2005, AwesomePlay Productions, Inc.\n"
		"AweMUD comes with ABSOLUTELY NO WARRANTY; see COPYING for details.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; for details, see the file COPYING.\n");

#if (defined(sparc) || defined(__sparc)) && defined(sun)
	// do nothing; GC_INIT() fails in C++ mode and is only necessary
	// for older Solaris environment, which we aren't going to support
	// anyhow.
#else
	// initialize GC
	GC_INIT();
#endif

	// load settings
	if (IManager::require(SettingsManager))
		return 1;
	if (SettingsManager.parse_argv(argc, argv))
		return 1;
	if (SettingsManager.get_config_file() && SettingsManager.load_file(SettingsManager.get_config_file()))
		return 1;

	// change to chroot dir, but don't actually chroot yet
	if (SettingsManager.get_chroot()) {
		if (chdir(SettingsManager.get_chroot())) {
			Log::Error << "chroot() failed: " << SettingsManager.get_chroot() << ": " << strerror(errno);
			return 1;
		}
	}

	// logging
	if (IManager::require(LogManager))
		return 1;

	// fork daemon
	if (SettingsManager.get_daemon()) {
		if (fork ())
			_exit (0);

		// close controlling TTY
#ifdef TIOCNOTTY
		int ttyfd = open ("/dev/tty", O_RDWR);
		ioctl (ttyfd, TIOCNOTTY);
		close (ttyfd);
#endif

		// new process group
		setpgid (0, 0);

		// we don't need these at all, don't want them
		close (0); // STDIN
		close (1); // STDOUT
		close (2); // STDERR

		// legacy code/crap, /dev/null as our stdin/stdout/stderr - yay...
		int base_fd = open ("/dev/null", O_RDWR); // stdin
		dup (base_fd); // stdout
		dup (base_fd); // stderr

		Log::Info << "Forked daemon";
	}

	// set time
	::time (&start_time);

	// write PID
	String pid_path = SettingsManager.get_pid_file();
	if (write_pid_file(pid_path))
		return 1;
	Log::Info << "Wrote PID file '" << pid_path << "'";

	// read group/user info
	struct group *grp = NULL;
	String group_name = SettingsManager.get_group();
	if (group_name && !str_is_number (group_name)) {
		if (str_is_number (group_name))
			grp = getgrgid (tolong(group_name));
		else
			grp = getgrnam (group_name);
		if (grp == NULL) {
			Log::Error << "Couldn't find group '" << group_name << "': " << strerror(errno);
			return 1;
		}
	}
	struct passwd *usr = NULL;
	String user_name = SettingsManager.get_user();
	if (user_name) {
		if (str_is_number (user_name))
			usr = getpwuid (tolong(user_name));
		else
			usr = getpwnam (user_name);
		if (usr == NULL) {
			Log::Error << "Couldn't find user '" << user_name << "': " << strerror(errno);
			return 1;
		}
	}

	// do chroot jail
	String chroot_dir = SettingsManager.get_chroot();
	if (chroot_dir) {
		if (chroot (chroot_dir)) {
			Log::Error << "chroot() failed: " << strerror (errno);
			return 1;
		}
		chdir ("/"); // activate chroot
		Log::Info << "Chrooted to '" << chroot_dir << "'";
	}

	// run all the cleanup stuff no matter how we exit (except crash)
	atexit(cleanup);

	// seting up signal handlers
	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = sigterm_handler;
	if (sigaction (SIGTERM, &sa, NULL)) {
		Log::Error << "sigaction() failed (SIGTERM)";
		return 1;
	}
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = sigint_handler;
	if (sigaction (SIGINT, &sa, NULL)) {
		Log::Error << "sigaction() failed (SIGINT)";
		return 1;
	}
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = SIG_IGN;
	if (sigaction (SIGPIPE, &sa, NULL)) {
		Log::Error << "sigaction() failed (SIGPIPE)";
		return 1;
	}
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = sighup_handler;
	if (sigaction (SIGHUP, &sa, NULL)) {
		Log::Error << "sigaction() failed (SIGHUP)";
		return 1;
	}

	init_random (); // random info

	// load all managers
	if (IManager::initialize_all())
		return 1;

	// listen sockets
	int player_ipv4 = -1;
	int player_ipv6 = -1;
	int control_unix = -1;

	// control interface
	String ctrl_path = SettingsManager.get_control_sock();
	if (ctrl_path) {
		control_unix = Network::listen_unix(ctrl_path);
		if (control_unix == -1)
			return 1;
		Log::Info << "Control interface active on " << ctrl_path;
	}

	// get port
	int accept_port = SettingsManager.get_port();

	// IPv6 message
#ifdef HAVE_IPV6
	if (SettingsManager.get_ipv6()) {
		player_ipv6 = Network::listen_tcp(accept_port, AF_INET6);
		if (player_ipv6 == -1)
			return 1;
		Log::Info << "IPv6 enabled";
	}
#endif // HAVE_IPV6

	// network server
	player_ipv4 = Network::listen_tcp(accept_port, AF_INET);
	if (player_ipv4 == -1)
		return 1;
	Log::Info << "Listening on port " << accept_port;

	// change user/group
	if (grp) {
		if (setregid (grp->gr_gid, grp->gr_gid)) {
			Log::Error << "Couldn't set group to '" << grp->gr_name << "': " << strerror(errno);
			return 1;
		}
		Log::Info << "Set group to " << grp->gr_name << " (" << grp->gr_gid << ")";
		// drop supplemental groups
		setgroups(0, NULL);
	}
	if (usr) {
		if (setreuid (usr->pw_uid, usr->pw_uid)) {
			Log::Error << "Couldn't set user to '" << usr->pw_name << "': " << strerror(errno);
			return 1;
		}
		Log::Info << "Set user to " << usr->pw_name << " (" << usr->pw_uid << ")";
	}

	// run init hook
	Hooks::ready();

	// all set
	Log::Info << "Ready";

	// initialize time
	struct timeval nexttick;
	gettimeofday(&nexttick, NULL);
	timeval_add_ms(nexttick, 1000 / TICKS_PER_ROUND);
	ulong last_autosave = 0;
	ulong cur_ticks = 0;
	game_ticks = 0;

	// initialize listen sockets
	if (NetworkManager.add_socket(new TelnetListener(player_ipv4))) {
		Log::Error << "NetworkManager.add_socket() failed";
		return 1;
	}
	if (player_ipv6 != -1) {
		if (NetworkManager.add_socket(new TelnetListener(player_ipv6))) {
			Log::Error << "NetworkManager.add_socket() failed";
			return 1;
		}
	}
	if (control_unix != -1) {
		if (NetworkManager.add_socket(new ControlListener(control_unix))) {
			Log::Error << "NetworkManager.add_socket() failed";
			return 1;
		}
	}

	// begin main game loop - wee!
	running = true;
	while (running) {
		// poll timeout
		long timeout = -1;

		// need to run now to process data?
		if (EventManager.events_pending())
			timeout = 0;
		// behind on ticks?
		else if (cur_ticks > game_ticks)
			timeout = 0;
		// have players?  need a timeout for game updates
		else if (PlayerManager.count())
			timeout = ms_until_timeval(nexttick);

		// do select - no player, don't timeout
		NetworkManager.poll(timeout);

		// update by one tick per loop at most...
		// update timer
		struct timeval current;
		gettimeofday(&current, NULL);
		if (timercmp(&current, &nexttick, >=)) {
			++game_ticks;

			// time of next tick
			timeval_add_ms(nexttick, 1000 / TICKS_PER_ROUND);
			if (timercmp(&current, &nexttick, >))
				nexttick = current;

			// update entities
			EntityManager.heartbeat();

			// update weather
			WeatherManager.update();

			// update time
			bool is_day = TimeManager.time.is_day ();
			uint hour = TimeManager.time.get_hour ();
			TimeManager.time.update (1);

			// change from day/night
			if (is_day && !TimeManager.time.is_day ())
				if (!TimeManager.calendar.sunset_text.empty())
					ZoneManager.announce(TimeManager.calendar.sunset_text[get_random(TimeManager.calendar.sunset_text.size())], ANFL_OUTDOORS);
			else if (!is_day && TimeManager.time.is_day ())
				if (!TimeManager.calendar.sunrise_text.empty())
					ZoneManager.announce(TimeManager.calendar.sunrise_text[get_random(TimeManager.calendar.sunrise_text.size())], ANFL_OUTDOORS);

			// new hour
			if (TimeManager.time.get_hour() != hour)
				Hooks::change_hour();
		}

		// handle events
		EventManager.process();

		// do auto-save
		if ((cur_ticks - last_autosave) >= (uint)SettingsManager.get_auto_save() * TICKS_PER_ROUND * 60) {
			last_autosave = cur_ticks;
			Log::Info << "Auto-saving...";
			IManager::save_all();
		}

		// check for reload
		if (signaled_reload == true) {
			signaled_reload = false;
			Log::Info << "Server received a SIGHUP";
			IManager::save_all();
			LogManager.reset();
		}

		// check for signaled_shutdown
		if (signaled_shutdown == true) {
			signaled_shutdown = false;
			Log::Info << "Server received a terminating signal";
			AweMUD::shutdown();
		}

		// force run of GC
		// GC_gcollect();
	}

	// all done running - save the world
	IManager::save_all();

	// shutdown all managers
	IManager::shutdown_all();

	return 0;
}
