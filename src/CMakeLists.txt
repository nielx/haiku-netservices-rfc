add_library(netservices_rfc 
	HttpAuthentication.cpp
	HttpForm.cpp
	HttpHeaders.cpp
	HttpMethod.cpp
	HttpRequest.cpp
	HttpSession.cpp
	NetworkRequest.cpp
	UrlProtocolRoster.cpp
	UrlRequest.cpp
	UrlResult.cpp
)

target_include_directories(netservices_rfc PUBLIC
                           "${PROJECT_SOURCE_DIR}/headers/public" 
                           )
