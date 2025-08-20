#ifndef TAHTA_H
#define TAHTA_H

#include "Sabitler.h"
#include "Hamle.h"
#include <vector>
#include <string>
#include <random>

// Oyun durumu yapısı - geri alınamaz bilgileri saklar
struct TahtaDurumu {
    bool rokHaklari[2][2];  // [renk][kısa/uzun]
    int enPassantKare;
    int elliHamleKurali;
    uint64_t zobristHash;
    
    TahtaDurumu() : enPassantKare(-1), elliHamleKurali(0), zobristHash(0) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                rokHaklari[i][j] = false;
            }
        }
    }
};

class Tahta {
    friend class AramaMotoru; // Prefetch optimizasyonu için
private:
    // Bitboard'lar - her taş türü ve renk için
    Bitboard bitboardlar[2][7]; // [renk][taş türü] - 7. indeks tüm taşlar için
    
    // Oyun durumu
    Renk sira;
    int enPassantKare;
    bool rokHaklari[2][2]; // [renk][kısa/uzun]
    int elliHamleKurali;
    int hamleNumarasi;
    
    // Zobrist hash
    uint64_t zobristHash;
    static uint64_t zobristTaslar[64][2][6];
    static uint64_t zobristRok[2][2];
    static uint64_t zobristEnPassant[8];
    static uint64_t zobristSira;
    static bool zobristBaslatildi;
    
    // Oyun durumu geçmişi
    std::vector<TahtaDurumu> durumGecmisi;
    
    // Son yapılan hamle
    Hamle sonHamle;
    
    // Zobrist tabloları başlat
    static void zobristBaslat();
    
    // Yardımcı fonksiyonlar
    void bitboardGuncelle();
    uint64_t zobristHesapla() const;

public:
    // Yapıcı ve yıkıcı
    Tahta();
    ~Tahta() = default;
    
    // Kopyalama ve atama
    Tahta(const Tahta& diger);
    Tahta& operator=(const Tahta& diger);
    
    // Tahta kurulumu
    void baslangicPozisyonu();
    void fenKur(const std::string& fen);
    std::string fenAl() const;
    
    // Taş erişim fonksiyonları
    TasTuru tasAl(int kare) const;
    Renk renkAl(int kare) const;
    bool bosMu(int kare) const;
    
    // Bitboard erişimi
    Bitboard getBitboard(Renk renk, TasTuru tas) const {
        return bitboardlar[static_cast<int>(renk)][static_cast<int>(tas)];
    }
    
    Bitboard getTumTaslar(Renk renk) const {
        return bitboardlar[static_cast<int>(renk)][6];
    }
    
    Bitboard getTumTaslar() const {
        return bitboardlar[0][6] | bitboardlar[1][6];
    }
    
    // Hamle yapma ve geri alma
    void hamleYap(const Hamle& hamle);
    void hamleGeriAl(const Hamle& hamle);
    
    // Pas geçme (null move için)
    void pasGec() {
        sira = tersRenk(sira);
        zobristHash ^= zobristSira;
        enPassantKare = -1;
    }
    
    void pasGeriAl() {
        sira = tersRenk(sira);
        zobristHash ^= zobristSira;
    }
    
    // Oyun durumu kontrolleri
    bool sahCekildiMi(Renk renk) const;
    bool legalMi(const Hamle& hamle) const;
    bool matMi() const;
    bool patMi() const;
    bool berabereMi() const;
    
    // Rok kontrolü
    bool kisaRokYapabilirMi(Renk renk) const;
    bool uzunRokYapabilirMi(Renk renk) const;
    
    // Getters
    Renk getSira() const { return sira; }
    int getEnPassantKare() const { return enPassantKare; }
    uint64_t getZobristHash() const { return zobristHash; }
    int getElliHamleKurali() const { return elliHamleKurali; }
    int getHamleNumarasi() const { return hamleNumarasi; }
    const Hamle& getSonHamle() const { return sonHamle; }
    
    // Setters
    void setSira(Renk r) { sira = r; }
    
    // Saldırı kontrolleri
    bool kareAtakAltindaMi(int kare, Renk saldiran) const;
    Bitboard saldirilar(int kare, TasTuru tas, Renk renk) const;
    
    // Şah pozisyonu
    int sahPozisyonu(Renk renk) const;
    
    // Yeni eklenen metodlar - Degerlendirici için
    int sahBul(Renk renk) const { return sahPozisyonu(renk); }
    Bitboard taslariBul(Renk renk, TasTuru tas) const { return getBitboard(renk, tas); }
    Bitboard taslariBul(Renk renk) const { return getTumTaslar(renk); }
    Bitboard taslariBul(TasTuru tas) const { 
        return getBitboard(Renk::BEYAZ, tas) | getBitboard(Renk::SIYAH, tas); 
    }
    Bitboard tumTaslar() const { return getTumTaslar(); }
    TasTuru karedekiTasTuru(int kare) const { return tasAl(kare); }
    Renk siraKimde() const { return getSira(); }
    
    // Debug ve görselleştirme
    void yazdir() const;
    std::string toString() const;
    
private:
    // Hamle yapma yardımcıları
    void normalHamleYap(const Hamle& hamle);
    void rokYap(const Hamle& hamle);
    void enPassantYap(const Hamle& hamle);
    void terfiYap(const Hamle& hamle);
    
    // Hamle geri alma yardımcıları
    void normalHamleGeriAl(const Hamle& hamle);
    void rokGeriAl(const Hamle& hamle);
    void enPassantGeriAl(const Hamle& hamle);
    void terfiGeriAl(const Hamle& hamle);
    
    // Taş yerleştirme/kaldırma
    void tasYerlestir(int kare, Renk renk, TasTuru tas);
    void tasKaldir(int kare, Renk renk, TasTuru tas);
    void tasTasi(int kaynak, int hedef, Renk renk, TasTuru tas);
};

#endif // TAHTA_H