add_library(netservices_rfc 
	HttpAuthentication.cpp
	HttpForm.cpp
	HttpHeaders.cpp
	HttpMethod.cpp
	HttpRequest.cpp
	HttpResult.cpp
	HttpSession.cpp
	NetworkRequest.cpp
	UrlProtocolRoster.cpp
	UrlRequest.cpp
	UrlResult.cpp
)

target_include_directories(netservices_rfc PUBLIC
                           "${PROJECT_SOURCE_DIR}/headers/public" 
                           "/boot/system/develop/headers/private/net"
                           "/boot/system/develop/headers/private/support"
                           )

set_target_properties(netservices_rfc PROPERTIES
	CXX_STANDARD 17
)
