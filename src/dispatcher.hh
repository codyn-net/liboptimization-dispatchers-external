#ifndef __EXTERNAL_DISPATCHER_H__
#define __EXTERNAL_DISPATCHER_H__

#include <optimization/dispatcher.hh>
#include <network/UnixServer/unixserver.hh>
#include <network/Client/client.hh>
#include <os/Pipe/pipe.hh>

namespace external
{
	class Dispatcher : public optimization::Dispatcher
	{
		sigc::connection d_timeout;
		Glib::Pid d_pid;

		base::Cloneable<os::FileDescriptor> d_readResponse;

		public:
			Dispatcher();
			~Dispatcher();

			/* Public functions */
			virtual bool RunTask();
			virtual void Stop();
		private:
			/* Private functions */
			void KillExternal();

			void OnExternalKilled(GPid pid, int ret);
			
			bool Launch();
			bool LaunchPersistent();

			bool OnTimeout();
			
			bool OnResponseData(os::FileDescriptor::DataArgs &args);

			std::string ResolveExternalExecutable(std::string const &path);
			bool Persistent();

			void ReadDataFrom(os::FileDescriptor &readFrom);
			std::map<std::string, std::string> SetupEnvironment();
			bool SetupArguments(std::vector<std::string> &argv);

			void SendTask(os::FileDescriptor &out);
			void SendTaskProtobuf(os::FileDescriptor &out);
			void SendTaskText(os::FileDescriptor &out);

			bool TextMode();
			bool ExtractProtobuf(os::FileDescriptor::DataArgs &args);
			bool ExtractText(os::FileDescriptor::DataArgs &args);
	};
}

#endif /* __EXTERNAL_DISPATCHER_H__ */
