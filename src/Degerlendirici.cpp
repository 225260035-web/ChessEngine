/*
  Satranç Motoru - Stockfish tabanlı değerlendirme sistemi
  Türkçe isimlendirme kullanılmıştır.
*/

#include "Degerlendirici.h"
#include "Tahta.h"
#include "BitboardYardimci.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace Iz {

  enum Izleme { IZLEME_YOK, IZLEME_VAR };

  enum Terim { // İlk 8 giriş TasTuru için ayrılmıştır
    MATERYAL = 8, DENGESIZLIK, MOBILITE, TEHDIT, GECER_PIYON, ALAN, INISIYATIF, TOPLAM, TERIM_SAYISI
  };

  int skorlar[TERIM_SAYISI][2]; // [terim][renk]

  double cp_donustur(int deger) { return double(deger) / 100.0; }

  void ekle(int idx, Renk r, int skor) {
    skorlar[idx][static_cast<int>(r)] = skor;
  }

  void ekle(int idx, int beyaz, int siyah = 0) {
    skorlar[idx][static_cast<int>(Renk::BEYAZ)] = beyaz;
    skorlar[idx][static_cast<int>(Renk::SIYAH)] = siyah;
  }
}

using namespace Iz;

namespace {

  // Tembel ve alan değerlendirme eşikleri
  constexpr int TembelEsik = 1400;
  constexpr int AlanEsigi = 12222;

  // Şah saldırı ağırlıkları taş türüne göre
  constexpr int SahSaldiriAgirliklari[6] = { 0, 81, 52, 44, 10, 0 };

  // Düşman güvenli şahları için cezalar
  constexpr int VezirGuvenliSah = 780;
  constexpr int KaleGuvenliSah = 1080;
  constexpr int FilGuvenliSah = 635;
  constexpr int AtGuvenliSah = 790;

  // Mobilite bonusları [TasTuru-2][saldırılan kare sayısı]
  // Stockfish'te S(mg, eg) makrosuyla tanımlanmış, burada sadece orta oyun değerlerini kullanıyoruz
  constexpr int MobiliteBonus[4][32] = {
    // Atlar
    { -62, -53, -12, -4, 3, 13, 22, 28, 33, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    // Filler  
    { -48, -20, 16, 26, 38, 51, 55, 63, 63, 68, 81, 81, 91, 98, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    // Kaleler
    { -58, -27, -15, -10, -5, -2, 9, 16, 30, 29, 32, 38, 46, 48, 58, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    // Vezirler
    { -39, -21, 3, 3, 14, 22, 28, 41, 43, 48, 56, 60, 60, 66, 67, 70,
      71, 73, 79, 88, 88, 99, 102, 102, 106, 109, 113, 116, 0, 0, 0, 0 }
  };

  // Kale dosya bonusları
  constexpr int KaleDosyada[2] = { 21, 47 }; // [yarı açık, açık]

  // Küçük taş ve kale tehditleri
  constexpr int KucukTasTehdit[6] = { 0, 6, 59, 79, 90, 79 };
  constexpr int KaleTehdit[6] = { 0, 3, 38, 38, 0, 51 };

  // Geçer piyon sıra bonusları
  constexpr int GecerPiyonBonus[8] = { 0, 10, 17, 15, 62, 168, 276, 0 };

  // Çeşitli bonuslar ve cezalar
  constexpr int FilPiyonlar = 3;
  constexpr int KoseFilCezasi = 50;
  constexpr int KanatSaldiri = 8;
  constexpr int Asili = 69;
  constexpr int SahKoruyucu = 7;
  constexpr int AtVezirUzerinde = 16;
  constexpr int UzunDiyagonalFil = 45;
  constexpr int KucukTasPiyonArkasi = 18;
  constexpr int IleriKarakol = 30;
  constexpr int GecerDosya = 11;
  constexpr int PiyonsuzKanat = 17;
  constexpr int KisitliTas = 7;
  constexpr int UlasilabilirKarakol = 32;
  constexpr int KaleVezirDosyasinda = 7;
  constexpr int KayanVezirUzerinde = 59;
  constexpr int SahTehdit = 24;
  constexpr int PiyonItmeTehdit = 48;
  constexpr int GuvenliPiyonTehdit = 173;
  constexpr int TuzakKale = 52;
  constexpr int ZayifVezir = 49;

  // Değerlendirme sınıfı
  template<Izleme T>
  class Degerlendirme {
  public:
    explicit Degerlendirme(const Tahta& t) : tahta(t) {}
    int deger();

  private:
    template<Renk Biz> void baslat();
    template<Renk Biz, TasTuru Tas> int taslar();
    template<Renk Biz> int sah() const;
    template<Renk Biz> int tehditler() const;
    template<Renk Biz> int gecerler() const;
    template<Renk Biz> int alan() const;
    int inisiyatif(int skor) const;

    const Tahta& tahta;
    Bitboard mobiliteAlani[2];
    int mobilite[2] = { 0, 0 };
    
    // Saldırı bitboardları
    Bitboard saldiranlar[2][7]; // [renk][taş türü]
    Bitboard ciftSaldiri[2];
    Bitboard sahHalkasi[2];
    int sahSaldirganSayisi[2];
    int sahSaldirganAgirligi[2];
    int sahSaldiriSayisi[2];
    
    // Materyal dengesizliği (basit değerlendirme için)
    int materyalDengesizligi;
    
    // Piyon yapısı değerlendirmesi (basit)
    int piyonYapisiSkoru[2];
    
    // Oyun evresi
    int oyunEvresi; // 0-256 (0=son oyun, 256=orta oyun)
  };

  // Başlatma fonksiyonu
  template<Izleme T> template<Renk Biz>
  void Degerlendirme<T>::baslat() {
    constexpr Renk Onlar = (Biz == Renk::BEYAZ ? Renk::SIYAH : Renk::BEYAZ);
    constexpr int Ileri = (Biz == Renk::BEYAZ ? 8 : -8);
    constexpr Bitboard DusukSiralar = (Biz == Renk::BEYAZ ? SATIR_2 | SATIR_3 : SATIR_7 | SATIR_6);

    // Şah pozisyonu
    int sahKare = tahta.sahBul(Biz);
    if (sahKare < 0) return; // Şah yoksa hata

    // Piyon saldırıları
    Bitboard piyonSaldiri = 0;
    Bitboard piyonlar = tahta.taslariBul(Biz, TasTuru::PIYON);
    
    // Piyonlar için basit saldırı hesabı
    if (Biz == Renk::BEYAZ) {
        piyonSaldiri = ((piyonlar & ~DOSYA_A) << 7) | ((piyonlar & ~DOSYA_H) << 9);
    } else {
        piyonSaldiri = ((piyonlar & ~DOSYA_H) >> 7) | ((piyonlar & ~DOSYA_A) >> 9);
    }

    // Mobilite alanı - şah, vezir ve engellenmiş piyonlar hariç
    Bitboard engellenmis = piyonlar & (tahta.tumTaslar() >> Ileri);
    mobiliteAlani[static_cast<int>(Biz)] = ~(engellenmis | tahta.taslariBul(Biz, TasTuru::SAH) | tahta.taslariBul(Biz, TasTuru::VEZIR));

    // Saldırı tablolarını başlat
    saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::SAH)] = BitboardYardimci::sahHamlesi[sahKare];
    saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::PIYON)] = piyonSaldiri;
    saldiranlar[static_cast<int>(Biz)][6] = saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::SAH)] | piyonSaldiri;

    // Şah güvenlik tabloları
    int dosya = sutunIndeksi(sahKare);
    int sira = satirIndeksi(sahKare);
    
    // Şah halkası - şah etrafındaki kareler
    sahHalkasi[static_cast<int>(Biz)] = BitboardYardimci::sahHamlesi[sahKare];
    
    // Düşman piyonlarının şah halkasına saldırı sayısı
    Bitboard dusmanPiyonSaldiri = 0;
    Bitboard dusmanPiyonlar = tahta.taslariBul(Onlar, TasTuru::PIYON);
    if (Onlar == Renk::BEYAZ) {
        dusmanPiyonSaldiri = ((dusmanPiyonlar & ~DOSYA_A) << 7) | ((dusmanPiyonlar & ~DOSYA_H) << 9);
    } else {
        dusmanPiyonSaldiri = ((dusmanPiyonlar & ~DOSYA_H) >> 7) | ((dusmanPiyonlar & ~DOSYA_A) >> 9);
    }
    
    sahSaldirganSayisi[static_cast<int>(Onlar)] = BitboardYardimci::popcount(sahHalkasi[static_cast<int>(Biz)] & dusmanPiyonSaldiri);
    sahSaldiriSayisi[static_cast<int>(Onlar)] = 0;
    sahSaldirganAgirligi[static_cast<int>(Onlar)] = 0;

    // Çift piyon saldırısını kaldır şah halkasından
    ciftSaldiri[static_cast<int>(Biz)] = 0;
  }

  // Taş değerlendirmesi
  template<Izleme T> template<Renk Biz, TasTuru Tas>
  int Degerlendirme<T>::taslar() {
    constexpr Renk Onlar = (Biz == Renk::BEYAZ ? Renk::SIYAH : Renk::BEYAZ);
    constexpr int Ileri = (Biz == Renk::BEYAZ ? 8 : -8);
    constexpr Bitboard IleriKarakolSiralari = (Biz == Renk::BEYAZ ? SATIR_4 | SATIR_5 | SATIR_6 
                                                                  : SATIR_5 | SATIR_4 | SATIR_3);

    Bitboard taslar = tahta.taslariBul(Biz, Tas);
    int skor = 0;

    saldiranlar[static_cast<int>(Biz)][static_cast<int>(Tas)] = 0;

    while (taslar) {
        int kare = BitboardYardimci::lsb(taslar);
        taslar &= taslar - 1;

        // Saldırı hesaplama
        Bitboard saldiri = 0;
        
        switch (Tas) {
            case TasTuru::AT:
                saldiri = BitboardYardimci::atHamlesi[kare];
                break;
            case TasTuru::FIL:
                saldiri = BitboardYardimci::filSaldirisi(kare, tahta.tumTaslar());
                break;
            case TasTuru::KALE:
                saldiri = BitboardYardimci::kaleSaldirisi(kare, tahta.tumTaslar());
                break;
            case TasTuru::VEZIR:
                saldiri = BitboardYardimci::vezirSaldirisi(kare, tahta.tumTaslar());
                break;
            default:
                break;
        }

        // Saldırı tablolarını güncelle
        ciftSaldiri[static_cast<int>(Biz)] |= saldiranlar[static_cast<int>(Biz)][6] & saldiri;
        saldiranlar[static_cast<int>(Biz)][static_cast<int>(Tas)] |= saldiri;
        saldiranlar[static_cast<int>(Biz)][6] |= saldiri;

        // Şah güvenliği
        if (saldiri & sahHalkasi[static_cast<int>(Onlar)]) {
            sahSaldirganSayisi[static_cast<int>(Biz)]++;
            sahSaldirganAgirligi[static_cast<int>(Biz)] += SahSaldiriAgirliklari[static_cast<int>(Tas)];
            sahSaldiriSayisi[static_cast<int>(Biz)] += BitboardYardimci::popcount(saldiri & saldiranlar[static_cast<int>(Onlar)][static_cast<int>(TasTuru::SAH)]);
        }

        // Mobilite
        int mob = BitboardYardimci::popcount(saldiri & mobiliteAlani[static_cast<int>(Biz)]);
        mobilite[static_cast<int>(Biz)] += MobiliteBonus[static_cast<int>(Tas) - 1][mob];

        // Taşa özel değerlendirmeler
        if (Tas == TasTuru::FIL || Tas == TasTuru::AT) {
            // İleri karakol bonusu
            Bitboard karakolKareleri = IleriKarakolSiralari & saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::PIYON)];
            if (karakolKareleri & (1ULL << kare))
                skor += IleriKarakol * (Tas == TasTuru::AT ? 2 : 1);

            // Piyon arkası bonus
            if ((1ULL << (kare - Ileri)) & tahta.taslariBul(Biz, TasTuru::PIYON))
                skor += KucukTasPiyonArkasi;

            // Şah koruyucu ceza
            int sahKare = tahta.sahBul(Biz);
            int mesafe = std::max(std::abs(satirIndeksi(kare) - satirIndeksi(sahKare)),
                                  std::abs(sutunIndeksi(kare) - sutunIndeksi(sahKare)));
            skor -= SahKoruyucu * mesafe;

            if (Tas == TasTuru::FIL) {
                // Aynı renk piyonlar cezası
                Bitboard ayniRenkKareler = ((kare / 8 + kare % 8) % 2 == 0) ? 
                    0x55AA55AA55AA55AAULL : 0xAA55AA55AA55AA55ULL;
                int piyonSayisi = BitboardYardimci::popcount(tahta.taslariBul(Biz, TasTuru::PIYON) & ayniRenkKareler);
                skor -= FilPiyonlar * piyonSayisi;

                // Uzun diyagonal bonusu
                if (BitboardYardimci::popcount(BitboardYardimci::filSaldirisi(kare, tahta.tumTaslar()) & 0x1818000000ULL) > 1)
                    skor += UzunDiyagonalFil;
            }
        }

        if (Tas == TasTuru::KALE) {
            // Vezir dosyası bonusu
            if ((1ULL << kare) & tahta.taslariBul(TasTuru::VEZIR))
                skor += KaleVezirDosyasinda;

            // Açık/yarı açık dosya bonusu
            Bitboard dosya = DOSYA_A << (kare % 8);
            bool bizimPiyonVar = (dosya & tahta.taslariBul(Biz, TasTuru::PIYON)) != 0;
            bool onlarinPiyonVar = (dosya & tahta.taslariBul(Onlar, TasTuru::PIYON)) != 0;
            
            if (!bizimPiyonVar)
                skor += KaleDosyada[onlarinPiyonVar ? 0 : 1];

            // Sıkışmış kale cezası
            if (mob <= 3) {
                int sahDosya = tahta.sahBul(Biz) % 8;
                int kaleDosya = kare % 8;
                if ((sahDosya < 4) == (kaleDosya < sahDosya))
                    skor -= TuzakKale;
            }
        }

        if (Tas == TasTuru::VEZIR) {
            // Zayıf vezir cezası
            Bitboard pinleyenler = 0;
            Bitboard dusmanKayicilar = tahta.taslariBul(Onlar, TasTuru::KALE) | tahta.taslariBul(Onlar, TasTuru::FIL);
            
            // Basit pin kontrolü
            if (dusmanKayicilar) {
                skor -= ZayifVezir;
            }
        }
    }

    if (T == Izleme::IZLEME_VAR)
        Iz::ekle(static_cast<int>(Tas), Biz, skor);

    return skor;
  }

  // Şah güvenliği değerlendirmesi
  template<Izleme T> template<Renk Biz>
  int Degerlendirme<T>::sah() const {
    constexpr Renk Onlar = (Biz == Renk::BEYAZ ? Renk::SIYAH : Renk::BEYAZ);
    
    int sahKare = tahta.sahBul(Biz);
    if (sahKare < 0) return 0;

    // Basit şah güvenliği değerlendirmesi
    int skor = 0;
    int sahTehlike = 0;

    // Zayıf kareler
    Bitboard zayif = saldiranlar[static_cast<int>(Onlar)][6]
                   & ~ciftSaldiri[static_cast<int>(Biz)]
                   & (~saldiranlar[static_cast<int>(Biz)][6] | saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::SAH)]);

    // Güvenli şahlar
    Bitboard guvenli = ~tahta.taslariBul(Onlar);
    guvenli &= ~saldiranlar[static_cast<int>(Biz)][6] | (zayif & ciftSaldiri[static_cast<int>(Onlar)]);

    // Kale şahları
    Bitboard kaleShahi = BitboardYardimci::kaleSaldirisi(sahKare, tahta.tumTaslar()) & guvenli & saldiranlar[static_cast<int>(Onlar)][static_cast<int>(TasTuru::KALE)];
    if (kaleShahi)
        sahTehlike += KaleGuvenliSah;

    // Fil şahları
    Bitboard filShahi = BitboardYardimci::filSaldirisi(sahKare, tahta.tumTaslar()) & guvenli & saldiranlar[static_cast<int>(Onlar)][static_cast<int>(TasTuru::FIL)];
    if (filShahi)
        sahTehlike += FilGuvenliSah;

    // At şahları
    Bitboard atShahi = BitboardYardimci::atHamlesi[sahKare] & saldiranlar[static_cast<int>(Onlar)][static_cast<int>(TasTuru::AT)];
    if (atShahi & guvenli)
        sahTehlike += AtGuvenliSah;

    // Şah tehlike hesabı
    sahTehlike += sahSaldirganSayisi[static_cast<int>(Onlar)] * sahSaldirganAgirligi[static_cast<int>(Onlar)]
                + 185 * BitboardYardimci::popcount(sahHalkasi[static_cast<int>(Biz)] & zayif)
                + 69 * sahSaldiriSayisi[static_cast<int>(Onlar)]
                - 873 * (tahta.taslariBul(Onlar, TasTuru::VEZIR) == 0 ? 1 : 0)
                - 6 * skor / 8
                + 37;

    // Tehlikeyi skora dönüştür
    if (sahTehlike > 100)
        skor -= sahTehlike * sahTehlike / 4096;

    // Piyonsuz kanat cezası
    int sahDosya = sahKare % 8;
    Bitboard kanatMaskesi = (sahDosya < 4) ? (DOSYA_A | DOSYA_B | DOSYA_C) : (DOSYA_F | DOSYA_G | DOSYA_H);
    if (!(tahta.taslariBul(TasTuru::PIYON) & kanatMaskesi))
        skor -= PiyonsuzKanat;

    if (T == Izleme::IZLEME_VAR)
        Iz::ekle(TERIM_SAYISI - 1, Biz, skor);

    return skor;
  }

  // Tehdit değerlendirmesi
  template<Izleme T> template<Renk Biz>
  int Degerlendirme<T>::tehditler() const {
    constexpr Renk Onlar = (Biz == Renk::BEYAZ ? Renk::SIYAH : Renk::BEYAZ);
    constexpr int Ileri = (Biz == Renk::BEYAZ ? 8 : -8);
    constexpr Bitboard Sira3 = (Biz == Renk::BEYAZ ? SATIR_3 : SATIR_6);

    int skor = 0;

    // Piyon olmayan düşman taşları
    Bitboard piyonOlmayanDusmanlar = tahta.taslariBul(Onlar) & ~tahta.taslariBul(Onlar, TasTuru::PIYON);

    // Güçlü korunan kareler
    Bitboard gucluKorunan = saldiranlar[static_cast<int>(Onlar)][static_cast<int>(TasTuru::PIYON)]
                          | (ciftSaldiri[static_cast<int>(Onlar)] & ~ciftSaldiri[static_cast<int>(Biz)]);

    // Korunan düşmanlar
    Bitboard korunan = piyonOlmayanDusmanlar & gucluKorunan;

    // Zayıf düşmanlar
    Bitboard zayif = tahta.taslariBul(Onlar) & ~gucluKorunan & saldiranlar[static_cast<int>(Biz)][6];

    // Tehdit bonusları
    if (korunan | zayif) {
        Bitboard b = (korunan | zayif) & (saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::AT)] | saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::FIL)]);
        while (b) {
            int kare = BitboardYardimci::lsb(b);
            b &= b - 1;
            TasTuru hedefTas = tahta.karedekiTasTuru(kare);
            if (hedefTas != TasTuru::YOK)
                skor += KucukTasTehdit[static_cast<int>(hedefTas)];
        }

        b = zayif & saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::KALE)];
        while (b) {
            int kare = BitboardYardimci::lsb(b);
            b &= b - 1;
            TasTuru hedefTas = tahta.karedekiTasTuru(kare);
            if (hedefTas != TasTuru::YOK)
                skor += KaleTehdit[static_cast<int>(hedefTas)];
        }

        if (zayif & saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::SAH)])
            skor += SahTehdit;

        b = ~saldiranlar[static_cast<int>(Onlar)][6] | piyonOlmayanDusmanlar & ciftSaldiri[static_cast<int>(Biz)];
        skor += Asili * BitboardYardimci::popcount(zayif & b);
    }

    // Kısıtlı taş bonusu
    Bitboard b = saldiranlar[static_cast<int>(Onlar)][6] & ~gucluKorunan & saldiranlar[static_cast<int>(Biz)][6];
    skor += KisitliTas * BitboardYardimci::popcount(b);

    // Güvenli piyon tehditleri
    Bitboard guvenli = ~saldiranlar[static_cast<int>(Onlar)][6] | saldiranlar[static_cast<int>(Biz)][6];
    b = tahta.taslariBul(Biz, TasTuru::PIYON) & guvenli;
    
    // Piyon saldırıları
    Bitboard piyonSaldiri = 0;
    if (Biz == Renk::BEYAZ) {
        piyonSaldiri = ((b & ~DOSYA_A) << 7) | ((b & ~DOSYA_H) << 9);
    } else {
        piyonSaldiri = ((b & ~DOSYA_H) >> 7) | ((b & ~DOSYA_A) >> 9);
    }
    
    skor += GuvenliPiyonTehdit * BitboardYardimci::popcount(piyonSaldiri & piyonOlmayanDusmanlar);

    // Piyon itme tehditleri
    b = (tahta.taslariBul(Biz, TasTuru::PIYON) << Ileri) & ~tahta.tumTaslar();
    b |= ((b & Sira3) << Ileri) & ~tahta.tumTaslar();
    b &= ~saldiranlar[static_cast<int>(Onlar)][static_cast<int>(TasTuru::PIYON)] & guvenli;
    
    // İtilen piyonların saldırıları
    if (Biz == Renk::BEYAZ) {
        piyonSaldiri = ((b & ~DOSYA_A) << 7) | ((b & ~DOSYA_H) << 9);
    } else {
        piyonSaldiri = ((b & ~DOSYA_H) >> 7) | ((b & ~DOSYA_A) >> 9);
    }
    
    skor += PiyonItmeTehdit * BitboardYardimci::popcount(piyonSaldiri & piyonOlmayanDusmanlar);

    // Vezir tehditleri
    if (BitboardYardimci::popcount(tahta.taslariBul(Onlar, TasTuru::VEZIR)) == 1) {
        int vezirKare = BitboardYardimci::lsb(tahta.taslariBul(Onlar, TasTuru::VEZIR));
        guvenli = mobiliteAlani[static_cast<int>(Biz)] & ~gucluKorunan;

        b = saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::AT)] & BitboardYardimci::atHamlesi[vezirKare];
        skor += AtVezirUzerinde * BitboardYardimci::popcount(b & guvenli);

        b = (saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::FIL)] & BitboardYardimci::filSaldirisi(vezirKare, tahta.tumTaslar()))
          | (saldiranlar[static_cast<int>(Biz)][static_cast<int>(TasTuru::KALE)] & BitboardYardimci::kaleSaldirisi(vezirKare, tahta.tumTaslar()));
        skor += KayanVezirUzerinde * BitboardYardimci::popcount(b & guvenli & ciftSaldiri[static_cast<int>(Biz)]);
    }

    if (T == Izleme::IZLEME_VAR)
        Iz::ekle(TEHDIT, Biz, skor);

    return skor;
  }

  // Geçer piyon değerlendirmesi
  template<Izleme T> template<Renk Biz>
  int Degerlendirme<T>::gecerler() const {
    constexpr Renk Onlar = (Biz == Renk::BEYAZ ? Renk::SIYAH : Renk::BEYAZ);
    constexpr int Ileri = (Biz == Renk::BEYAZ ? 8 : -8);

    int skor = 0;

    // Basit geçer piyon tespiti
    Bitboard bizimPiyonlar = tahta.taslariBul(Biz, TasTuru::PIYON);
    Bitboard onlarinPiyonlari = tahta.taslariBul(Onlar, TasTuru::PIYON);

    while (bizimPiyonlar) {
        int kare = BitboardYardimci::lsb(bizimPiyonlar);
        bizimPiyonlar &= bizimPiyonlar - 1;

        int sira = satirIndeksi(kare);
        int dosya = sutunIndeksi(kare);

        // Geçer piyon kontrolü
        bool gecer = true;
        
        // Önündeki kareleri kontrol et
        if (Biz == Renk::BEYAZ) {
            for (int s = sira + 1; s < 8; s++) {
                int kontrol = kareIndeksi(s, dosya);
                // Karşı piyon var mı?
                if (onlarinPiyonlari & (1ULL << kontrol)) {
                    gecer = false;
                    break;
                }
                // Yan dosyalarda karşı piyon var mı?
                if (dosya > 0 && (onlarinPiyonlari & (1ULL << kareIndeksi(s, dosya - 1)))) {
                    gecer = false;
                    break;
                }
                if (dosya < 7 && (onlarinPiyonlari & (1ULL << kareIndeksi(s, dosya + 1)))) {
                    gecer = false;
                    break;
                }
            }
        } else {
            for (int s = sira - 1; s >= 0; s--) {
                int kontrol = kareIndeksi(s, dosya);
                if (onlarinPiyonlari & (1ULL << kontrol)) {
                    gecer = false;
                    break;
                }
                if (dosya > 0 && (onlarinPiyonlari & (1ULL << kareIndeksi(s, dosya - 1)))) {
                    gecer = false;
                    break;
                }
                if (dosya < 7 && (onlarinPiyonlari & (1ULL << kareIndeksi(s, dosya + 1)))) {
                    gecer = false;
                    break;
                }
            }
        }

        if (gecer) {
            int gecerSira = (Biz == Renk::BEYAZ) ? sira : 7 - sira;
            int bonus = GecerPiyonBonus[gecerSira];

            // Şah yakınlığı
            if (gecerSira > 2) {
                int bizimSahKare = tahta.sahBul(Biz);
                int onlarinSahKare = tahta.sahBul(Onlar);
                int blokKare = kare + Ileri;

                int w = 5 * gecerSira - 13;
                int onlarMesafe = std::max(std::abs(satirIndeksi(onlarinSahKare) - satirIndeksi(blokKare)),
                                          std::abs(sutunIndeksi(onlarinSahKare) - sutunIndeksi(blokKare)));
                int bizimMesafe = std::max(std::abs(satirIndeksi(bizimSahKare) - satirIndeksi(blokKare)),
                                          std::abs(sutunIndeksi(bizimSahKare) - sutunIndeksi(blokKare)));
                
                bonus += (onlarMesafe * 19 / 4 - bizimMesafe * 2) * w / 100;
            }

            skor += bonus - GecerDosya * (dosya < 4 ? dosya : 7 - dosya);
        }
    }

    if (T == Izleme::IZLEME_VAR)
        Iz::ekle(GECER_PIYON, Biz, skor);

    return skor;
  }

  // Alan değerlendirmesi
  template<Izleme T> template<Renk Biz>
  int Degerlendirme<T>::alan() const {
    if (materyalDengesizligi < AlanEsigi / 100)
        return 0;

    constexpr Renk Onlar = (Biz == Renk::BEYAZ ? Renk::SIYAH : Renk::BEYAZ);
    constexpr int Geri = (Biz == Renk::BEYAZ ? -8 : 8);
    constexpr Bitboard AlanMaskesi = 
        Biz == Renk::BEYAZ ? (DOSYA_C | DOSYA_D | DOSYA_E | DOSYA_F) & (SATIR_2 | SATIR_3 | SATIR_4)
                           : (DOSYA_C | DOSYA_D | DOSYA_E | DOSYA_F) & (SATIR_7 | SATIR_6 | SATIR_5);

    // Güvenli kareler
    Bitboard guvenli = AlanMaskesi
                     & ~tahta.taslariBul(Biz, TasTuru::PIYON)
                     & ~saldiranlar[static_cast<int>(Onlar)][static_cast<int>(TasTuru::PIYON)];

    // Arkadaki kareler
    Bitboard arkada = tahta.taslariBul(Biz, TasTuru::PIYON);
    arkada |= (arkada >> Geri);
    arkada |= ((arkada >> Geri) >> Geri);

    int bonus = BitboardYardimci::popcount(guvenli) + BitboardYardimci::popcount(arkada & guvenli & ~saldiranlar[static_cast<int>(Onlar)][6]);
    int agirlik = BitboardYardimci::popcount(tahta.taslariBul(Biz)) - 1;
    int skor = bonus * agirlik * agirlik / 16;

    if (T == Izleme::IZLEME_VAR)
        Iz::ekle(ALAN, Biz, skor);

    return skor;
  }

  // İnisiyatif değerlendirmesi
  template<Izleme T>
  int Degerlendirme<T>::inisiyatif(int skor) const {
    int ortaOyunDegeri = skor;
    int sonOyunDegeri = skor;

    int disKanatMesafe = std::abs(sutunIndeksi(tahta.sahBul(Renk::BEYAZ)) - sutunIndeksi(tahta.sahBul(Renk::SIYAH)))
                       - std::abs(satirIndeksi(tahta.sahBul(Renk::BEYAZ)) - satirIndeksi(tahta.sahBul(Renk::SIYAH)));

    bool sizinti = satirIndeksi(tahta.sahBul(Renk::BEYAZ)) > 3
                 || satirIndeksi(tahta.sahBul(Renk::SIYAH)) < 4;

    bool ikiKanattaPiyon = (tahta.taslariBul(TasTuru::PIYON) & (DOSYA_A | DOSYA_B | DOSYA_C))
                        && (tahta.taslariBul(TasTuru::PIYON) & (DOSYA_F | DOSYA_G | DOSYA_H));

    // Basit geçer piyon sayısı
    int gecerSayisi = 0;
    Bitboard beyazPiyonlar = tahta.taslariBul(Renk::BEYAZ, TasTuru::PIYON);
    Bitboard siyahPiyonlar = tahta.taslariBul(Renk::SIYAH, TasTuru::PIYON);
    
    while (beyazPiyonlar) {
        int kare = BitboardYardimci::lsb(beyazPiyonlar);
        beyazPiyonlar &= beyazPiyonlar - 1;
        
        bool gecer = true;
        int dosya = sutunIndeksi(kare);
        int sira = satirIndeksi(kare);
        
        for (int s = sira + 1; s < 8; s++) {
            if (siyahPiyonlar & (1ULL << kareIndeksi(s, dosya))) {
                gecer = false;
                break;
            }
        }
        if (gecer) gecerSayisi++;
    }

    bool neredeyseKazanilmaz = gecerSayisi == 0 && disKanatMesafe < 0 && !ikiKanattaPiyon;

    // Karmaşıklık hesabı
    int karmasiklik = 9 * gecerSayisi
                    + 11 * BitboardYardimci::popcount(tahta.taslariBul(TasTuru::PIYON))
                    + 9 * disKanatMesafe
                    + 12 * (sizinti ? 1 : 0)
                    + 21 * (ikiKanattaPiyon ? 1 : 0)
                    + 51 * (materyalDengesizligi == 0 ? 1 : 0)
                    - 43 * (neredeyseKazanilmaz ? 1 : 0)
                    - 100;

    // Bonus uygula
    int u = ((ortaOyunDegeri > 0) - (ortaOyunDegeri < 0)) * std::max(std::min(karmasiklik + 50, 0), -std::abs(ortaOyunDegeri));
    int v = ((sonOyunDegeri > 0) - (sonOyunDegeri < 0)) * std::max(karmasiklik, -std::abs(sonOyunDegeri));

    if (T == Izleme::IZLEME_VAR)
        Iz::ekle(INISIYATIF, u, v);

    return (u * oyunEvresi + v * (256 - oyunEvresi)) / 256;
  }

  // Ana değerlendirme fonksiyonu
  template<Izleme T>
  int Degerlendirme<T>::deger() {
    // Materyal dengesi
    materyalDengesizligi = 0;
    for (int tas = 0; tas < 6; tas++) {
        int beyaz = BitboardYardimci::popcount(tahta.taslariBul(Renk::BEYAZ, static_cast<TasTuru>(tas)));
        int siyah = BitboardYardimci::popcount(tahta.taslariBul(Renk::SIYAH, static_cast<TasTuru>(tas)));
        materyalDengesizligi += (beyaz - siyah) * Degerlendirici::ORTA_OYUN_TAS_DEGERLERI[tas];
    }

    // Basit piyon yapısı
    piyonYapisiSkoru[0] = piyonYapisiSkoru[1] = 0;

    // Oyun evresi (0-256)
    int toplamMateryal = 0;
    for (int tas = 1; tas < 5; tas++) { // At, fil, kale, vezir
        toplamMateryal += BitboardYardimci::popcount(tahta.taslariBul(static_cast<TasTuru>(tas))) * Degerlendirici::ORTA_OYUN_TAS_DEGERLERI[tas];
    }
    oyunEvresi = std::min(256, toplamMateryal / 32);

    // Pozisyon değerlendirme skoru
    int skor = materyalDengesizligi;

    // Tembel değerlendirme
    if (std::abs(skor) > TembelEsik)
        return tahta.siraKimde() == Renk::BEYAZ ? skor : -skor;

    // Detaylı değerlendirme
    baslat<Renk::BEYAZ>();
    baslat<Renk::SIYAH>();

    // Taşları değerlendir
    skor += taslar<Renk::BEYAZ, TasTuru::AT>() - taslar<Renk::SIYAH, TasTuru::AT>();
    skor += taslar<Renk::BEYAZ, TasTuru::FIL>() - taslar<Renk::SIYAH, TasTuru::FIL>();
    skor += taslar<Renk::BEYAZ, TasTuru::KALE>() - taslar<Renk::SIYAH, TasTuru::KALE>();
    skor += taslar<Renk::BEYAZ, TasTuru::VEZIR>() - taslar<Renk::SIYAH, TasTuru::VEZIR>();

    skor += mobilite[0] - mobilite[1];

    skor += sah<Renk::BEYAZ>() - sah<Renk::SIYAH>();
    skor += tehditler<Renk::BEYAZ>() - tehditler<Renk::SIYAH>();
    skor += gecerler<Renk::BEYAZ>() - gecerler<Renk::SIYAH>();
    skor += alan<Renk::BEYAZ>() - alan<Renk::SIYAH>();

    skor += inisiyatif(skor);

    // Tempo bonusu
    skor += 10;

    if (T == Izleme::IZLEME_VAR) {
        Iz::ekle(MATERYAL, materyalDengesizligi);
        Iz::ekle(DENGESIZLIK, 0);
        Iz::ekle(MOBILITE, mobilite[0], mobilite[1]);
        Iz::ekle(TOPLAM, skor);
    }

    return tahta.siraKimde() == Renk::BEYAZ ? skor : -skor;
  }

} // namespace

// Tablo başlatma
bool Degerlendirici::tablolarBaslatildi = false;
std::array<std::array<int, 64>, 6> Degerlendirici::ortaOyunPST[2];
std::array<std::array<int, 64>, 6> Degerlendirici::sonOyunPST[2];
std::array<int, 64> Degerlendirici::gecerPiyonBonus;
std::array<int, 64> Degerlendirici::izolePiyonCeza;
std::array<int, 64> Degerlendirici::geriPiyonCeza;
std::array<int, 64> Degerlendirici::ciftPiyonCeza;

void Degerlendirici::tablolariBaslat() {
    if (tablolarBaslatildi) return;

    // Basit PST değerleri
    for (int renk = 0; renk < 2; renk++) {
        for (int tas = 0; tas < 6; tas++) {
            for (int kare = 0; kare < 64; kare++) {
                ortaOyunPST[renk][tas][kare] = 0;
                sonOyunPST[renk][tas][kare] = 0;
            }
        }
    }

    // Piyon PST
    int piyonOrtaOyun[64] = {
         0,  0,  0,  0,  0,  0,  0,  0,
         5, 10, 10,-20,-20, 10, 10,  5,
         5, -5,-10,  0,  0,-10, -5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5,  5, 10, 25, 25, 10,  5,  5,
        10, 10, 20, 30, 30, 20, 10, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
         0,  0,  0,  0,  0,  0,  0,  0
    };

    // At PST - Stockfish değerleri
    int atOrtaOyun[64] = {
        -167, -89, -34, -49,  61, -97, -15, -107,
         -73, -41,  72,  36,  23,  62,   7,  -17,
         -47,  60,  37,  65,  84, 129,  73,   44,
          -9,  17,  19,  53,  37,  69,  18,   22,
         -13,   4,  16,  13,  28,  19,  21,   -8,
         -23,  -9,  12,  10,  19,  17,  25,  -16,
         -29, -53, -12,  -3,  -1,  18, -14,  -19,
        -105, -21, -58, -33, -17, -28, -19,  -23
    };
    
    // Fil PST - Stockfish değerleri
    int filOrtaOyun[64] = {
        -29,   4, -82, -37, -25, -42,   7,  -8,
        -26,  16, -18, -13,  30,  59,  18, -47,
        -16,  37,  43,  40,  35,  50,  37,  -2,
         -4,   5,  19,  50,  37,  37,   7,  -2,
         -6,  13,  13,  26,  34,  12,  10,   4,
          0,  15,  15,  15,  14,  27,  18,  10,
          4,  15,  16,   0,   7,  21,  33,   1,
        -33,  -3, -14, -21, -13, -12, -39, -21
    };
    
    // Kale PST - Stockfish değerleri
    int kaleOrtaOyun[64] = {
         32,  42,  32,  51, 63,  9,  31,  43,
         27,  32,  58,  62, 80, 67,  26,  44,
         -5,  19,  26,  36, 17, 45,  61,  16,
        -24, -11,   7,  26, 24, 35,  -8, -20,
        -36, -26, -12,  -1,  9, -7,   6, -23,
        -45, -25, -16, -17,  3,  0,  -5, -33,
        -44, -16, -20,  -9, -1, 11,  -6, -71,
        -19, -13,   1,  17, 16,  7, -37, -26
    };
    
    // Vezir PST - Stockfish değerleri
    int vezirOrtaOyun[64] = {
        -28,   0,  29,  12,  59,  44,  43,  45,
        -24, -39,  -5,   1, -16,  57,  28,  54,
        -13, -17,   7,   8,  29,  56,  47,  57,
        -27, -27, -16, -16,  -1,  17,  -2,   1,
         -9, -26,  -9, -10,  -2,  -4,   3,  -3,
        -14,   2, -11,  -2,  -5,   2,  14,   5,
        -35,  -8,  11,   2,   8,  15,  -3,   1,
         -1, -18,  -9,  10, -15, -25, -31, -50
    };

    // PST'leri ayarla
    for (int kare = 0; kare < 64; kare++) {
        // Beyaz için
        ortaOyunPST[0][0][kare] = piyonOrtaOyun[kare];
        ortaOyunPST[0][1][kare] = atOrtaOyun[kare];
        ortaOyunPST[0][2][kare] = filOrtaOyun[kare];
        ortaOyunPST[0][3][kare] = kaleOrtaOyun[kare];
        ortaOyunPST[0][4][kare] = vezirOrtaOyun[kare];
        ortaOyunPST[0][5][kare] = 0; // Şah
        
        // Siyah için (ters çevir)
        int tersKare = (7 - kare / 8) * 8 + (kare % 8);
        ortaOyunPST[1][0][tersKare] = piyonOrtaOyun[kare];
        ortaOyunPST[1][1][tersKare] = atOrtaOyun[kare];
        ortaOyunPST[1][2][tersKare] = filOrtaOyun[kare];
        ortaOyunPST[1][3][tersKare] = kaleOrtaOyun[kare];
        ortaOyunPST[1][4][tersKare] = vezirOrtaOyun[kare];
        ortaOyunPST[1][5][tersKare] = 0; // Şah
    }

    // Piyon yapısı tabloları
    for (int kare = 0; kare < 64; kare++) {
        int sira = kare / 8;
        int dosya = kare % 8;
        
        // Geçer piyon bonusu
        gecerPiyonBonus[kare] = 10 + sira * 10;
        
        // İzole piyon cezası
        izolePiyonCeza[kare] = -10 - (3 - std::abs(dosya - 3)) * 5;
        
        // Geri piyon cezası
        geriPiyonCeza[kare] = -8;
        
        // Çift piyon cezası
        ciftPiyonCeza[kare] = -12;
    }

    tablolarBaslatildi = true;
}

// Ana değerlendirme fonksiyonu
int Degerlendirici::degerlendir(const Tahta& tahta) {
    return Degerlendirme<Izleme::IZLEME_YOK>(tahta).deger();
}

// Materyal değerlendirme
int Degerlendirici::materyalDegerlendir(const Tahta& tahta) {
    int deger = 0;
    
    for (int tas = 0; tas < 6; tas++) {
        int beyazSayisi = BitboardYardimci::popcount(tahta.taslariBul(Renk::BEYAZ, static_cast<TasTuru>(tas)));
        int siyahSayisi = BitboardYardimci::popcount(tahta.taslariBul(Renk::SIYAH, static_cast<TasTuru>(tas)));
        deger += (beyazSayisi - siyahSayisi) * ORTA_OYUN_TAS_DEGERLERI[tas];
    }
    
    return tahta.siraKimde() == Renk::BEYAZ ? deger : -deger;
}

// Diğer yardımcı fonksiyonlar
int Degerlendirici::pozisyonelDegerlendir(const Tahta& tahta) {
    int deger = 0;
    
    // Piece-square table değerleri
    for (int kare = 0; kare < 64; kare++) {
        TasTuru tas = tahta.tasAl(kare);
        if (tas == TasTuru::YOK) continue;
        
        Renk renk = tahta.renkAl(kare);
        int pstDeger = ortaOyunPST[static_cast<int>(renk)][static_cast<int>(tas)][kare];
        
        // Beyaz için pozitif, siyah için negatif
        deger += (renk == Renk::BEYAZ) ? pstDeger : -pstDeger;
    }
    
    return tahta.siraKimde() == Renk::BEYAZ ? deger : -deger;
}

int Degerlendirici::piyonYapisiDegerlendir(const Tahta& tahta) {
    return 0; // Basit implementasyon
}

int Degerlendirici::mobiliteDeğerlendir(const Tahta& tahta) {
    return 0; // Basit implementasyon
}

int Degerlendirici::sahGuvenligiDegerlendir(const Tahta& tahta) {
    return 0; // Basit implementasyon
}

int Degerlendirici::merkezKontroluDegerlendir(const Tahta& tahta) {
    return 0; // Basit implementasyon
}

int Degerlendirici::filCiftiBonus(const Tahta& tahta, Renk renk) {
    return BitboardYardimci::popcount(tahta.taslariBul(renk, TasTuru::FIL)) >= 2 ? FIL_CIFTI_BONUS : 0;
}

int Degerlendirici::kaleAktivitesi(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

int Degerlendirici::atPozisyonu(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

int Degerlendirici::vezirAktivitesi(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

// Piyon yapısı analizi
Bitboard Degerlendirici::gecerPiyonlar(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

Bitboard Degerlendirici::izolePiyonlar(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

Bitboard Degerlendirici::geriPiyonlar(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

Bitboard Degerlendirici::ciftPiyonlar(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

Bitboard Degerlendirici::zayifPiyonlar(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

// Oyun evresi
OyunEvresi Degerlendirici::oyunEvresiBelirle(const Tahta& tahta) {
    int toplamMateryal = 0;
    
    for (int tas = 1; tas < 6; tas++) { // Piyonlar hariç
        toplamMateryal += BitboardYardimci::popcount(tahta.taslariBul(static_cast<TasTuru>(tas))) * ORTA_OYUN_TAS_DEGERLERI[tas];
    }
    
    if (toplamMateryal > 6000) return OyunEvresi::ACILIS;
    if (toplamMateryal > 3000) return OyunEvresi::ORTA_OYUN;
    return OyunEvresi::SON_OYUN;
}

int Degerlendirici::evreKatsayisi(const Tahta& tahta) {
    int toplamMateryal = 0;
    
    for (int tas = 1; tas < 5; tas++) { // At, fil, kale, vezir
        toplamMateryal += BitboardYardimci::popcount(tahta.taslariBul(static_cast<TasTuru>(tas))) * ORTA_OYUN_TAS_DEGERLERI[tas];
    }
    
    return std::min(256, toplamMateryal / 32);
}

// Özel durumlar
int Degerlendirici::sonOyunDegerlendirmesi(const Tahta& tahta) {
    return 0; // Basit implementasyon
}

bool Degerlendirici::yetersizMateryal(const Tahta& tahta) {
    // Sadece şahlar kaldıysa
    if (BitboardYardimci::popcount(tahta.tumTaslar()) == 2) return true;
    
    // Şah ve at veya şah ve fil
    if (BitboardYardimci::popcount(tahta.tumTaslar()) == 3) {
        if (tahta.taslariBul(TasTuru::AT) || tahta.taslariBul(TasTuru::FIL))
            return true;
    }
    
    return false;
}

int Degerlendirici::drawishFactor(const Tahta& tahta) {
    return 0; // Basit implementasyon
}

// PST erişimi
int Degerlendirici::pieceSquareValue(TasTuru tas, Renk renk, int kare, OyunEvresi evre) {
    if (evre == OyunEvresi::ORTA_OYUN || evre == OyunEvresi::ACILIS)
        return ortaOyunPST[static_cast<int>(renk)][static_cast<int>(tas)][kare];
    else
        return sonOyunPST[static_cast<int>(renk)][static_cast<int>(tas)][kare];
}

// Mobilite
int Degerlendirici::tasMobilitesi(const Tahta& tahta, int kare, TasTuru tas, Renk renk) {
    return 0; // Basit implementasyon
}

// Tehdit değerlendirmesi
int Degerlendirici::tehditDegerlendir(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

// Şah güvenliği
int Degerlendirici::sahKalkaniBonusu(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

int Degerlendirici::sahSaldiriTehdidi(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

// Açık hatlar
Bitboard Degerlendirici::acikSutunlar(const Tahta& tahta) {
    Bitboard acik = 0;
    Bitboard piyonlar = tahta.taslariBul(TasTuru::PIYON);
    
    for (int dosya = 0; dosya < 8; dosya++) {
        Bitboard dosyaMaske = DOSYA_A << dosya;
        if (!(piyonlar & dosyaMaske))
            acik |= dosyaMaske;
    }
    
    return acik;
}

Bitboard Degerlendirici::yariAcikSutunlar(const Tahta& tahta, Renk renk) {
    Bitboard yariAcik = 0;
    Bitboard bizimPiyonlar = tahta.taslariBul(renk, TasTuru::PIYON);
    Bitboard onlarinPiyonlari = tahta.taslariBul(tersRenk(renk), TasTuru::PIYON);
    
    for (int dosya = 0; dosya < 8; dosya++) {
        Bitboard dosyaMaske = DOSYA_A << dosya;
        if (!(bizimPiyonlar & dosyaMaske) && (onlarinPiyonlari & dosyaMaske))
            yariAcik |= dosyaMaske;
    }
    
    return yariAcik;
}

int Degerlendirici::acikHatKontrolu(const Tahta& tahta, Renk renk) {
    return 0; // Basit implementasyon
}

// İnisiyatif
int Degerlendirici::inisiyatifDegerlendir(const Tahta& tahta) {
    return 0; // Basit implementasyon
}

// Özel yardımcı fonksiyonlar
int Degerlendirici::interpolate(int ortaOyunDeger, int sonOyunDeger, int evreKatsayisi) {
    return (ortaOyunDeger * evreKatsayisi + sonOyunDeger * (256 - evreKatsayisi)) / 256;
}

Bitboard Degerlendirici::onPiyonlar(const Tahta& tahta, Renk renk, int kare) {
    return 0; // Basit implementasyon
}

Bitboard Degerlendirici::arkaPiyonlar(const Tahta& tahta, Renk renk, int kare) {
    return 0; // Basit implementasyon
}