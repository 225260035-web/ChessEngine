#ifndef ARAYUZ_H
#define ARAYUZ_H

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "Sabitler.h"
#include "Hamle.h"
#include <memory>
#include <map>
#include <vector>
#include <string>

// İleri tanımlamalar
class Tahta;
class Oyun;

class Arayuz {
private:
    // SFML pencere ve görüntüleme
    sf::RenderWindow pencere;
    sf::View tahtaGorunumu;
    sf::View panelGorunumu;
    
    // Boyutlar ve konumlar
    static constexpr int KARE_BOYUTU = 80;
    static constexpr int TAHTA_BOYUTU = 8 * KARE_BOYUTU;
    static constexpr int PANEL_GENISLIGI = 300;
    static constexpr int PENCERE_GENISLIGI = TAHTA_BOYUTU + PANEL_GENISLIGI;
    static constexpr int PENCERE_YUKSEKLIGI = TAHTA_BOYUTU;
    
    // Renkler
    sf::Color beyazKareRengi;
    sf::Color siyahKareRengi;
    sf::Color seciliKareRengi;
    sf::Color legalHamleRengi;
    sf::Color sonHamleKaynakRengi;
    sf::Color sonHamleHedefRengi;
    sf::Color sahKontrolRengi;
    sf::Color rokHamleRengi;  // Rok hamlesi için özel renk
    sf::Color panelArkaplanRengi;
    
    // Taş görselleri
    std::map<std::pair<Renk, TasTuru>, sf::Texture> tasTextureleri;
    std::map<std::pair<Renk, TasTuru>, sf::Sprite> tasSpriteleri;
    
    // Fontlar
    sf::Font anaFont;
    sf::Font sembolFont;
    
    // Oyun durumu görselleri
    int seciliKare;
    std::vector<int> legalHamleKareleri;
    std::vector<int> rokHamleKareleri;  // Rok hamleleri için ayrı liste
    Hamle sonHamle;
    bool hamleAnimasyonu;
    
    // Sürükleme değişkenleri
    bool suruklemeAktif;
    int suruklenenKare;
    sf::Vector2f suruklemeOffset;
    
    // Terfi seçimi
    bool terfiSecimAktif;
    int terfiKaynak;
    int terfiHedef;
    
    // Panel bilgileri
    std::vector<std::string> hamleGecmisi;
    std::string motorDusuncesi;
    int motorDerinlik;
    int motorSkor;
    uint64_t motorDugumSayisi;
    
    // Oyun referansı
    Oyun* oyun;
    
    // Derinlik ayarı
    int hedefDerinlik;
    
    // FEN girişi
    bool fenGirisAktif;
    std::string fenMetni;
    bool sadeceDegerlendirmeModAktif;
    sf::Clock clock;
    
public:
    // Yapıcı ve yıkıcı
    Arayuz();
    ~Arayuz();
    
    // Başlatma
    bool baslat();
    void oyunBagla(Oyun* o) { oyun = o; }
    
    // Ana döngü
    void calistir();
    
    // Güncelleme fonksiyonları
    void olaylariIsle();
    void guncelle();
    void ciz();
    
    // Tahta çizimi
    void tahtaCiz();
    void taslariCiz(const Tahta& tahta);
    void seciliKareCiz();
    void legalHamleleriCiz();
    void sonHamleyiCiz();
    void sahKontrolCiz(const Tahta& tahta);
    
    // Panel çizimi
    void panelCiz();
    void oyunBilgileriniCiz();
    void hamleGecmisiCiz();
    void motorBilgileriniCiz();
    void kontrolButonlariCiz();
    void derinlikKontrolleriniCiz();
    
    // Kullanıcı etkileşimi
    void fareHareketiniIsle(const sf::Event& olay);
    void fareTiklamisiniIsle(const sf::Event& olay);
    void fareBirakmaIsle(const sf::Event& olay);
    void klavyeGirdisiniIsle(const sf::Event& olay);
    
    // Hamle işleme
    void hamleYap(int kaynak, int hedef);
    void terfiSeciminiGoster(int kaynak, int hedef);
    void terfiTasiniSec(TasTuru tas);
    void terfiPenceresiCiz();
    void terfiSecimiTiklamaIsle(const sf::Vector2f& pozisyon);
    
    // Yardımcı fonksiyonlar
    int pixeldenKareye(const sf::Vector2f& pozisyon) const;
    sf::Vector2f kareyePixel(int kare) const;
    bool tahtaIcindeMi(const sf::Vector2f& pozisyon) const;
    
    // Görsel yükleme
    bool tasGorselleriniYukle();
    bool fontlariYukle();
    
    // Animasyon
    void hamleAnimasyonuBaslat(const Hamle& hamle);
    void animasyonuGuncelle();
    
    // Bilgi güncelleme
    void hamleEkle(const Hamle& hamle, const std::string& notasyon);
    void motorBilgisiGuncelle(int derinlik, int skor, uint64_t dugumSayisi, 
                             const std::string& dusunce);
    void oyunSonuBildir(const std::string& sonuc);
    
    // FEN işlemleri
    void fenGirisiniGoster();
    void fenGirisiniKapat();
    void fenGirisineCiz();
    void fenYukle(const std::string& fen);
    
    // Ayarlar
    void renkleriAyarla();
    void tahtagorunumuTersle();
    
private:
    // Yardımcı çizim fonksiyonları
    void dikdortgenCiz(int kare, const sf::Color& renk, float kalinlik = 0);
    void daireCiz(int kare, const sf::Color& renk, float yariCap);
    void metinCiz(const std::string& metin, const sf::Vector2f& pozisyon, 
                  int boyut, const sf::Color& renk);
    
    // Sprite yönetimi
    void spriteKonumlandir(sf::Sprite& sprite, int kare);
    void spriteOlceklendir(sf::Sprite& sprite);
    
    // Koordinat notasyonu
    void koordinatlariCiz();
    std::string kareNotasyonu(int kare) const;
};

#endif // ARAYUZ_H