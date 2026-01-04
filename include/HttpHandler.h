#pragma once

#include "HttpRequest.h"
#include "HttpResponse.h"

class HttpHandler {
public:
    static HttpResponse handle(const HttpRequest& request);
};
