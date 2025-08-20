#ifndef OYUN_H
#define OYUN_H

#include "Sabitler.h"
#include "Tahta.h"
#include "HamleUretici.h"
#include "AramaMotoru.h"
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <future>
#include <string>

// İleri tanımlama
class Arayuz;

// Oyuncu türleri
enum class OyuncuTuru {
    INSAN,
    BILGISAYAR
};

// Oyun durumu
enum class OyunDurumu {
    DEVAM,
    MAT,
    PAT,
    BERABERE_TEKRAR,
    BERABERE_50_HAMLE,
    BERABERE_YETERSIZ_MATERYAL,
    ZAMAN_BITTI
};

// Oyuncu bilgisi
struct Oyuncu {
    OyuncuTuru tur;
    Renk renk;
    std::string isim;
    int dusunmeSuresi; // Milisaniye cinsinden
    int aramaDerinligi;
    
    Oyuncu(OyuncuTuru t, Renk r, const std::string& i = "")
        : tur(t), renk(r), isim(i), dusunmeSuresi(5000), aramaDerinligi(6) {}
};

// Oyun ayarları
struct OyunAyarlari {
    int beyazSure;      // Dakika cinsinden
    int siyahSure;
    int ekSure;         // Hamle başına ek süre (saniye)
    bool sonsuzAnaliz;
    int motorDerinlik;
    size_t ttBoyutMB;
    
    OyunAyarlari()
        : beyazSure(10), siyahSure(10), ekSure(0),
          sonsuzAnaliz(false), motorDerinlik(8), ttBoyutMB(64) {}
};

// Ana oyun sınıfı
class Oyun {
private:
    // Oyun durumu
    Tahta tahta;
    OyunDurumu durum;
    std::vector<Hamle> hamleGecmisi;
    std::vector<std::string> fenGecmisi;
    
    // Oyuncular
    Oyuncu beyazOyuncu;
    Oyuncu siyahOyuncu;
    
    // Motor
    std::unique_ptr<AramaMotoru> motor;
    std::future<Hamle> motorSonucu;
    std::mutex motorMutex;
    bool motorDusunuyor;
    
    // Arayüz referansı
    Arayuz* arayuz;
    
    // Oyun ayarları
    OyunAyarlari ayarlar;
    
    // Zaman yönetimi
    std::chrono::steady_clock::time_point oyunBaslangic;
    std::chrono::milliseconds beyazKalanSure;
    std::chrono::milliseconds siyahKalanSure;
    std::chrono::steady_clock::time_point hamleBaslangic;
    
public:
    // Yapıcı ve yıkıcı
    Oyun();
    ~Oyun();
    
    // Oyun başlatma
    void yeniOyun();
    void fendenBaslat(const std::string& fen);
    void fenYukle(const std::string& fen);
    void ayarlariYap(const OyunAyarlari& yeniAyarlar);
    
    // Arayüz bağlantısı
    void arayuzBagla(Arayuz* a) { arayuz = a; }
    
    // Oyuncu ayarları
    void oyuncuAyarla(Renk renk, OyuncuTuru tur, const std::string& isim = "");
    void motorDerinligiAyarla(int derinlik);
    
    // Hamle yapma
    bool hamleYap(const Hamle& hamle);
    bool hamleYap(int kaynak, int hedef, TasTuru terfiTasi = TasTuru::VEZIR);
    void hamleGeriAl();
    
    // Motor kontrolü
    void motoruBaslat();
    void motoruDurdur();
    bool motorHazirMi();
    void motorHamlesiniAl();
    void motorDerinliginiAyarla(int derinlik);
    
    // Oyun durumu kontrolleri
    OyunDurumu durumKontrol();
    bool oyunBittiMi() const { return durum != OyunDurumu::DEVAM; }
    std::string sonucMetni() const;
    bool siradakiOyuncuInsan() const;
    
    // Legal hamle kontrolü
    bool legalHamlelerVar() const;
    HamleListesi legalHamleleriAl() const;
    HamleListesi legalHamleleriAl(int kare) const;
    
    // Getters
    const Tahta& getTahta() const { return tahta; }
    Renk getSira() const { return tahta.getSira(); }
    OyunDurumu getDurum() const { return durum; }
    const std::vector<Hamle>& getHamleGecmisi() const { return hamleGecmisi; }
    bool motorDusunuyorMu() const { return motorDusunuyor; }
    
    // FEN ve PGN
    std::string fenAl() const { return tahta.fenAl(); }
    std::string pgnAl() const;
    bool pgnYukle(const std::string& pgn);
    
    // Analiz
    void pozisyonAnalizi();
    int pozisyonDegeri() const;
    std::vector<std::pair<Hamle, int>> hamleAnalizi();
    
    // Zaman yönetimi
    std::chrono::milliseconds kalanSure(Renk renk) const;
    void sureGuncelle();
    bool sureKontrol();
    
    // Debug
    void tahtaYazdir() const { tahta.yazdir(); }
    void istatistikleriYazdir() const;
    
private:
    // Yardımcı fonksiyonlar
    void hamleKaydet(const Hamle& hamle);
    bool berabereTekrariMi() const;
    bool elliHamleKuraliMi() const;
    bool yetersizMateryalMi() const;
    
    // Motor thread fonksiyonu
    void motorDusun();
    
    // Notasyon dönüşümleri
    std::string hamleNotasyonu(const Hamle& hamle) const;
    Hamle notasyondanHamle(const std::string& notasyon) const;
    
    // PGN yardımcıları
    std::vector<std::string> pgnParcala(const std::string& pgn) const;
    bool pgnHamleOyna(const std::string& hamle);
};

#endif // OYUN_H