#pragma once

#include "web/web_page.h"
#include "state_management.h"

#include <pistache/endpoint.h>

class PistacheServerHandler : public Pistache::Http::Handler
{
	HTTP_PROTOTYPE(PistacheServerHandler)
	void onRequest(const Pistache::Http::Request& request, Pistache::Http::ResponseWriter writer) override
	{
		if(request.resource() == "/status" || request.resource() == "/json/status")
		{
			std::string p = mandeye::produceReport();
			writer.send(Pistache::Http::Code::Ok, p);
			return;
		}
		else if(request.resource() == "/jquery.js")
		{
			writer.send(Pistache::Http::Code::Ok, gJQUERYData);
			return;
		}
		else if(request.resource() == "/trig/start_bag")
		{
			mandeye::StartScan();
			writer.send(Pistache::Http::Code::Ok, "");
			return;
		}
		else if(request.resource() == "/trig/stop_bag")
		{
			mandeye::StopScan();
			writer.send(Pistache::Http::Code::Ok, "");
			return;
		}
		else if(request.resource() == "/trig/stopscan")
		{
			mandeye::TriggerStopScan();
			writer.send(Pistache::Http::Code::Ok, "");
			return;
		}
		writer.send(Pistache::Http::Code::Ok, gINDEX_HTMData);
	}
};