#include "BitboardYardimci.h"
#include <cstring>

// Statik değişken tanımlamaları
std::array<std::array<Bitboard, 64>, 64> BitboardYardimci::ISIN_SALDIRI = {};
std::array<Bitboard, 64> BitboardYardimci::AT_HAMLELERI = {};
std::array<Bitboard, 64> BitboardYardimci::SAH_HAMLELERI = {};
std::array<std::array<Bitboard, 64>, 2> BitboardYardimci::PIYON_SALDIRILARI = {};
std::array<Bitboard, 8> BitboardYardimci::DOSYA_MASKELERI = {};

// Magic bitboard tanımlamaları
MagicEntry BitboardYardimci::rookMagics[64];
MagicEntry BitboardYardimci::bishopMagics[64];
Bitboard BitboardYardimci::rookAttacks[64 * 4096];
Bitboard BitboardYardimci::bishopAttacks[64 * 512];

// Statik başlatıcı
static struct BitboardBaslatici {
    BitboardBaslatici() {
        BitboardYardimci::tablolariBaslat();
    }
} bitboardBaslatici;

void BitboardYardimci::tablolariBaslat() {
    // At hamleleri tablosunu oluştur
    for (int kare = 0; kare < 64; kare++) {
        Bitboard hamleler = 0;
        int satir = satirIndeksi(kare);
        int sutun = sutunIndeksi(kare);
        
        // 8 olası at hamlesi
        int atDelta[][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };
        
        for (auto& delta : atDelta) {
            int yeniSatir = satir + delta[0];
            int yeniSutun = sutun + delta[1];
            
            if (yeniSatir >= 0 && yeniSatir < 8 && yeniSutun >= 0 && yeniSutun < 8) {
                int yeniKare = kareIndeksi(yeniSatir, yeniSutun);
                bitEkle(hamleler, yeniKare);
            }
        }
        
        AT_HAMLELERI[kare] = hamleler;
    }
    
    // Şah hamleleri tablosunu oluştur
    for (int kare = 0; kare < 64; kare++) {
        Bitboard hamleler = 0;
        int satir = satirIndeksi(kare);
        int sutun = sutunIndeksi(kare);
        
        // 8 olası şah hamlesi
        int sahDelta[][2] = {
            {-1, -1}, {-1, 0}, {-1, 1},
            {0, -1}, {0, 1},
            {1, -1}, {1, 0}, {1, 1}
        };
        
        for (auto& delta : sahDelta) {
            int yeniSatir = satir + delta[0];
            int yeniSutun = sutun + delta[1];
            
            if (yeniSatir >= 0 && yeniSatir < 8 && yeniSutun >= 0 && yeniSutun < 8) {
                int yeniKare = kareIndeksi(yeniSatir, yeniSutun);
                bitEkle(hamleler, yeniKare);
            }
        }
        
        SAH_HAMLELERI[kare] = hamleler;
    }
    
    // Piyon saldırı tablosunu oluştur
    for (int kare = 0; kare < 64; kare++) {
        // Beyaz piyon saldırıları
        Bitboard beyazSaldirilar = 0;
        if (kare < 56) { // 8. satırda değilse
            if (sutunIndeksi(kare) > 0) { // Sol kenar değilse
                bitEkle(beyazSaldirilar, kare + 7);
            }
            if (sutunIndeksi(kare) < 7) { // Sağ kenar değilse
                bitEkle(beyazSaldirilar, kare + 9);
            }
        }
        PIYON_SALDIRILARI[0][kare] = beyazSaldirilar;
        
        // Siyah piyon saldırıları
        Bitboard siyahSaldirilar = 0;
        if (kare >= 8) { // 1. satırda değilse
            if (sutunIndeksi(kare) > 0) { // Sol kenar değilse
                bitEkle(siyahSaldirilar, kare - 9);
            }
            if (sutunIndeksi(kare) < 7) { // Sağ kenar değilse
                bitEkle(siyahSaldirilar, kare - 7);
            }
        }
        PIYON_SALDIRILARI[1][kare] = siyahSaldirilar;
    }
    
    // Işın saldırı tablosunu oluştur (basit versiyon)
    for (int kaynak = 0; kaynak < 64; kaynak++) {
        for (int hedef = 0; hedef < 64; hedef++) {
            if (kaynak == hedef) {
                ISIN_SALDIRI[kaynak][hedef] = 0;
                continue;
            }
            
            int kaynakSatir = satirIndeksi(kaynak);
            int kaynakSutun = sutunIndeksi(kaynak);
            int hedefSatir = satirIndeksi(hedef);
            int hedefSutun = sutunIndeksi(hedef);
            
            Bitboard isin = 0;
            
            // Aynı satırda mı?
            if (kaynakSatir == hedefSatir) {
                int baslangic = std::min(kaynakSutun, hedefSutun) + 1;
                int bitis = std::max(kaynakSutun, hedefSutun);
                for (int s = baslangic; s < bitis; s++) {
                    bitEkle(isin, kareIndeksi(kaynakSatir, s));
                }
            }
            // Aynı sütunda mı?
            else if (kaynakSutun == hedefSutun) {
                int baslangic = std::min(kaynakSatir, hedefSatir) + 1;
                int bitis = std::max(kaynakSatir, hedefSatir);
                for (int s = baslangic; s < bitis; s++) {
                    bitEkle(isin, kareIndeksi(s, kaynakSutun));
                }
            }
            // Çaprazda mı?
            else if (std::abs(kaynakSatir - hedefSatir) == std::abs(kaynakSutun - hedefSutun)) {
                int satirYon = (hedefSatir > kaynakSatir) ? 1 : -1;
                int sutunYon = (hedefSutun > kaynakSutun) ? 1 : -1;
                
                int satir = kaynakSatir + satirYon;
                int sutun = kaynakSutun + sutunYon;
                
                while (satir != hedefSatir) {
                    bitEkle(isin, kareIndeksi(satir, sutun));
                    satir += satirYon;
                    sutun += sutunYon;
                }
            }
            
            ISIN_SALDIRI[kaynak][hedef] = isin;
        }
    }
    
    // Dosya maskelerini başlat
    for (int dosya = 0; dosya < 8; dosya++) {
        Bitboard maske = 0;
        for (int satir = 0; satir < 8; satir++) {
            bitEkle(maske, kareIndeksi(satir, dosya));
        }
        DOSYA_MASKELERI[dosya] = maske;
    }
    
    // Magic bitboard tablolarını başlat
    initMagicBitboards();

    // Takma isimleri ayarla
    for (int i = 0; i < 64; i++) {
        atHamlesi[i] = AT_HAMLELERI[i];
        sahHamlesi[i] = SAH_HAMLELERI[i];
    }
}

Bitboard BitboardYardimci::isinSaldirisi(int kaynak, int hedef, Bitboard engeller) {
    return ISIN_SALDIRI[kaynak][hedef] & ~engeller;
}

Bitboard BitboardYardimci::kaleSaldirilari(int kare, Bitboard engeller) {
    Bitboard saldirilar = 0;
    
    // Kuzey yönü
    for (int hedef = kare + 8; hedef < 64; hedef += 8) {
        bitEkle(saldirilar, hedef);
        if (bitVar(engeller, hedef)) break;
    }
    
    // Güney yönü
    for (int hedef = kare - 8; hedef >= 0; hedef -= 8) {
        bitEkle(saldirilar, hedef);
        if (bitVar(engeller, hedef)) break;
    }
    
    // Doğu yönü
    int sutun = sutunIndeksi(kare);
    for (int hedef = kare + 1; sutunIndeksi(hedef) > sutun && hedef < 64; hedef++) {
        bitEkle(saldirilar, hedef);
        if (bitVar(engeller, hedef)) break;
    }
    
    // Batı yönü
    for (int hedef = kare - 1; sutunIndeksi(hedef) < sutun && hedef >= 0; hedef--) {
        bitEkle(saldirilar, hedef);
        if (bitVar(engeller, hedef)) break;
    }
    
    return saldirilar;
}

Bitboard BitboardYardimci::filSaldirilari(int kare, Bitboard engeller) {
    Bitboard saldirilar = 0;
    int satir = satirIndeksi(kare);
    int sutun = sutunIndeksi(kare);
    
    // Kuzey-doğu
    for (int s = satir + 1, st = sutun + 1; s < 8 && st < 8; s++, st++) {
        int hedef = kareIndeksi(s, st);
        bitEkle(saldirilar, hedef);
        if (bitVar(engeller, hedef)) break;
    }
    
    // Kuzey-batı
    for (int s = satir + 1, st = sutun - 1; s < 8 && st >= 0; s++, st--) {
        int hedef = kareIndeksi(s, st);
        bitEkle(saldirilar, hedef);
        if (bitVar(engeller, hedef)) break;
    }
    
    // Güney-doğu
    for (int s = satir - 1, st = sutun + 1; s >= 0 && st < 8; s--, st++) {
        int hedef = kareIndeksi(s, st);
        bitEkle(saldirilar, hedef);
        if (bitVar(engeller, hedef)) break;
    }
    
    // Güney-batı
    for (int s = satir - 1, st = sutun - 1; s >= 0 && st >= 0; s--, st--) {
        int hedef = kareIndeksi(s, st);
        bitEkle(saldirilar, hedef);
        if (bitVar(engeller, hedef)) break;
    }
    
    return saldirilar;
}

Bitboard BitboardYardimci::vezirSaldirilari(int kare, Bitboard engeller) {
return kaleSaldirilari(kare, engeller) | filSaldirilari(kare, engeller);
}

// Magic bitboard başlatma fonksiyonları
void BitboardYardimci::initMagicBitboards() {
    // Basitleştirilmiş magic bitboard başlatma
    // Gerçek uygulamada önceden hesaplanmış magic sayıları kullanılır
    
    int rookShifts[64] = {
        52, 53, 53, 53, 53, 53, 53, 52,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        52, 53, 53, 53, 53, 53, 53, 52
    };
    
    int bishopShifts[64] = {
        58, 59, 59, 59, 59, 59, 59, 58,
        59, 59, 59, 59, 59, 59, 59, 59,
        59, 59, 57, 57, 57, 57, 59, 59,
        59, 59, 57, 55, 55, 57, 59, 59,
        59, 59, 57, 55, 55, 57, 59, 59,
        59, 59, 57, 57, 57, 57, 59, 59,
        59, 59, 59, 59, 59, 59, 59, 59,
        58, 59, 59, 59, 59, 59, 59, 58
    };
    
    // Rook attacks için offset
    Bitboard* rookOffset = rookAttacks;
    for (int sq = 0; sq < 64; sq++) {
        rookMagics[sq].attacks = rookOffset;
        rookMagics[sq].shift = rookShifts[sq];
        rookMagics[sq].mask = rookMask(sq);
        rookMagics[sq].magic = 0x1234567890ABCDEFULL; // Placeholder
        
        int size = 1 << (64 - rookShifts[sq]);
        rookOffset += size;
    }
    
    // Bishop attacks için offset
    Bitboard* bishopOffset = bishopAttacks;
    for (int sq = 0; sq < 64; sq++) {
        bishopMagics[sq].attacks = bishopOffset;
        bishopMagics[sq].shift = bishopShifts[sq];
        bishopMagics[sq].mask = bishopMask(sq);
        bishopMagics[sq].magic = 0xFEDCBA0987654321ULL; // Placeholder
        
        int size = 1 << (64 - bishopShifts[sq]);
        bishopOffset += size;
    }
}

Bitboard BitboardYardimci::rookMask(int sq) {
    Bitboard mask = 0;
    int rank = sq / 8;
    int file = sq % 8;
    
    // Dikey
    for (int r = rank + 1; r < 7; r++) mask |= 1ULL << (r * 8 + file);
    for (int r = rank - 1; r > 0; r--) mask |= 1ULL << (r * 8 + file);
    
    // Yatay
    for (int f = file + 1; f < 7; f++) mask |= 1ULL << (rank * 8 + f);
    for (int f = file - 1; f > 0; f--) mask |= 1ULL << (rank * 8 + f);
    
    return mask;
}

Bitboard BitboardYardimci::bishopMask(int sq) {
    Bitboard mask = 0;
    int rank = sq / 8;
    int file = sq % 8;
    
    // Kuzeydoğu
    for (int r = rank + 1, f = file + 1; r < 7 && f < 7; r++, f++)
        mask |= 1ULL << (r * 8 + f);
    
    // Kuzeybatı
    for (int r = rank + 1, f = file - 1; r < 7 && f > 0; r++, f--)
        mask |= 1ULL << (r * 8 + f);
    
    // Güneydoğu
    for (int r = rank - 1, f = file + 1; r > 0 && f < 7; r--, f++)
        mask |= 1ULL << (r * 8 + f);
    
    // Güneybatı
    for (int r = rank - 1, f = file - 1; r > 0 && f > 0; r--, f--)
        mask |= 1ULL << (r * 8 + f);
    
    return mask;
}

// Takma isim dizileri - Degerlendirici için
Bitboard BitboardYardimci::atHamlesi[64];
Bitboard BitboardYardimci::sahHamlesi[64];