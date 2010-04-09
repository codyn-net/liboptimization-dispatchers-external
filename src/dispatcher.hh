#ifndef __EXTERNAL_DISPATCHER_H__
#define __EXTERNAL_DISPATCHER_H__

#include <optimization/dispatcher.hh>
#include <jessevdk/network/unixserver.hh>
#include <jessevdk/network/client.hh>
#include <jessevdk/os/pipe.hh>

namespace external
{
	class Dispatcher : public optimization::Dispatcher
	{
		sigc::connection d_timeout;
		Glib::Pid d_pid;

		jessevdk::base::Cloneable<jessevdk::os::FileDescriptor> d_readResponse;

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
			
			bool OnResponseData(jessevdk::os::FileDescriptor::DataArgs &args);

			std::string ResolveExternalExecutable(std::string const &path);
			bool Persistent();

			void ReadDataFrom(jessevdk::os::FileDescriptor &readFrom);
			std::map<std::string, std::string> SetupEnvironment();
			bool SetupArguments(std::vector<std::string> &argv);

			void SendTask(jessevdk::os::FileDescriptor &out);
			void SendTaskProtobuf(jessevdk::os::FileDescriptor &out);
			void SendTaskText(jessevdk::os::FileDescriptor &out);

			bool TextMode();
			bool ExtractProtobuf(jessevdk::os::FileDescriptor::DataArgs &args);
			bool ExtractText(jessevdk::os::FileDescriptor::DataArgs &args);

			std::string WorkingDirectory();
			bool PersistNumeric(std::string const &persist);
	};
}

#endif /* __EXTERNAL_DISPATCHER_H__ */
