#include "AramaMotoru.h"
#include "Tahta.h"
#include "HamleUretici.h"
#include "Degerlendirici.h"
#include <iostream>
#include <algorithm>
#include <cstring>

// TranspositionTable implementasyonu
TranspositionTable::TranspositionTable(size_t boyutMB) 
    : suankiYas(0), hit(0), miss(0) {
    // MB'dan giriş sayısına çevir
    size_t girisSayisi = (boyutMB * 1024 * 1024) / sizeof(TTGiris);
    // 2'nin kuvveti yap
    boyut = 1;
    while (boyut < girisSayisi) boyut <<= 1;
    
    tablo.resize(boyut);
    temizle();
}

void TranspositionTable::temizle() {
    for (auto& giris : tablo) {
        giris.hash = 0;
        giris.derinlik = -1;
    }
    hit = miss = 0;
}

void TranspositionTable::kaydet(uint64_t hash, int derinlik, int skor, 
                               TTBayrak bayrak, const Hamle& enIyiHamle) {
    size_t indeks = hash & (boyut - 1);
    TTGiris& giris = tablo[indeks];
    
    // Değiştirme stratejisi: daha derin veya aynı yaş
    if (giris.derinlik <= derinlik || giris.yas < suankiYas) {
        giris.hash = hash;
        giris.derinlik = derinlik;
        giris.skor = skor;
        giris.bayrak = bayrak;
        giris.enIyiHamle = enIyiHamle;
        giris.yas = suankiYas;
    }
}

bool TranspositionTable::ara(uint64_t hash, TTGiris& giris) const {
    size_t indeks = hash & (boyut - 1);
    const TTGiris& tabloGiris = tablo[indeks];
    
    if (tabloGiris.hash == hash && tabloGiris.derinlik >= 0) {
        giris = tabloGiris;
        hit++;
        return true;
    }
    
    miss++;
    return false;
}

double TranspositionTable::hitOrani() const {
    size_t toplam = hit + miss;
    return toplam > 0 ? (double)hit / toplam : 0.0;
}

// PVTablosu implementasyonu
void PVTablosu::temizle() {
    std::memset(pvUzunluk, 0, sizeof(pvUzunluk));
}

void PVTablosu::guncelle(int ply, const Hamle& hamle) {
    pv[ply][ply] = hamle;
    
    // Bir sonraki ply'ın PV'sini kopyala
    for (int i = ply + 1; i < pvUzunluk[ply + 1] + ply + 1; i++) {
        pv[ply][i] = pv[ply + 1][i];
    }
    
    pvUzunluk[ply] = pvUzunluk[ply + 1] + 1;
}

void PVTablosu::kopyala(int ply) {
    pvUzunluk[ply] = ply;
}

std::vector<Hamle> PVTablosu::pvAl(int derinlik) const {
    std::vector<Hamle> sonuc;
    int uzunluk = std::min(derinlik, pvUzunluk[0]);
    for (int i = 0; i < uzunluk; i++) {
        sonuc.push_back(pv[0][i]);
    }
    return sonuc;
}

// AramaMotoru implementasyonu
AramaMotoru::AramaMotoru(size_t ttBoyutMB) 
    : maxDerinlik(0), aramaDevam(true), threadSayisi(1) {
    tt = std::make_unique<TranspositionTable>(ttBoyutMB);
    std::memset(killerHamleler, 0, sizeof(killerHamleler));
    std::memset(historyTablo, 0, sizeof(historyTablo));
    std::memset(countermoveTablo, 0, sizeof(countermoveTablo));
    std::memset(butterflyTablo, 0, sizeof(butterflyTablo));
    
    // Thread sayısını belirle
    threadleriBaslat();
}

Hamle AramaMotoru::enIyiHamleyiBul(Tahta& tahta, int derinlik, 
                                   std::chrono::milliseconds sure) {
    aramaBaslangic = std::chrono::steady_clock::now();
    maxDerinlik = derinlik;
    aramaDevam = true;
    istatistikler.sifirla();
    
    // TT yaşını artır
    tt->yeniArama();
    
    Hamle enIyiHamle;
    int enIyiSkor = -SONSUZ;
    
    std::cout << "AramaMotoru: Baslangic - Derinlik: " << derinlik << std::endl;
    
    // Önce legal hamleleri kontrol et
    HamleListesi legalHamleler;
    HamleUretici::tumHamleleriUret(tahta, legalHamleler);
    
    // Legal hamleleri filtrele
    HamleListesi gecerliHamleler;
    for (const Hamle& hamle : legalHamleler) {
        tahta.hamleYap(hamle);
        if (!tahta.sahCekildiMi(tersRenk(tahta.getSira()))) {
            gecerliHamleler.ekle(hamle);
        }
        tahta.hamleGeriAl(hamle);
    }
    
    // Hiç legal hamle yoksa (mat veya pat)
    if (gecerliHamleler.bosMu()) {
        std::cout << "AramaMotoru: Legal hamle yok!" << std::endl;
        return enIyiHamle;
    }
    
    // En az bir legal hamle var, varsayılan olarak ilkini seç
    enIyiHamle = gecerliHamleler[0];
    
    // PV tablosunu temizle
    pvTablo.temizle();
    
    // İteratif derinleştirme
    for (int d = 1; d <= derinlik && aramaDevam; d++) {
        // Her derinlik için düğüm sayısını kaydet
        uint64_t oncekiDugumSayisi = istatistikler.dugumSayisi;
        
        int skor = iteratifDerinlestirme(tahta, d);
        
        if (aramaDevam) {
            auto pv = pvTablo.pvAl(d);
            if (!pv.empty()) {
                enIyiHamle = pv[0];
                enIyiSkor = skor;
            } else {
                // PV boşsa bile skoru güncelle
                enIyiSkor = skor;
            }
            
            // İstatistikleri güncelle
            istatistikler.derinlik = d;
            istatistikler.enIyiSkor = skor;
            
            // Bu derinlikteki düğüm sayısı
            uint64_t buDerinlikDugumler = istatistikler.dugumSayisi - oncekiDugumSayisi;
            
            // Bilgi çıktısı
            std::cout << "info depth " << d << " score cp " << skor;
            std::cout << " nodes " << buDerinlikDugumler;
            std::cout << " total_nodes " << istatistikler.dugumSayisi;
            std::cout << " pv";
            for (const auto& hamle : pv) {
                std::cout << " " << hamle.notasyon();
            }
            std::cout << std::endl;
        }
        
        // Mat bulunduysa daha derine bakma
        if (std::abs(skor) > MAT_DEGERI - 100) {
            break;
        }
    }
    
    // İstatistikleri güncelle
    auto simdi = std::chrono::steady_clock::now();
    istatistikler.gecenSure = std::chrono::duration_cast<std::chrono::milliseconds>(simdi - aramaBaslangic);
    
    // Debug çıktısı kaldırıldı
    
    return enIyiHamle;
}

int AramaMotoru::iteratifDerinlestirme(Tahta& tahta, int hedefDerinlik) {
    int skor = 0;
    int oncekiSkor = 0;
    
    // Debug çıktısı kaldırıldı
    
    // PV'yi bu derinlik için temizle
    pvTablo.kopyala(0);
    
    // İlk aramayi tam pencere ile yap
    int alpha = -SONSUZ;
    int beta = SONSUZ;
    
    skor = alphaBeta(tahta, hedefDerinlik, alpha, beta, 0);
    
    // Aspiration window sadece derinlik >= 4 için kullan
    if (hedefDerinlik >= 4 && std::abs(skor) < MAT_DEGERI - 1000) {
        int delta = ASPIRATION_WINDOW;
        alpha = skor - delta;
        beta = skor + delta;
        
        // Aspiration search
        int aspSkor = alphaBeta(tahta, hedefDerinlik, alpha, beta, 0);
        
        // Genişleyen pencere ile fail durumlarını handle et
        while ((aspSkor <= alpha || aspSkor >= beta) && delta < 500) {
            delta = delta + delta / 2; // Pencereyi %50 genişlet
            
            if (aspSkor <= alpha) {
                alpha = skor - delta;
                beta = skor + delta / 2;
            } else {
                alpha = skor - delta / 2;
                beta = skor + delta;
            }
            
            aspSkor = alphaBeta(tahta, hedefDerinlik, alpha, beta, 0);
        }
        
        skor = aspSkor;
    }
    
    // Debug çıktısı kaldırıldı
    
    return skor;
}

int AramaMotoru::alphaBeta(Tahta& tahta, int derinlik, int alpha, int beta, int ply) {
    istatistikler.dugumSayisi++;
    
    // Zaman kontrolü
    if ((istatistikler.dugumSayisi & 4095) == 0 && !zamanKontrol()) {
        aramaDevam = false;
        return 0;
    }
    
    // Berabere kontrolü (3 tekrar)
    if (ply > 0 && tahta.getElliHamleKurali() >= 100) {
        return BERABERE;
    }
    
    // Transposition table araması
    TTGiris ttGiris;
    uint64_t hash = tahta.getZobristHash();
    
    // Prefetch TT entry
    PREFETCH(&tt->tablo[hash & (tt->boyut - 1)]);
    
    bool ttVurusu = tt->ara(hash, ttGiris);
    
    if (ttVurusu && ttGiris.derinlik >= derinlik && ply > 0) {
        if (ttGiris.bayrak == TTBayrak::EXACT) {
            return ttGiris.skor;
        } else if (ttGiris.bayrak == TTBayrak::LOWER && ttGiris.skor > alpha) {
            alpha = ttGiris.skor;
        } else if (ttGiris.bayrak == TTBayrak::UPPER && ttGiris.skor < beta) {
            beta = ttGiris.skor;
        }
        
        if (alpha >= beta) {
            return ttGiris.skor;
        }
    }
    
    // Derinlik 0'a ulaştıysa quiescence araması
    if (derinlik <= 0) {
        return quiescence(tahta, alpha, beta, ply);
    }
    
    // Null Move Pruning
    if (derinlik >= 3 && !tahta.sahCekildiMi(tahta.getSira()) && ply > 0) {
        // Pas geç
        tahta.pasGec();
        int nullSkor = -alphaBeta(tahta, derinlik - NULL_MOVE_REDUCTION - 1, -beta, -beta + 1, ply + 1);
        tahta.pasGeriAl();
        
        if (nullSkor >= beta) {
            istatistikler.nullMoveKesmeleri++;
            return beta;
        }
    }
    
    // Hamle üret
    HamleListesi hamleler;
    HamleUretici::tumHamleleriUret(tahta, hamleler);
    
    // Debug çıktısı kaldırıldı
    
    // Mat veya pat kontrolü
    if (hamleler.bosMu()) {
        if (tahta.sahCekildiMi(tahta.getSira())) {
            return -MAT_DEGERI + ply; // Mat
        } else {
            return BERABERE; // Pat
        }
    }
    
    // Hamle sıralama
    Hamle* ttHamle = ttVurusu ? &ttGiris.enIyiHamle : nullptr;
    hamleleriSirala(hamleler, tahta, ttHamle, ply);
    
    int enIyiSkor = -SONSUZ;
    Hamle enIyiHamle;
    TTBayrak ttBayrak = TTBayrak::UPPER;
    
    int legalHamleSayisi = 0;
    
    // Hamleleri dene
    for (int i = 0; i < hamleler.getBoyut(); i++) {
        const Hamle& hamle = hamleler[i];
        
        // Hamle yap
        tahta.hamleYap(hamle);
        
        // Legal mi kontrolü - hamleyi yapan tarafın şahı tehdit altında mı?
        if (tahta.sahCekildiMi(tersRenk(tahta.getSira()))) {
            tahta.hamleGeriAl(hamle);
            continue;
        }
        
        legalHamleSayisi++;
        
        int skor;
        
        // Late Move Reduction (LMR)
        int yeniDerinlik = derinlik - 1;
        if (derinlik >= LMR_DEPTH_THRESHOLD && 
            legalHamleSayisi > LMR_MOVES_THRESHOLD &&
            hamle.alinanTas == TasTuru::YOK &&
            hamle.tur != HamleTuru::TERFI &&
            !tahta.sahCekildiMi(tahta.getSira())) {
            // Reduction amount
            yeniDerinlik -= 1;
            if (legalHamleSayisi > 10) yeniDerinlik -= 1;
        }
        
        // PVS (Principal Variation Search)
        if (legalHamleSayisi == 1) {
            skor = -alphaBeta(tahta, yeniDerinlik, -beta, -alpha, ply + 1);
        } else {
            // Null window araması
            skor = -alphaBeta(tahta, yeniDerinlik, -alpha - 1, -alpha, ply + 1);
            
            if (skor > alpha && skor < beta) {
                // Re-search with full depth if reduced search fails high
                if (yeniDerinlik < derinlik - 1) {
                    skor = -alphaBeta(tahta, derinlik - 1, -alpha - 1, -alpha, ply + 1);
                }
                
                // Full window re-search if still in bounds
                if (skor > alpha && skor < beta) {
                    skor = -alphaBeta(tahta, derinlik - 1, -beta, -alpha, ply + 1);
                }
            }
        }
        
        // Hamle geri al
        tahta.hamleGeriAl(hamle);
        
        // Arama durdurulduysa çık
        if (!aramaDevam) {
            return 0;
        }
        
        // En iyi skor güncelleme
        if (skor > enIyiSkor) {
            enIyiSkor = skor;
            enIyiHamle = hamle;
            
            if (skor > alpha) {
                alpha = skor;
                ttBayrak = TTBayrak::EXACT;
                
                // PV güncelle
                pvTablo.guncelle(ply, hamle);
                
                if (skor >= beta) {
                    // Beta kesme
                    istatistikler.betaKesmeleri++;
                    ttBayrak = TTBayrak::LOWER;
                    
                    // Killer hamle güncelle
                    if (hamle.alinanTas == TasTuru::YOK) {
                        killerHamleEkle(hamle, ply);
                    }
                    
                    // History güncelle
                    historyGuncelle(hamle, derinlik);
                    
                    // Butterfly board güncelle
                    butterflyTablo[tahta.getSira() == Renk::BEYAZ ? 0 : 1][hamle.kaynak][hamle.hedef] += derinlik * derinlik;
                    
                    // Countermove güncelle
                    if (ply > 0 && tahta.getSonHamle().gecerliMi()) {
                        const Hamle& oncekiHamle = tahta.getSonHamle();
                        countermoveTablo[oncekiHamle.kaynak][oncekiHamle.hedef] = hamle;
                    }
                    
                    break;
                }
            }
        }
    }
    
    // Hiç legal hamle yoksa mat veya pat
    if (legalHamleSayisi == 0) {
        if (tahta.sahCekildiMi(tahta.getSira())) {
            return -MAT_DEGERI + ply; // Mat
        } else {
            return BERABERE; // Pat
        }
    }
    
    // Transposition table'a kaydet
    tt->kaydet(tahta.getZobristHash(), derinlik, enIyiSkor, ttBayrak, enIyiHamle);
    
    return enIyiSkor;
}

int AramaMotoru::quiescence(Tahta& tahta, int alpha, int beta, int ply) {
    istatistikler.qDugumSayisi++;
    
    // Pozisyon değerlendirmesi
    int evalSkor = Degerlendirici::degerlendir(tahta);
    
    if (evalSkor >= beta) {
        return beta;
    }
    
    if (evalSkor > alpha) {
        alpha = evalSkor;
    }
    
    // Sadece alma hamlelerini üret
    HamleListesi hamleler;
    HamleUretici::saldiriHamleleriUret(tahta, hamleler);
    
    // Hamle sıralama
    hamleleriSirala(hamleler, tahta, nullptr, ply);
    
    // Hamleleri dene
    for (const Hamle& hamle : hamleler) {
        // Prefetch hamle hedefi için bitboard'ları
        PREFETCH(&tahta.bitboardlar[0][0]);
        PREFETCH(&tahta.bitboardlar[1][0]);
        
        // Hamle yap
        tahta.hamleYap(hamle);
        
        // Legal mi kontrolü
        if (tahta.sahCekildiMi(tersRenk(tahta.getSira()))) {
            tahta.hamleGeriAl(hamle);
            continue;
        }
        
        int skor = -quiescence(tahta, -beta, -alpha, ply + 1);
        
        // Hamle geri al
        tahta.hamleGeriAl(hamle);
        
        if (skor >= beta) {
            return beta;
        }
        
        if (skor > alpha) {
            alpha = skor;
        }
    }
    
    return alpha;
}

void AramaMotoru::hamleleriSirala(HamleListesi& hamleler, const Tahta& tahta, 
                                  const Hamle* ttHamle, int ply) {
    // Her hamle için skor hesapla
    for (int i = 0; i < hamleler.getBoyut(); i++) {
        hamleler[i].skor = hamleSkoru(hamleler[i], tahta, ttHamle, ply);
    }
    
    // Sıralama
    hamleler.sirala();
}

int AramaMotoru::hamleSkoru(const Hamle& hamle, const Tahta& tahta, 
                            const Hamle* ttHamle, int ply) {
    // TT hamlesi
    if (ttHamle && hamle == *ttHamle) {
        return HASH_HAMLE_SKOR;
    }
    
    // Winning capture - SEE (Static Exchange Evaluation) kullanarak
    if (hamle.alinanTas != TasTuru::YOK) {
        // MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
        // Gelişmiş versiyon: SEE pozitif olan hamleler önce
        int kazancDegeri = TAS_DEGERLERI[static_cast<int>(hamle.alinanTas)];
        int saldirganDegeri = TAS_DEGERLERI[static_cast<int>(hamle.tas)];
        return KAZANC_SKOR_TABAN + kazancDegeri - saldirganDegeri;
    }
    
    // Killer hamleler - öncelikli
    if (killerHamleMi(hamle, ply)) {
        return hamle == killerHamleler[ply][0] ? KILLER_SKOR_1 : KILLER_SKOR_2;
    }
    
    // Countermove heuristic
    if (ply > 0) {
        const Hamle& oncekiHamle = tahta.getSonHamle();
        if (oncekiHamle.gecerliMi() && hamle == countermoveTablo[oncekiHamle.kaynak][oncekiHamle.hedef]) {
            return KILLER_SKOR_2 - 100;
        }
    }
    
    // Rok hamleleri
    if (hamle.tur == HamleTuru::KISA_ROK || hamle.tur == HamleTuru::UZUN_ROK) {
        return 75000;
    }
    
    // History + butterfly board combination
    int histSkor = historySkoru(hamle);
    int butterflySkor = 0;
    if (tahta.getSira() == Renk::BEYAZ) {
        butterflySkor = butterflyTablo[0][hamle.kaynak][hamle.hedef];
    } else {
        butterflySkor = butterflyTablo[1][hamle.kaynak][hamle.hedef];
    }
    
    return histSkor + (butterflySkor / 100);
}

void AramaMotoru::killerHamleEkle(const Hamle& hamle, int ply) {
    if (ply >= MAX_PLY) return;
    
    if (hamle != killerHamleler[ply][0]) {
        killerHamleler[ply][1] = killerHamleler[ply][0];
        killerHamleler[ply][0] = hamle;
    }
}

bool AramaMotoru::killerHamleMi(const Hamle& hamle, int ply) const {
    if (ply >= MAX_PLY) return false;
    return hamle == killerHamleler[ply][0] || hamle == killerHamleler[ply][1];
}

void AramaMotoru::historyGuncelle(const Hamle& hamle, int derinlik) {
    historyTablo[hamle.kaynak][hamle.hedef] += derinlik * derinlik;
    
    // Overflow kontrolü
    if (historyTablo[hamle.kaynak][hamle.hedef] > 100000) {
        // Tüm değerleri yarıya indir
        for (int i = 0; i < 64; i++) {
            for (int j = 0; j < 64; j++) {
                historyTablo[i][j] /= 2;
            }
        }
    }
}

int AramaMotoru::historySkoru(const Hamle& hamle) const {
    return historyTablo[hamle.kaynak][hamle.hedef] / HISTORY_SKOR_BOLENI;
}

bool AramaMotoru::zamanKontrol() const {
    // Zaman kontrolü kaldırıldı - her zaman devam et
    return true;
}

// Saldırı hamleleri üretme (HamleUretici'ye eklenecek)
void HamleUretici::saldiriHamleleriUret(const Tahta& tahta, HamleListesi& hamleler) {
    hamleler.temizle();
    
    HamleListesi tumHamleler;
    tumHamleleriUret(tahta, tumHamleler);
    
    // Sadece alma hamlelerini filtrele
    for (const Hamle& hamle : tumHamleler) {
        if (hamle.alinanTas != TasTuru::YOK || 
            hamle.tur == HamleTuru::TERFI || 
            hamle.tur == HamleTuru::TERFI_ALMA) {
            hamleler.ekle(hamle);
        }
    }
}

// Lazy SMP İmplementasyonu
void AramaMotoru::threadleriBaslat(int cpuSayisi) {
    if (cpuSayisi == 0) {
        cpuSayisi = std::thread::hardware_concurrency();
        if (cpuSayisi == 0) cpuSayisi = 4; // Varsayılan
    }
    
    threadSayisi = std::min(cpuSayisi, MAX_THREADS);
    threadler.resize(threadSayisi);
    
    for (int i = 0; i < threadSayisi; i++) {
        threadler[i].threadId = i;
        threadler[i].aramaTamamlandi = true;
    }
    
    std::cout << "Lazy SMP: " << threadSayisi << " thread başlatıldı" << std::endl;
}

void AramaMotoru::threadleriDurdur() {
    aramaDevam = false;
    
    for (auto& td : threadler) {
        if (td.thread && td.thread->joinable()) {
            td.thread->join();
        }
    }
}

void AramaMotoru::threadAramasi(ThreadData& td, const Tahta& anaTahta) {
    // Her thread kendi tahtasını kopyalar
    td.tahta = std::make_unique<Tahta>(anaTahta);
    td.aramaTamamlandi = false;
    
    // Thread ID'ye göre farklı derinlikler
    int threadDerinlik = td.derinlik + (td.threadId % 2);
    
    // Arama yap
    int skor = iteratifDerinlestirme(*td.tahta, threadDerinlik);
    
    // En iyi hamleyi güncelle
    {
        std::lock_guard<std::mutex> lock(enIyiHamleMutex);
        auto pv = pvTablo.pvAl(threadDerinlik);
        if (!pv.empty()) {
            td.enIyiHamle = pv[0];
            td.skor = skor;
        }
    }
    
    td.aramaTamamlandi = true;
}

Hamle AramaMotoru::paralelEnIyiHamleyiBul(Tahta& tahta, int derinlik, 
                                         std::chrono::milliseconds sure) {
    // Parametreleri ayarla
    maxDerinlik = derinlik;
    maxSure = sure;
    aramaDevam = true;
    aramaBaslangic = std::chrono::steady_clock::now();
    
    // İstatistikleri sıfırla
    istatistikler.sifirla();
    tt->yeniArama();
    
    // Ana thread'de arama yap - tahta kopyası üzerinde
    Tahta anaTahta = tahta; // Kopyasını oluştur
    Hamle anaHamle = enIyiHamleyiBul(anaTahta, derinlik, sure);
    int anaSkor = istatistikler.enIyiSkor;
    
    // Eğer tek thread varsa, normal aramayı döndür
    if (threadSayisi <= 1) {
        return anaHamle;
    }
    
    // Diğer thread'leri başlat
    for (int i = 1; i < threadSayisi; i++) {
        threadler[i].derinlik = derinlik;
        threadler[i].thread = std::make_unique<std::thread>(
            &AramaMotoru::threadAramasi, this, 
            std::ref(threadler[i]), std::ref(tahta)
        );
    }
    
    // Thread'lerin bitmesini bekle
    for (int i = 1; i < threadSayisi; i++) {
        if (threadler[i].thread && threadler[i].thread->joinable()) {
            threadler[i].thread->join();
        }
    }
    
    // En iyi sonucu seç
    Hamle enIyiHamle = anaHamle;
    int enIyiSkor = anaSkor;
    
    for (int i = 1; i < threadSayisi; i++) {
        if (threadler[i].aramaTamamlandi && threadler[i].skor > enIyiSkor) {
            enIyiSkor = threadler[i].skor;
            enIyiHamle = threadler[i].enIyiHamle;
            std::cout << "Thread " << i << " daha iyi hamle buldu: " 
                      << enIyiHamle.notasyon() << " skor: " << enIyiSkor << std::endl;
        }
    }
    
    return enIyiHamle;
}