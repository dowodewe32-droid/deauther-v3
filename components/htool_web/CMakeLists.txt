idf_component_register(
    SRCS "htool_web.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi http_parser esp_netif
)