#include "HamleUretici.h"
#include "Tahta.h"
#include "BitboardYardimci.h"
#include <algorithm>

// Statik değişken tanımlamaları
bool HamleUretici::tablolarHazir = false;
HamleUretici::MagicEntry HamleUretici::kaleMagic[64];
HamleUretici::MagicEntry HamleUretici::filMagic[64];
Bitboard HamleUretici::kaleSaldiriTablosu[102400];
Bitboard HamleUretici::filSaldiriTablosu[5248];

// Tüm hamleleri üret
void HamleUretici::tumHamleleriUret(const Tahta& tahta, HamleListesi& hamleler) {
    hamleler.temizle();
    
    Renk sira = tahta.getSira();
    
    // Her taş türü için hamle üret
    piyonHamleleriUret(tahta, hamleler, sira);
    atHamleleriUret(tahta, hamleler, sira);
    filHamleleriUret(tahta, hamleler, sira);
    kaleHamleleriUret(tahta, hamleler, sira);
    vezirHamleleriUret(tahta, hamleler, sira);
    sahHamleleriUret(tahta, hamleler, sira);
    
    // Özel hamleler
    rokHamleleriUret(tahta, hamleler, sira);
    enPassantHamleleriUret(tahta, hamleler, sira);
}

// Piyon hamleleri üret
void HamleUretici::piyonHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk) {
    Bitboard piyonlar = tahta.getBitboard(renk, TasTuru::PIYON);
    Bitboard tumTaslar = tahta.getTumTaslar();
    Bitboard rakipTaslar = tahta.getTumTaslar(tersRenk(renk));
    
    int ileriYon = (renk == Renk::BEYAZ) ? 8 : -8;
    int baslangicSatir = (renk == Renk::BEYAZ) ? 1 : 6;
    int terfiSatir = (renk == Renk::BEYAZ) ? 6 : 1;
    
    while (piyonlar) {
        int kaynak = BitboardYardimci::enDusukBitIndeksi(piyonlar);
        int satir = satirIndeksi(kaynak);
        
        // Tek kare ileri
        int hedef = kaynak + ileriYon;
        if (hedef >= 0 && hedef < 64 && !BitboardYardimci::bitVar(tumTaslar, hedef)) {
            if (satir == terfiSatir) {
                // Terfi hamleleri
                terfiHamleleriEkle(hamleler, kaynak, hedef, HamleTuru::TERFI);
            } else {
                hamleler.ekle(kaynak, hedef, TasTuru::PIYON, HamleTuru::NORMAL);
            }
            
            // İki kare ileri (başlangıç pozisyonundan)
            if (satir == baslangicSatir) {
                int ikiKareHedef = kaynak + 2 * ileriYon;
                if (!BitboardYardimci::bitVar(tumTaslar, ikiKareHedef)) {
                    hamleler.ekle(kaynak, ikiKareHedef, TasTuru::PIYON, HamleTuru::IKI_KARE);
                }
            }
        }
        
        // Saldırı hamleleri
        Bitboard saldirilar = BitboardYardimci::PIYON_SALDIRILARI[static_cast<int>(renk)][kaynak];
        Bitboard saldiriHedefleri = saldirilar & rakipTaslar;
        
        while (saldiriHedefleri) {
            int saldiriHedef = BitboardYardimci::enDusukBitIndeksi(saldiriHedefleri);
            TasTuru alinanTas = tahta.tasAl(saldiriHedef);
            
            if (satir == terfiSatir) {
                // Terfi alma hamleleri
                Hamle terfiHamle(kaynak, saldiriHedef, TasTuru::PIYON, alinanTas, HamleTuru::TERFI_ALMA);
                terfiHamle.terfiTasi = TasTuru::VEZIR;
                hamleler.ekle(terfiHamle);
                
                terfiHamle.terfiTasi = TasTuru::KALE;
                hamleler.ekle(terfiHamle);
                
                terfiHamle.terfiTasi = TasTuru::FIL;
                hamleler.ekle(terfiHamle);
                
                terfiHamle.terfiTasi = TasTuru::AT;
                hamleler.ekle(terfiHamle);
            } else {
                hamleler.ekle(Hamle(kaynak, saldiriHedef, TasTuru::PIYON, alinanTas, HamleTuru::ALMA));
            }
        }
    }
}

// At hamleleri üret
void HamleUretici::atHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk) {
    Bitboard atlar = tahta.getBitboard(renk, TasTuru::AT);
    Bitboard kendiTaslar = tahta.getTumTaslar(renk);
    
    while (atlar) {
        int kaynak = BitboardYardimci::enDusukBitIndeksi(atlar);
        Bitboard hedefler = BitboardYardimci::AT_HAMLELERI[kaynak] & ~kendiTaslar;
        
        while (hedefler) {
            int hedef = BitboardYardimci::enDusukBitIndeksi(hedefler);
            TasTuru alinanTas = tahta.tasAl(hedef);
            
            if (alinanTas != TasTuru::YOK) {
                hamleler.ekle(Hamle(kaynak, hedef, TasTuru::AT, alinanTas, HamleTuru::ALMA));
            } else {
                hamleler.ekle(kaynak, hedef, TasTuru::AT, HamleTuru::NORMAL);
            }
        }
    }
}

// Fil hamleleri üret
void HamleUretici::filHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk) {
    Bitboard filler = tahta.getBitboard(renk, TasTuru::FIL);
    Bitboard tumTaslar = tahta.getTumTaslar();
    Bitboard kendiTaslar = tahta.getTumTaslar(renk);
    
    while (filler) {
        int kaynak = BitboardYardimci::enDusukBitIndeksi(filler);
        Bitboard saldirilar = BitboardYardimci::filSaldirilari(kaynak, tumTaslar);
        Bitboard hedefler = saldirilar & ~kendiTaslar;
        
        isinHamleleriUret(tahta, hamleler, kaynak, hedefler, TasTuru::FIL);
    }
}

// Kale hamleleri üret
void HamleUretici::kaleHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk) {
    Bitboard kaleler = tahta.getBitboard(renk, TasTuru::KALE);
    Bitboard tumTaslar = tahta.getTumTaslar();
    Bitboard kendiTaslar = tahta.getTumTaslar(renk);
    
    while (kaleler) {
        int kaynak = BitboardYardimci::enDusukBitIndeksi(kaleler);
        Bitboard saldirilar = BitboardYardimci::kaleSaldirilari(kaynak, tumTaslar);
        Bitboard hedefler = saldirilar & ~kendiTaslar;
        
        isinHamleleriUret(tahta, hamleler, kaynak, hedefler, TasTuru::KALE);
    }
}

// Vezir hamleleri üret
void HamleUretici::vezirHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk) {
    Bitboard vezirler = tahta.getBitboard(renk, TasTuru::VEZIR);
    Bitboard tumTaslar = tahta.getTumTaslar();
    Bitboard kendiTaslar = tahta.getTumTaslar(renk);
    
    while (vezirler) {
        int kaynak = BitboardYardimci::enDusukBitIndeksi(vezirler);
        Bitboard saldirilar = BitboardYardimci::vezirSaldirilari(kaynak, tumTaslar);
        Bitboard hedefler = saldirilar & ~kendiTaslar;
        
        isinHamleleriUret(tahta, hamleler, kaynak, hedefler, TasTuru::VEZIR);
    }
}

// Şah hamleleri üret
void HamleUretici::sahHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk) {
    Bitboard sah = tahta.getBitboard(renk, TasTuru::SAH);
    int kaynak = BitboardYardimci::enDusukBitIndeksi(sah);
    
    Bitboard kendiTaslar = tahta.getTumTaslar(renk);
    Bitboard hedefler = BitboardYardimci::SAH_HAMLELERI[kaynak] & ~kendiTaslar;
    
    while (hedefler) {
        int hedef = BitboardYardimci::enDusukBitIndeksi(hedefler);
        TasTuru alinanTas = tahta.tasAl(hedef);
        
        if (alinanTas != TasTuru::YOK) {
            hamleler.ekle(Hamle(kaynak, hedef, TasTuru::SAH, alinanTas, HamleTuru::ALMA));
        } else {
            hamleler.ekle(kaynak, hedef, TasTuru::SAH, HamleTuru::NORMAL);
        }
    }
}

// Rok hamleleri üret
void HamleUretici::rokHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk) {
    // Kısa rok
    if (tahta.kisaRokYapabilirMi(renk)) {
        int sahKare = (renk == Renk::BEYAZ) ? E1 : E8;
        int hedefKare = sahKare + 2;
        hamleler.ekle(sahKare, hedefKare, TasTuru::SAH, HamleTuru::KISA_ROK);
    }
    
    // Uzun rok
    if (tahta.uzunRokYapabilirMi(renk)) {
        int sahKare = (renk == Renk::BEYAZ) ? E1 : E8;
        int hedefKare = sahKare - 2;
        hamleler.ekle(sahKare, hedefKare, TasTuru::SAH, HamleTuru::UZUN_ROK);
    }
}

// En passant hamleleri üret
void HamleUretici::enPassantHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, Renk renk) {
    int epKare = tahta.getEnPassantKare();
    if (epKare == -1) return;
    
    int epSatir = satirIndeksi(epKare);
    int epSutun = sutunIndeksi(epKare);
    int piyonSatir = (renk == Renk::BEYAZ) ? 4 : 3;
    
    // En passant yapabilecek piyonları kontrol et
    if ((renk == Renk::BEYAZ && epSatir == 5) || (renk == Renk::SIYAH && epSatir == 2)) {
        // Sol piyon
        if (epSutun > 0) {
            int solPiyonKare = kareIndeksi(piyonSatir, epSutun - 1);
            if (tahta.tasAl(solPiyonKare) == TasTuru::PIYON && 
                tahta.renkAl(solPiyonKare) == renk) {
                hamleler.ekle(Hamle(solPiyonKare, epKare, TasTuru::PIYON, 
                                   TasTuru::PIYON, HamleTuru::EN_PASSANT));
            }
        }
        
        // Sağ piyon
        if (epSutun < 7) {
            int sagPiyonKare = kareIndeksi(piyonSatir, epSutun + 1);
            if (tahta.tasAl(sagPiyonKare) == TasTuru::PIYON && 
                tahta.renkAl(sagPiyonKare) == renk) {
                hamleler.ekle(Hamle(sagPiyonKare, epKare, TasTuru::PIYON, 
                                   TasTuru::PIYON, HamleTuru::EN_PASSANT));
            }
        }
    }
}

// Terfi hamleleri ekle
void HamleUretici::terfiHamleleriEkle(HamleListesi& hamleler, int kaynak, int hedef, HamleTuru tur) {
    Hamle hamle(kaynak, hedef, TasTuru::PIYON, tur);
    
    hamle.terfiTasi = TasTuru::VEZIR;
    hamleler.ekle(hamle);
    
    hamle.terfiTasi = TasTuru::KALE;
    hamleler.ekle(hamle);
    
    hamle.terfiTasi = TasTuru::FIL;
    hamleler.ekle(hamle);
    
    hamle.terfiTasi = TasTuru::AT;
    hamleler.ekle(hamle);
}

// Işın hamleleri üret (kale, fil, vezir için)
void HamleUretici::isinHamleleriUret(const Tahta& tahta, HamleListesi& hamleler, 
                                     int kare, Bitboard saldirilar, TasTuru tas) {
    while (saldirilar) {
        int hedef = BitboardYardimci::enDusukBitIndeksi(saldirilar);
        TasTuru alinanTas = tahta.tasAl(hedef);
        
        if (alinanTas != TasTuru::YOK) {
            hamleler.ekle(Hamle(kare, hedef, tas, alinanTas, HamleTuru::ALMA));
        } else {
            hamleler.ekle(kare, hedef, tas, HamleTuru::NORMAL);
        }
    }
}

// Tabloları başlat
void HamleUretici::tablolariBaslat() {
    if (tablolarHazir) return;
    
    // Bu fonksiyon magic bitboard tablolarını başlatmalıydı
    // Ancak basit saldırı üretimi için şimdilik yeterli
    tablolarHazir = true;
}