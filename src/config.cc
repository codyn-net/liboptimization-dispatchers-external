/*
 * config.cc
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

#include "config.hh"

using namespace std;
using namespace external;

Config *Config::s_instance = 0;

Config::Config()
:
	Secure(true)
{
	registerProperty("secure", Secure);
}

Config &
Config::Initialize(string const &filename)
{
	if (!s_instance)
	{
		s_instance = new Config();
	}
	
	s_instance->read(filename);
	return *s_instance;
}

Config &
Config::Instance()
{
	return *s_instance;
}
