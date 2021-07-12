add_library(netservices_rfc UrlProtocolRoster.cpp UrlRequest.cpp)

target_include_directories(netservices_rfc PUBLIC
                           "${PROJECT_SOURCE_DIR}/headers/public" 
                           )
