cmake_minimum_required(VERSION 3.10)
project(SatrancMotoru VERSION 1.0.0 LANGUAGES CXX)

# C++ standardı
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Derleme bayrakları
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# SFML bulma
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Include dizinleri
include_directories(${CMAKE_SOURCE_DIR}/include)

# Kaynak dosyaları
set(KAYNAK_DOSYALARI
    src/ana.cpp
    src/Tahta.cpp
    src/HamleUretici.cpp
    src/Degerlendirici.cpp
    src/AramaMotoru.cpp
    src/Arayuz.cpp
    src/Oyun.cpp
    src/BitboardYardimci.cpp
)

# Başlık dosyaları
set(BASLIK_DOSYALARI
    include/Tahta.h
    include/HamleUretici.h
    include/Degerlendirici.h
    include/AramaMotoru.h
    include/Arayuz.h
    include/Oyun.h
    include/Hamle.h
    include/BitboardYardimci.h
    include/Sabitler.h
)

# Çalıştırılabilir dosya
add_executable(SatrancMotoru ${KAYNAK_DOSYALARI} ${BASLIK_DOSYALARI})

# SFML kütüphanelerini bağla
target_link_libraries(SatrancMotoru sfml-graphics sfml-window sfml-system)

# Kaynaklar klasörünü çıktı dizinine kopyala
add_custom_command(TARGET SatrancMotoru POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/kaynaklar $<TARGET_FILE_DIR:SatrancMotoru>/kaynaklar)

# Thread desteği
find_package(Threads REQUIRED)
target_link_libraries(SatrancMotoru Threads::Threads)