#pragma once

#include "response.h"
#include "request.h"
#include "server.h"
#include "codes.h"
#include "result.h"


namespace http {
	
	class request;
	class response;
    class server;
    enum class codes;
	
	//typedef std::function< result<const response, const codes> (const request& req, response& resp) > handler; fail to compile angry check for const it seems
	typedef result<codes> handlerResult;
	typedef handlerResult handlerRes;
	typedef std::function<handlerRes(request& req, response& resp, server& serv)> handler;
	//typedef std::function< result<codes> (const request& req, response& resp) > handler;
}