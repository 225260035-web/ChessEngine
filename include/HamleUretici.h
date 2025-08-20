#ifndef HAMLE_URETICI_H
#define HAMLE_URETICI_H

#include "Sabitler.h"
#include "Hamle.h"
#include "BitboardYardimci.h"

// İleri tanımlama
class Tahta;

class HamleUretici {
private:
    // Statik lookup tabloları - bir kez hesaplanır
    static bool tablolarHazir;
    
    // Sihirli sayılar (Magic Bitboards) için
    struct MagicEntry {
        Bitboard maske;
        Bitboard magic;
        int shift;
        Bitboard* saldirilar;
    };
    
    static MagicEntry kaleMagic[64];
    static MagicEntry filMagic[64];
    static Bitboard kaleSaldiriTablosu[102400]; // Yaklaşık boyut
    static Bitboard filSaldiriTablosu[5248];    // Yaklaşık boyut

public:    
    // Tablo başlatma fonksiyonları
    static void tablolariBaslat();
    
private:
    static void atHamleleriniBaslat();
    static void sahHamleleriniBaslat();
    static void piyonSaldirilariniBaslat();
    static void magicBitboardBaslat();
    
    // Magic bitboard yardımcıları
    static Bitboard kaleHamleMaskesi(int kare);
    static Bitboard filHamleMaskesi(int kare);
    static Bitboard kaleSaldiriHesapla(int kare, Bitboard engeller);
    static Bitboard filSaldiriHesapla(int kare, Bitboard engeller);
    
public:
    // Hamle üretimi - ana fonksiyon
    static void tumHamleleriUret(const Tahta& tahta, HamleListesi& hamleler);
    static void saldiriHamleleriUret(const Tahta& tahta, HamleListesi& hamleler);
    static void kacisHamleleriUret(const Tahta& tahta, HamleListesi& hamleler);
    
    // Taş bazlı hamle üretimi
    static void piyonHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk);
    static void atHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk);
    static void filHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk);
    static void kaleHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk);
    static void vezirHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk);
    static void sahHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk);
    
    // Özel hamleler
    static void rokHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk);
    static void enPassantHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk);
    
    // Saldırı hesaplamaları (magic bitboard kullanarak)
    static Bitboard atSaldirilari(int kare);
    static Bitboard sahSaldirilari(int kare);
    static Bitboard piyonSaldirilari(int kare, Renk renk);
    static Bitboard kaleSaldirilari(int kare, Bitboard engeller);
    static Bitboard filSaldirilari(int kare, Bitboard engeller);
    static Bitboard vezirSaldirilari(int kare, Bitboard engeller);
    
    // Yardımcı fonksiyonlar
    static bool pseudoLegalMi(const Tahta& tahta, const Hamle& hamle);
    static void hamleleriSirala(HamleListesi& hamleler, const Tahta& tahta);
    
    // Hamle skorlama (hamle sıralama için)
    static int hamlePuanla(const Hamle& hamle, const Tahta& tahta);
    
    // Pin ve tehdit analizi
    static Bitboard pinliTaslar(const Tahta& tahta, Renk renk);
    static Bitboard tehditAltindakiKareler(const Tahta& tahta, Renk saldiran);
    
private:
    // Piyon hamle yardımcıları
    static void piyonItmeHamleleri(const Tahta& tahta, HamleListesi& hamleler, 
                                   Renk renk, Bitboard hedefler);
    static void piyonSaldiriHamleleri(const Tahta& tahta, HamleListesi& hamleler, 
                                     Renk renk, Bitboard hedefler);
    static void terfiHamleleriEkle(HamleListesi& hamleler, int kaynak, int hedef, 
                                   HamleTuru tur);
    
    // Işın hamleleri (kale, fil, vezir için)
    static void isinHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, 
                                  int kare, Bitboard saldirilar, TasTuru tas);
};

#endif // HAMLE_URETICI_H