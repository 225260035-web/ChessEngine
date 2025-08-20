#include "Oyun.h"
#include "Arayuz.h"
#include "HamleUretici.h"
#include "BitboardYardimci.h"
#include "Degerlendirici.h"
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    (void)argc; // Unused parameter
    (void)argv; // Unused parameter
    
    std::cout << "Satranc Motoru - C++ / SFML\n";
    std::cout << "===========================\n\n";
    
    // Tabloları başlat
    BitboardYardimci::tablolariBaslat();
    HamleUretici::tablolariBaslat();
    Degerlendirici::tablolariBaslat();
    
    // Exception handling devre dışı olduğu için try-catch kaldırıldı
    // Arayüz oluştur
    auto arayuz = std::make_unique<Arayuz>();
    
    // Arayüzü başlat
    if (!arayuz->baslat()) {
        std::cerr << "Hata: Arayüz başlatılamadı!\n";
        return 1;
    }
    
    // Oyun oluştur
    auto oyun = std::make_unique<Oyun>();
    
    // Oyun ayarları
    OyunAyarlari ayarlar;
    ayarlar.motorDerinlik = 6;
    ayarlar.ttBoyutMB = 64;
    ayarlar.beyazSure = 10;
    ayarlar.siyahSure = 10;
    
    oyun->ayarlariYap(ayarlar);
    
    // Oyuncuları ayarla (Beyaz: İnsan, Siyah: Bilgisayar)
    oyun->oyuncuAyarla(Renk::BEYAZ, OyuncuTuru::INSAN, "Insan");
    oyun->oyuncuAyarla(Renk::SIYAH, OyuncuTuru::BILGISAYAR, "Motor");
    
    // Arayüz ve oyunu bağla
    arayuz->oyunBagla(oyun.get());
    oyun->arayuzBagla(arayuz.get());
    
    // Yeni oyun başlat
    oyun->yeniOyun();
    
    // Ana döngüyü çalıştır
    arayuz->calistir();
    
    return 0;
}