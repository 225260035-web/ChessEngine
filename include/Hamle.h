#ifndef HAMLE_H
#define HAMLE_H

#include "Sabitler.h"
#include <string>

// Hamle veri yapısı
struct Hamle {
    int kaynak;           // Başlangıç kare (0-63)
    int hedef;            // Hedef kare (0-63)
    TasTuru tas;          // Hareket eden taş
    TasTuru alinanTas;    // Alınan taş (varsa)
    HamleTuru tur;        // Hamle türü
    TasTuru terfiTasi;    // Terfi taşı (terfi hamlesi için)
    int skor;             // Hamle sıralama skoru
    
    // Yapıcılar
    Hamle() : kaynak(0), hedef(0), tas(TasTuru::YOK), alinanTas(TasTuru::YOK), 
              tur(HamleTuru::NORMAL), terfiTasi(TasTuru::YOK), skor(0) {}
    
    Hamle(int k, int h, TasTuru t, HamleTuru ht = HamleTuru::NORMAL) 
        : kaynak(k), hedef(h), tas(t), alinanTas(TasTuru::YOK), 
          tur(ht), terfiTasi(TasTuru::YOK), skor(0) {}
    
    Hamle(int k, int h, TasTuru t, TasTuru at, HamleTuru ht) 
        : kaynak(k), hedef(h), tas(t), alinanTas(at), 
          tur(ht), terfiTasi(TasTuru::YOK), skor(0) {}
    
    // Karşılaştırma operatörleri
    bool operator==(const Hamle& diger) const {
        return kaynak == diger.kaynak && 
               hedef == diger.hedef && 
               terfiTasi == diger.terfiTasi;
    }
    
    bool operator!=(const Hamle& diger) const {
        return !(*this == diger);
    }
    
    // Sıralama için (skor bazlı)
    bool operator<(const Hamle& diger) const {
        return skor > diger.skor; // Büyük skor önce
    }
    
    // Geçerlilik kontrolü
    bool gecerliMi() const {
        return kaynak >= 0 && kaynak < 64 && hedef >= 0 && hedef < 64;
    }
    
    // Hamleyi notasyon string'ine çevir (örn: "e2e4", "e7e8q")
    std::string notasyon() const {
        std::string sonuc;
        
        // Kaynak kare
        sonuc += static_cast<char>('a' + sutunIndeksi(kaynak));
        sonuc += static_cast<char>('1' + satirIndeksi(kaynak));
        
        // Hedef kare
        sonuc += static_cast<char>('a' + sutunIndeksi(hedef));
        sonuc += static_cast<char>('1' + satirIndeksi(hedef));
        
        // Terfi varsa
        if (tur == HamleTuru::TERFI || tur == HamleTuru::TERFI_ALMA) {
            switch (terfiTasi) {
                case TasTuru::VEZIR: sonuc += 'q'; break;
                case TasTuru::KALE: sonuc += 'r'; break;
                case TasTuru::FIL: sonuc += 'b'; break;
                case TasTuru::AT: sonuc += 'n'; break;
                default: break;
            }
        }
        
        return sonuc;
    }
    
    // Debug için string gösterimi
    std::string toString() const {
        std::string sonuc = notasyon();
        sonuc += " (";
        sonuc += tasAdi(tas);
        
        if (alinanTas != TasTuru::YOK) {
            sonuc += " x ";
            sonuc += tasAdi(alinanTas);
        }
        
        switch (tur) {
            case HamleTuru::KISA_ROK: sonuc += ", O-O"; break;
            case HamleTuru::UZUN_ROK: sonuc += ", O-O-O"; break;
            case HamleTuru::EN_PASSANT: sonuc += ", e.p."; break;
            case HamleTuru::IKI_KARE: sonuc += ", 2-kare"; break;
            default: break;
        }
        
        sonuc += ")";
        return sonuc;
    }
};

// Hamle listesi (sabit boyutlu dizi, hız için)
class HamleListesi {
private:
    static constexpr int MAX_HAMLE = 256;
    Hamle hamleler[MAX_HAMLE];
    int boyut;
    
public:
    HamleListesi() : boyut(0) {}
    
    void ekle(const Hamle& hamle) {
        if (boyut < MAX_HAMLE) {
            hamleler[boyut++] = hamle;
        }
    }
    
    void ekle(int kaynak, int hedef, TasTuru tas, HamleTuru tur = HamleTuru::NORMAL) {
        ekle(Hamle(kaynak, hedef, tas, tur));
    }
    
    void ekle(int kaynak, int hedef, TasTuru tas, TasTuru alinanTas, HamleTuru tur) {
        ekle(Hamle(kaynak, hedef, tas, alinanTas, tur));
    }
    
    int getBoyut() const { return boyut; }
    bool bosMu() const { return boyut == 0; }
    
    Hamle& operator[](int indeks) { return hamleler[indeks]; }
    const Hamle& operator[](int indeks) const { return hamleler[indeks]; }
    
    // İterator desteği
    Hamle* begin() { return hamleler; }
    Hamle* end() { return hamleler + boyut; }
    const Hamle* begin() const { return hamleler; }
    const Hamle* end() const { return hamleler + boyut; }
    
    void temizle() { boyut = 0; }
    
    // Hamle sıralama (insertion sort - küçük listeler için yeterli)
    void sirala() {
        for (int i = 1; i < boyut; i++) {
            Hamle anahtar = hamleler[i];
            int j = i - 1;
            
            while (j >= 0 && hamleler[j].skor < anahtar.skor) {
                hamleler[j + 1] = hamleler[j];
                j--;
            }
            
            hamleler[j + 1] = anahtar;
        }
    }
};

#endif // HAMLE_H