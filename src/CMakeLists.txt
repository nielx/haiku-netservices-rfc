add_library(netservices_rfc 
	HttpRequest.cpp NetworkRequest.cpp UrlProtocolRoster.cpp UrlRequest.cpp)

target_include_directories(netservices_rfc PUBLIC
                           "${PROJECT_SOURCE_DIR}/headers/public" 
                           )
