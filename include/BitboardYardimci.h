#ifndef BITBOARD_YARDIMCI_H
#define BITBOARD_YARDIMCI_H

#include "Sabitler.h"
#include <bit>
#include <array>
#include <immintrin.h>

// MSVC için intrinsics başlığı
#ifdef _MSC_VER
#include <intrin.h>
#endif

// Magic Bitboard yapıları
struct MagicEntry {
    Bitboard mask;
    Bitboard magic;
    Bitboard* attacks;
    unsigned shift;
};

class BitboardYardimci {
public:
    // Magic bitboard tabloları
    static MagicEntry rookMagics[64];
    static MagicEntry bishopMagics[64];
    static Bitboard rookAttacks[64 * 4096];   // Max 12 bit için
    static Bitboard bishopAttacks[64 * 512];  // Max 9 bit için
    
    // Bit sayma fonksiyonları (SIMD optimizasyonlu)
    static inline int bitSayisi(Bitboard bb) {
        #ifdef USE_POPCNT
            #ifdef _MSC_VER
                return static_cast<int>(__popcnt64(bb));
            #else
                return __builtin_popcountll(bb);
            #endif
        #else
            // Fallback implementation
            int count = 0;
            while (bb) {
                count++;
                bb &= bb - 1;
            }
            return count;
        #endif
    }
    
    // En düşük biti bul ve temizle (BMI1 destekli)
    static inline int enDusukBitIndeksi(Bitboard& bb) {
        #ifdef _MSC_VER
            unsigned long index;
            _BitScanForward64(&index, bb);
            bb &= bb - 1; // En düşük biti temizle
            return static_cast<int>(index);
        #else
            int indeks = __builtin_ctzll(bb);
            bb &= bb - 1; // En düşük biti temizle
            return indeks;
        #endif
    }
    
    // PEXT/PDEP işlemleri (BMI2 destekli)
    static inline Bitboard pext(Bitboard src, Bitboard mask) {
        #ifdef __BMI2__
            return _pext_u64(src, mask);
        #else
            Bitboard result = 0;
            int k = 0;
            while (mask) {
                if (src & mask & -mask)
                    result |= (1ULL << k);
                k++;
                mask &= mask - 1;
            }
            return result;
        #endif
    }
    
    // Belirli bir kareye bit ekle
    static inline void bitEkle(Bitboard& bb, int kare) {
        bb |= (1ULL << kare);
    }
    
    // Belirli bir karedeki biti temizle
    static inline void bitTemizle(Bitboard& bb, int kare) {
        bb &= ~(1ULL << kare);
    }
    
    // Belirli bir karede bit var mı?
    static inline bool bitVar(Bitboard bb, int kare) {
        return bb & (1ULL << kare);
    }
    
    // Bitboard'u tersine çevir
    static inline Bitboard tersineCevir(Bitboard bb) {
        #ifdef _MSC_VER
            return _byteswap_uint64(bb);
        #else
            return __builtin_bswap64(bb);
        #endif
    }
    
    // Doğu kaydırma (sağa)
    static inline Bitboard doguKaydir(Bitboard bb) {
        return (bb & ~DOSYA_H) << 1;
    }
    
    // Batı kaydırma (sola)
    static inline Bitboard batiKaydir(Bitboard bb) {
        return (bb & ~DOSYA_A) >> 1;
    }
    
    // Kuzey kaydırma (yukarı)
    static inline Bitboard kuzeyKaydir(Bitboard bb) {
        return bb << 8;
    }
    
    // Güney kaydırma (aşağı)
    static inline Bitboard guneyKaydir(Bitboard bb) {
        return bb >> 8;
    }
    
    // Kuzey-doğu kaydırma
    static inline Bitboard kuzeyDoguKaydir(Bitboard bb) {
        return (bb & ~DOSYA_H) << 9;
    }
    
    // Kuzey-batı kaydırma
    static inline Bitboard kuzeyBatiKaydir(Bitboard bb) {
        return (bb & ~DOSYA_A) << 7;
    }
    
    // Güney-doğu kaydırma
    static inline Bitboard guneyDoguKaydir(Bitboard bb) {
        return (bb & ~DOSYA_H) >> 7;
    }
    
    // Güney-batı kaydırma
    static inline Bitboard guneyBatiKaydir(Bitboard bb) {
        return (bb & ~DOSYA_A) >> 9;
    }
    
    // Işın saldırıları için magic bitboard tabloları (statik olarak başlatılacak)
    static std::array<std::array<Bitboard, 64>, 64> ISIN_SALDIRI;
    
    // At hamle tablosu
    static std::array<Bitboard, 64> AT_HAMLELERI;
    
    // Şah hamle tablosu
    static std::array<Bitboard, 64> SAH_HAMLELERI;
    
    // Piyon saldırı tabloları
    static std::array<std::array<Bitboard, 64>, 2> PIYON_SALDIRILARI;
    
    // Dosya maskeleri (sütunlar için)
    static std::array<Bitboard, 8> DOSYA_MASKELERI;
    
    // Tabloları başlat
    static void tablolariBaslat();
    
    // Işın saldırısı hesapla (kale ve fil için)
    static Bitboard isinSaldirisi(int kaynak, int hedef, Bitboard engeller);
    
    // Kale saldırıları
    static Bitboard kaleSaldirilari(int kare, Bitboard engeller);
    
    // Fil saldırıları
    static Bitboard filSaldirilari(int kare, Bitboard engeller);
    
    // Vezir saldırıları
    static Bitboard vezirSaldirilari(int kare, Bitboard engeller);
    
    // Takma isimler - Degerlendirici için
    static inline Bitboard kaleSaldirisi(int kare, Bitboard engeller) {
        return kaleSaldirilari(kare, engeller);
    }
    static inline Bitboard filSaldirisi(int kare, Bitboard engeller) {
        return filSaldirilari(kare, engeller);
    }
    static inline Bitboard vezirSaldirisi(int kare, Bitboard engeller) {
        return vezirSaldirilari(kare, engeller);
    }
    static inline int popcount(Bitboard bb) {
        return bitSayisi(bb);
    }
    static inline int lsb(Bitboard bb) {
        Bitboard bbCopy = bb;
        return enDusukBitIndeksi(bbCopy);
    }
    
    // Statik üye değişkenleri için takma isimler
    static Bitboard atHamlesi[64];
    static Bitboard sahHamlesi[64];

    // Magic bitboard fonksiyonları
    static inline Bitboard rookAttack(int sq, Bitboard occ) {
        const MagicEntry& entry = rookMagics[sq];
        return entry.attacks[((occ & entry.mask) * entry.magic) >> entry.shift];
    }
    
    static inline Bitboard bishopAttack(int sq, Bitboard occ) {
        const MagicEntry& entry = bishopMagics[sq];
        return entry.attacks[((occ & entry.mask) * entry.magic) >> entry.shift];
    }
    
    // SIMD ile çoklu bitboard işlemleri
    static inline void simdBitboardOr(Bitboard* result, const Bitboard* a, const Bitboard* b, int count) {
        #ifdef __AVX2__
        for (int i = 0; i < count; i += 4) {
            __m256i va = _mm256_loadu_si256((__m256i*)&a[i]);
            __m256i vb = _mm256_loadu_si256((__m256i*)&b[i]);
            __m256i vr = _mm256_or_si256(va, vb);
            _mm256_storeu_si256((__m256i*)&result[i], vr);
        }
        #else
        for (int i = 0; i < count; i++) {
            result[i] = a[i] | b[i];
        }
        #endif
    }

private:
    // Magic bitboard için yardımcı fonksiyonlar
    static Bitboard pozitifIsinSaldirisi(int kare, Bitboard engeller, int yon);
    static Bitboard negatifIsinSaldirisi(int kare, Bitboard engeller, int yon);
    
    // Magic bitboard başlatma
    static void initMagicBitboards();
    static Bitboard rookMask(int sq);
    static Bitboard bishopMask(int sq);
};

#endif // BITBOARD_YARDIMCI_H