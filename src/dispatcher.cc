/*
 * dispatcher.cc
 * This file is part of dispatcher-external
 *
 * Copyright (C) 2009 - Jesse van den Kieboom
 *
 * dispatcher-webots is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dispatcher-webots is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dispatcher-webots; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */


#include "dispatcher.hh"

#include <os/Environment/environment.hh>
#include <os/FileSystem/filesystem.hh>
#include <optimization/messages.hh>
#include <glibmm.h>
#include <base/String/string.hh>
#include <signal.h>
#include <sys/stat.h>
#include <pwd.h>
#include <sys/types.h>
#include "config.hh"

using namespace std;
using namespace external;
using namespace os;
using namespace network;
using namespace optimization::messages;
using namespace base;

Dispatcher::~Dispatcher()
{
	KillExternal();
}

void
Dispatcher::Stop()
{
	KillExternal();
	optimization::Dispatcher::Stop();
}

Dispatcher::Dispatcher()
:
	d_pid(0)
{
	Config::Initialize(PREFIXDIR "/libexec/liboptimization-dispatchers-1.0/external.conf");
}

void
Dispatcher::KillExternal()
{
	if (d_pid == 0)
	{
		return;
	}

	// Kill whole process group nicely first
	::kill(-(int)d_pid, SIGTERM);
	::kill(d_pid, SIGTERM);

	usleep(200000);
	
	// Then just kill it
	::kill(-(int)d_pid, SIGKILL);
	::kill(d_pid, SIGKILL);

	OnExternalKilled(d_pid, 0);
}

bool
Dispatcher::ExtractText(os::FileDescriptor::DataArgs &args)
{
	if (!String(args.data).endsWith("\n\n"))
	{
		args.buffer(args.data);
		return false;
	}

	// Parse it then
	vector<string> parts = String(args.data).split("\n");
	task::Response response;

	response.set_status(task::Response::Failed);
	task::Response::Failure &failure = *response.mutable_failure();
	failure.set_type(task::Response::Failure::Dispatcher);

	if (parts.size() == 0)
	{
		WriteResponse(response);
		return true;
	}

	if (parts[0] == "success")
	{
		response.set_status(task::Response::Success);
	}
	else
	{
		failure.set_message(String::join(vector<string>(parts.begin() + 1, parts.end()), "\n"));
		WriteResponse(response);
		return true;
	}

	for (vector<string>::iterator iter = parts.begin() + 1; iter != parts.end(); ++iter)
	{
		string stripped = String(*iter).strip();

		if (stripped == "")
		{
			continue;
		}

		stringstream s(stripped);
		string name;
		double value;

		s >> name;
		s >> value;

		task::Response::Fitness *fitness = response.add_fitness();
		fitness->set_name(name);
		fitness->set_value(value);
	}

	WriteResponse(response);
	return true;
}

bool
Dispatcher::ExtractProtobuf(os::FileDescriptor::DataArgs &args)
{
	bool ret = false;

	vector<task::Response> response;
	vector<task::Response>::iterator iter;

	optimization::Messages::Extract(args, response);

	for (iter = response.begin(); iter != response.end(); ++iter)
	{
		WriteResponse(*iter);
		ret = true;
	}

	return ret;
}

bool
Dispatcher::OnResponseData(os::FileDescriptor::DataArgs &args)
{
	bool ret;
	
	if (TextMode())
	{
		ret = ExtractText(args);
	}
	else
	{
		ret = ExtractProtobuf(args);
	}
	
	if (ret && !d_timeout)
	{
		// If we only expect one response, set a timeout just to make
		// sure we actually kill it
		d_timeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Dispatcher::OnTimeout), 2000);
	}

	return false;
}

bool
Dispatcher::OnTimeout()
{
	// Close webots
	KillExternal();
	return false;
}

void
Dispatcher::OnExternalKilled(GPid pid, int ret) 
{
	Glib::spawn_close_pid(pid);

	if (d_readResponse)
	{
		d_readResponse->onData().remove(*this, &Dispatcher::OnResponseData);
		d_readResponse->close();
	}
	
	if (d_timeout)
	{
		d_timeout.disconnect();
	}
	
	d_pid = 0;
	Main()->quit();
}

string
Dispatcher::ResolveExternalExecutable(std::string const &path)
{
	string ret = Glib::find_program_in_path(path);
	Config &config = Config::Instance();

	if (ret == "")
	{
		cerr << "Could not find external executable: " << path << endl;
		return ret;
	}
	
	if (!config.Secure)
	{
		return ret;
	}

	// System webots is fine
	if (String(ret).startsWith("/usr"))
	{
		return ret;
	}

	// Otherwise, must be owned by the user, and in his/her home directory
	struct stat buf;

	if (stat(ret.c_str(), &buf) != 0)
	{
		cerr << "External executable does not exist: "  << ret << endl;
		return "";
	}

	struct passwd *pwd = getpwuid(getuid());
	string homedir = pwd->pw_dir;

	if (buf.st_uid != getuid())
	{
		cerr << "Custom external executable is not owned by the user: " << ret << endl;
		return "";
	}
	else if (!String(ret).startsWith(homedir))
	{
		cerr << "Custom external executable is not in user home directory: " << ret << endl;
		return "";
	}
	else
	{
		return ret;
	}
}

map<string, string>
Dispatcher::SetupEnvironment()
{
	map<string, string> envp = Environment::all();
	
	string environment;
	if (Setting("environment", environment))
	{
		vector<string> vars = String(environment).split(",");
		
		for (vector<string>::iterator iter = vars.begin(); iter != vars.end(); ++iter)
		{
			vector<string> parts = String(*iter).split("=", 2);
			
			if (parts.size() == 2)
			{
				envp[parts[0]] = parts[1];
			}
			else if (parts.size() == 1)
			{
				envp[parts[0]] = "";
			}
		}
	}

	return envp;
}

bool
Dispatcher::SetupArguments(vector<string> &argv)
{
	string path;
	if (!Setting("path", path))
	{
		cerr << "Setting 'path' not set" << endl;
		return false;
	}
	
	path = ResolveExternalExecutable(path);
	
	if (path == "")
	{
		cerr << "Could not find external executable" << endl;
		return false;
	}
	
	argv.push_back(path);

	// Append arguments
	string args;
	
	if (Setting("arguments", args))
	{
		vector<string> splitted = Glib::shell_parse_argv(args);

		for (vector<string>::iterator iter = splitted.begin(); iter != splitted.end(); ++iter)
		{
			argv.push_back(*iter);
		}
	}

	return true;
}

bool
Dispatcher::RunTask()
{
	if (Persistent())
	{
		return LaunchPersistent();
	}
	else
	{
		return Launch();
	}
}

void
Dispatcher::ReadDataFrom(os::FileDescriptor &readFrom)
{
	d_readResponse = readFrom;
	d_readResponse->onData().add(*this, &Dispatcher::OnResponseData);
}

void
Dispatcher::SendTaskProtobuf(os::FileDescriptor &out)
{
	// Simply serialize and send it
	string serialized;

	if (optimization::Messages::Create(Request(), serialized))
	{
		out.write(serialized);
	}
}

void
Dispatcher::SendTaskText(os::FileDescriptor &out)
{
	optimization::messages::task::Task::Description const &dest = Request();
	stringstream s;

	for (size_t i = 0; i < dest.settings_size(); ++i)
	{
		s << "setting\t" << dest.settings(i).key() << "\t" << dest.settings(i).value() << endl;
	}

	s << endl;

	for (size_t i = 0; i < dest.parameters_size(); ++i)
	{
		optimization::messages::task::Task::Description::Parameter const &param = dest.parameters(i);

		s << "parameter\t" << param.name() << "\t" << param.value() << "\t" << param.min() << "\t" << param.max() << endl;
	}

	out.write(s.str());
}

void
Dispatcher::SendTask(os::FileDescriptor &out)
{
	string mode;

	// Check the mode in which to send it
	if (TextMode())
	{
		SendTaskText(out);
	}
	else
	{
		SendTaskProtobuf(out);
	}
}

bool
Dispatcher::LaunchPersistent()
{
	string persist;
	Setting("persistent", persist);

	// Try to connect to the persistent app
	Client client;
	size_t tried = 0;
	bool launched = false;
	
	while (!client)
	{
		client = Client::resolve<Client>(AddressInfo::Parse(persist));

		if (!client && !launched)
		{
			try
			{
				map<string, string> envp = SetupEnvironment();
				vector<string> argv;
				
				if (!SetupArguments(argv))
				{
					break;
				}

				GPid pid;
				gint sin;
				gint sout;
				gint serr;

				Glib::spawn_async_with_pipes("",
				                             argv,
				                             Environment::convert(envp),
				                             Glib::SPAWN_SEARCH_PATH,
				                             sigc::slot<void>(),
				                             &pid,
				                             &sin,
				                             &sout,
				                             &serr);

				close(sin);
				close(sout);
				close(serr);
			}
			catch (Glib::SpawnError &e)
			{
				cerr << "Error while spawning external: " << e.what() << endl;
				break;
			}

			launched = true;
		}

		if (!client)
		{
			if (++tried >= 5)
			{
				break;
			}

			usleep(200000);
		}
	}

	if (!client)
	{
		return false;
	}

	// Send task
	SendTask(client);
	ReadDataFrom(client);
}

bool
Dispatcher::Launch()
{
	map<string, string> envp = SetupEnvironment();
	vector<string> argv;
	
	if (!SetupArguments(argv))
	{
		return false;
	}

	gint sin;
	gint sout;

	try
	{
		Glib::spawn_async_with_pipes("",
		                             argv,
		                             Environment::convert(envp),
		                             Glib::SPAWN_DO_NOT_REAP_CHILD |
		                             Glib::SPAWN_SEARCH_PATH,
		                             sigc::slot<void>(),
		                             &d_pid,
		                             &sin,
		                             &sout,
		                             0);
	}
	catch (Glib::SpawnError &e)
	{
		cerr << "Error while spawning webots: " << e.what() << endl;
		return false;
	}
	
	Glib::signal_child_watch().connect(sigc::mem_fun(*this, &Dispatcher::OnExternalKilled), d_pid);

	// Read response from sout
	os::FileDescriptor fout(sout);
	ReadDataFrom(fout);

	os::FileDescriptor fin(sin);
	SendTask(fin);
	fin.close();

	return true;
}

bool
Dispatcher::Persistent()
{
	string persist;
	
	if (!Setting("persistent", persist))
	{
		return false;
	}

	return persist != "";
}

bool
Dispatcher::TextMode()
{
	string mode;
	return Setting("mode", mode) && mode == "text";
}
