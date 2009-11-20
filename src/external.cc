/*
 * external.cc
 * This file is part of dispatchers-external
 *
 * Copyright (C) 2009 - Jesse van den Kieboom
 *
 * dispatchers-external is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * dispatchers-external is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dispatchers-external; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */
 
 
#include "dispatcher.hh"

#include <signal.h>

external::Dispatcher dispatcher_external;

static void
nicely_stop(int sig)
{
	dispatcher_external.Stop();
}

int main (int argc, char const* argv[])
{
	signal(SIGTERM, nicely_stop);
	
	if (dispatcher_external.Run())
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
