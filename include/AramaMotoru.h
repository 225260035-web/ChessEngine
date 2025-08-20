#ifndef ARAMA_MOTORU_H
#define ARAMA_MOTORU_H

#include "Sabitler.h"
#include "Hamle.h"
#include <vector>
#include <chrono>
#include <atomic>
#include <memory>
#include <immintrin.h> // SIMD intrinsics
#include <thread>
#include <mutex>

// Cache line boyutu (modern CPU'lar için genellikle 64 byte)
#define CACHE_LINE_SIZE 64

// Prefetch makroları
#ifdef USE_PREFETCH
#define PREFETCH(addr) __builtin_prefetch(addr)
#define PREFETCH_W(addr) __builtin_prefetch(addr, 1)
#else
#define PREFETCH(addr)
#define PREFETCH_W(addr)
#endif

// İleri tanımlamalar
class Tahta;

// Cache-aligned Transposition Table girişi
struct alignas(CACHE_LINE_SIZE) TTGiris {
    uint64_t hash;
    int derinlik;
    int skor;
    TTBayrak bayrak;
    Hamle enIyiHamle;
    uint8_t yas;
    
    // Padding için ekstra alan (cache line'ı doldurmak için)
    char padding[CACHE_LINE_SIZE - sizeof(uint64_t) - 2*sizeof(int) - 
                 sizeof(TTBayrak) - sizeof(Hamle) - sizeof(uint8_t)];
};

// Transposition Table
class TranspositionTable {
    friend class AramaMotoru; // Prefetch için erişim
private:
    static constexpr size_t VARSAYILAN_BOYUT_MB = 64;
    std::vector<TTGiris> tablo;
    size_t boyut;
    uint8_t suankiYas;
    
    // İstatistikler
    mutable size_t hit;
    mutable size_t miss;
    
public:
    TranspositionTable(size_t boyutMB = VARSAYILAN_BOYUT_MB);
    ~TranspositionTable() = default;
    
    void kaydet(uint64_t hash, int derinlik, int skor, TTBayrak bayrak, 
                const Hamle& enIyiHamle);
    bool ara(uint64_t hash, TTGiris& giris) const;
    void temizle();
    void yasArtir() { suankiYas++; }
    void yeniArama() { yasArtir(); }  // Yeni eklenen metod
    
    // İstatistikler
    double hitOrani() const;
    void istatistikleriSifirla() { hit = miss = 0; }
};

// Arama istatistikleri
struct AramaIstatistikleri {
    uint64_t dugumSayisi;
    uint64_t qDugumSayisi;
    uint64_t ttHit;
    uint64_t ttMiss;
    uint64_t betaKesmeleri;
    uint64_t nullMoveKesmeleri;
    int maxDerinlik;
    int derinlik;
    int enIyiSkor;
    std::chrono::milliseconds gecenSure;
    
    void sifirla() {
        dugumSayisi = qDugumSayisi = ttHit = ttMiss = 0;
        betaKesmeleri = nullMoveKesmeleri = 0;
        maxDerinlik = 0;
        derinlik = 0;
        enIyiSkor = 0;
    }
    
    uint64_t saniyeBasinaDugum() const {
        auto ms = gecenSure.count();
        return ms > 0 ? (dugumSayisi * 1000) / ms : 0;
    }
};

// Principal Variation (PV) tablosu
class PVTablosu {
private:
    static constexpr int MAX_DERINLIK = 64;
    Hamle pv[MAX_DERINLIK][MAX_DERINLIK];
    int pvUzunluk[MAX_DERINLIK];
    
public:
    PVTablosu() { temizle(); }
    
    void guncelle(int ply, const Hamle& hamle);
    void kopyala(int ply);
    std::vector<Hamle> pvAl(int derinlik) const;
    void temizle();
};

// Lazy SMP için thread bilgisi
struct ThreadData {
    std::unique_ptr<std::thread> thread;
    std::unique_ptr<Tahta> tahta;
    int threadId = 0;
    int derinlik = 0;
    std::atomic<int> skor{0};
    std::atomic<bool> aramaTamamlandi{true};
    Hamle enIyiHamle;
    
    // Default constructor
    ThreadData() = default;
    
    // Move constructor
    ThreadData(ThreadData&& other) noexcept
        : thread(std::move(other.thread))
        , tahta(std::move(other.tahta))
        , threadId(other.threadId)
        , derinlik(other.derinlik)
        , skor(other.skor.load())
        , aramaTamamlandi(other.aramaTamamlandi.load())
        , enIyiHamle(other.enIyiHamle) {}
    
    // Delete copy operations
    ThreadData(const ThreadData&) = delete;
    ThreadData& operator=(const ThreadData&) = delete;
    
    // Move assignment
    ThreadData& operator=(ThreadData&& other) noexcept {
        if (this != &other) {
            thread = std::move(other.thread);
            tahta = std::move(other.tahta);
            threadId = other.threadId;
            derinlik = other.derinlik;
            skor = other.skor.load();
            aramaTamamlandi = other.aramaTamamlandi.load();
            enIyiHamle = other.enIyiHamle;
        }
        return *this;
    }
};

// Arama motoru ana sınıfı
class AramaMotoru {
private:
    // Arama parametreleri
    int maxDerinlik;
    std::chrono::milliseconds maxSure;
    std::atomic<bool> aramaDevam;
    
    // Lazy SMP için
    static constexpr int MAX_THREADS = 32;
    int threadSayisi;
    std::vector<ThreadData> threadler;
    std::mutex enIyiHamleMutex;
    
    // Veri yapıları
    std::unique_ptr<TranspositionTable> tt;
    PVTablosu pvTablo;
    AramaIstatistikleri istatistikler;
    
    // Killer moves ve history heuristic
    static constexpr int MAX_PLY = 128;
    alignas(CACHE_LINE_SIZE) Hamle killerHamleler[MAX_PLY][2];
    alignas(CACHE_LINE_SIZE) int historyTablo[64][64]; // [kaynak][hedef]
    
    // Countermove heuristic
    alignas(CACHE_LINE_SIZE) Hamle countermoveTablo[64][64]; // [kaynak][hedef]
    
    // Butterfly boards (history'nin geliştirilmiş versiyonu)
    alignas(CACHE_LINE_SIZE) int butterflyTablo[2][64][64]; // [renk][kaynak][hedef]
    
    // Move ordering skorları
    static constexpr int HASH_HAMLE_SKOR = 1000000;
    static constexpr int KAZANC_SKOR_TABAN = 100000;
    static constexpr int KILLER_SKOR_1 = 90000;
    static constexpr int KILLER_SKOR_2 = 80000;
    static constexpr int HISTORY_SKOR_BOLENI = 100;
    
    // Arama parametreleri
    static constexpr int NULL_MOVE_REDUCTION = 2;
    static constexpr int LMR_MOVES_THRESHOLD = 4;
    static constexpr int LMR_DEPTH_THRESHOLD = 3;
    static constexpr int ASPIRATION_WINDOW = 25;
    
    // Zaman kontrolü
    std::chrono::steady_clock::time_point aramaBaslangic;
    bool zamanKontrol() const;
    
public:
    AramaMotoru(size_t ttBoyutMB = 64);
    ~AramaMotoru() = default;
    
    // Ana arama fonksiyonu
    Hamle enIyiHamleyiBul(Tahta& tahta, int derinlik, 
                          std::chrono::milliseconds sure = std::chrono::milliseconds(0));
    
    // Arama kontrolü
    void aramaDurdur() { aramaDevam = false; }
    bool aramaDevamMi() const { return aramaDevam; }
    
    // İstatistikler
    const AramaIstatistikleri& getIstatistikler() const { return istatistikler; }
    int getSonDegerlendirme() const { return istatistikler.enIyiSkor; }
    
    // TT yönetimi
    void ttTemizle() { tt->temizle(); }
    void ttYasArtir() { tt->yasArtir(); }
    
private:
    // Arama algoritmaları
    int alphaBeta(Tahta& tahta, int derinlik, int alpha, int beta, int ply);
    int quiescence(Tahta& tahta, int alpha, int beta, int ply);
    
    // İteratif derinleştirme
    int iteratifDerinlestirme(Tahta& tahta, int maxDerinlik);
    
    // Hamle sıralama
    void hamleleriSirala(HamleListesi& hamleler, const Tahta& tahta, 
                        const Hamle* ttHamle, int ply);
    int hamleSkoru(const Hamle& hamle, const Tahta& tahta, 
                   const Hamle* ttHamle, int ply);
    
    // Arama iyileştirmeleri
    bool nullMovePruning(Tahta& tahta, int derinlik, int beta, int ply);
    int lateMoveReduction(int derinlik, int hamleNumarasi, bool pvNode);
    
    // Killer moves yönetimi
    void killerHamleEkle(const Hamle& hamle, int ply);
    bool killerHamleMi(const Hamle& hamle, int ply) const;
    
    // History heuristic
    void historyGuncelle(const Hamle& hamle, int derinlik);
    int historySkoru(const Hamle& hamle) const;
    
    // Yardımcı fonksiyonlar
    bool berabereTekrariMi(const Tahta& tahta) const;
    int matSkoru(int ply) const;
    
    // Debug ve bilgi çıktısı
    void aramaBaslangicBilgisi(int derinlik) const;
    void derinlikBilgisi(int derinlik, int skor, const std::vector<Hamle>& pv) const;
    void aramaSonucBilgisi() const;
    
    // Lazy SMP fonksiyonları
    void threadAramasi(ThreadData& td, const Tahta& anaThta);
    Hamle paralelEnIyiHamleyiBul(Tahta& tahta, int derinlik, 
                                 std::chrono::milliseconds sure = std::chrono::milliseconds(0));
    void threadleriBaslat(int cpuSayisi = 0);
    void threadleriDurdur();
};

#endif // ARAMA_MOTORU_H