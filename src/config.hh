#ifndef __EXTERNAL_CONFIG_H__
#define __EXTERNAL_CONFIG_H__

#include <jessevdk/base/config.hh>
#include <glibmm.h>

namespace external
{
	class Config : public jessevdk::base::Config
	{
		static Config *s_instance;

		public:
			bool Secure;
			Glib::ustring AllowedOwners;

			/* Constructor/destructor */
			static Config &Initialize(std::string const &filename);
			static Config &Instance();
		
			/* Public functions */
		private:
			/* Private functions */
			Config();
	};
}

#endif /* __EXTERNAL_CONFIG_H__ */
