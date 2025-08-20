#include "Tahta.h"
#include "BitboardYardimci.h"
#include "HamleUretici.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <random>
#include <cstring>

// Statik değişken tanımlamaları
uint64_t Tahta::zobristTaslar[64][2][6];
uint64_t Tahta::zobristRok[2][2];
uint64_t Tahta::zobristEnPassant[8];
uint64_t Tahta::zobristSira;
bool Tahta::zobristBaslatildi = false;

// Zobrist tabloları başlatma
void Tahta::zobristBaslat() {
    if (zobristBaslatildi) return;
    
    std::mt19937_64 rng(42); // Sabit seed
    std::uniform_int_distribution<uint64_t> dist;
    
    // Taş pozisyonları için
    for (int kare = 0; kare < 64; kare++) {
        for (int renk = 0; renk < 2; renk++) {
            for (int tas = 0; tas < 6; tas++) {
                zobristTaslar[kare][renk][tas] = dist(rng);
            }
        }
    }
    
    // Rok hakları için
    for (int renk = 0; renk < 2; renk++) {
        zobristRok[renk][0] = dist(rng); // Kısa rok
        zobristRok[renk][1] = dist(rng); // Uzun rok
    }
    
    // En passant için
    for (int sutun = 0; sutun < 8; sutun++) {
        zobristEnPassant[sutun] = dist(rng);
    }
    
    // Sıra için
    zobristSira = dist(rng);
    
    zobristBaslatildi = true;
}

// Yapıcı
Tahta::Tahta() {
    zobristBaslat();
    baslangicPozisyonu();
}

// Kopyalama yapıcısı
Tahta::Tahta(const Tahta& diger) {
    *this = diger;
}

// Atama operatörü
Tahta& Tahta::operator=(const Tahta& diger) {
    if (this != &diger) {
        std::memcpy(bitboardlar, diger.bitboardlar, sizeof(bitboardlar));
        sira = diger.sira;
        enPassantKare = diger.enPassantKare;
        std::memcpy(rokHaklari, diger.rokHaklari, sizeof(rokHaklari));
        elliHamleKurali = diger.elliHamleKurali;
        hamleNumarasi = diger.hamleNumarasi;
        zobristHash = diger.zobristHash;
        durumGecmisi = diger.durumGecmisi;
    }
    return *this;
}

// Başlangıç pozisyonunu kur
void Tahta::baslangicPozisyonu() {
    // Bitboard'ları temizle
    std::memset(bitboardlar, 0, sizeof(bitboardlar));
    
    // Beyaz taşları yerleştir
    bitboardlar[0][static_cast<int>(TasTuru::PIYON)] = BASLANGIC_BEYAZ_PIYON;
    bitboardlar[0][static_cast<int>(TasTuru::AT)] = BASLANGIC_BEYAZ_AT;
    bitboardlar[0][static_cast<int>(TasTuru::FIL)] = BASLANGIC_BEYAZ_FIL;
    bitboardlar[0][static_cast<int>(TasTuru::KALE)] = BASLANGIC_BEYAZ_KALE;
    bitboardlar[0][static_cast<int>(TasTuru::VEZIR)] = BASLANGIC_BEYAZ_VEZIR;
    bitboardlar[0][static_cast<int>(TasTuru::SAH)] = BASLANGIC_BEYAZ_SAH;
    
    // Siyah taşları yerleştir
    bitboardlar[1][static_cast<int>(TasTuru::PIYON)] = BASLANGIC_SIYAH_PIYON;
    bitboardlar[1][static_cast<int>(TasTuru::AT)] = BASLANGIC_SIYAH_AT;
    bitboardlar[1][static_cast<int>(TasTuru::FIL)] = BASLANGIC_SIYAH_FIL;
    bitboardlar[1][static_cast<int>(TasTuru::KALE)] = BASLANGIC_SIYAH_KALE;
    bitboardlar[1][static_cast<int>(TasTuru::VEZIR)] = BASLANGIC_SIYAH_VEZIR;
    bitboardlar[1][static_cast<int>(TasTuru::SAH)] = BASLANGIC_SIYAH_SAH;
    
    // Tüm taşları güncelle
    bitboardGuncelle();
    
    // Oyun durumu
    sira = Renk::BEYAZ;
    enPassantKare = -1;
    rokHaklari[0][0] = rokHaklari[0][1] = true; // Beyaz
    rokHaklari[1][0] = rokHaklari[1][1] = true; // Siyah
    elliHamleKurali = 0;
    hamleNumarasi = 1;
    
    // Durum geçmişini temizle
    durumGecmisi.clear();
    
    // Zobrist hash hesapla
    zobristHash = zobristHesapla();
}

// Bitboard'ları güncelle (tüm taşlar)
void Tahta::bitboardGuncelle() {
    // Beyaz tüm taşlar
    bitboardlar[0][6] = 0;
    for (int tas = 0; tas < 6; tas++) {
        bitboardlar[0][6] |= bitboardlar[0][tas];
    }
    
    // Siyah tüm taşlar
    bitboardlar[1][6] = 0;
    for (int tas = 0; tas < 6; tas++) {
        bitboardlar[1][6] |= bitboardlar[1][tas];
    }
}

// Zobrist hash hesapla
uint64_t Tahta::zobristHesapla() const {
    uint64_t hash = 0;
    
    // Taşlar
    for (int kare = 0; kare < 64; kare++) {
        TasTuru tas = tasAl(kare);
        if (tas != TasTuru::YOK) {
            Renk renk = renkAl(kare);
            hash ^= zobristTaslar[kare][static_cast<int>(renk)][static_cast<int>(tas)];
        }
    }
    
    // Rok hakları
    if (rokHaklari[0][0]) hash ^= zobristRok[0][0];
    if (rokHaklari[0][1]) hash ^= zobristRok[0][1];
    if (rokHaklari[1][0]) hash ^= zobristRok[1][0];
    if (rokHaklari[1][1]) hash ^= zobristRok[1][1];
    
    // En passant
    if (enPassantKare != -1) {
        hash ^= zobristEnPassant[sutunIndeksi(enPassantKare)];
    }
    
    // Sıra
    if (sira == Renk::SIYAH) {
        hash ^= zobristSira;
    }
    
    return hash;
}

// Taş al
TasTuru Tahta::tasAl(int kare) const {
    Bitboard kareMaske = 1ULL << kare;
    
    // Beyaz taşları kontrol et
    for (int tas = 0; tas < 6; tas++) {
        if (bitboardlar[0][tas] & kareMaske) {
            return static_cast<TasTuru>(tas);
        }
    }
    
    // Siyah taşları kontrol et
    for (int tas = 0; tas < 6; tas++) {
        if (bitboardlar[1][tas] & kareMaske) {
            return static_cast<TasTuru>(tas);
        }
    }
    
    return TasTuru::YOK;
}

// Renk al
Renk Tahta::renkAl(int kare) const {
    Bitboard kareMaske = 1ULL << kare;
    
    if (bitboardlar[0][6] & kareMaske) return Renk::BEYAZ;
    if (bitboardlar[1][6] & kareMaske) return Renk::SIYAH;
    
    return Renk::YOK;
}

// Boş mu?
bool Tahta::bosMu(int kare) const {
    Bitboard kareMaske = 1ULL << kare;
    return !((bitboardlar[0][6] | bitboardlar[1][6]) & kareMaske);
}

// Taş yerleştir
void Tahta::tasYerlestir(int kare, Renk renk, TasTuru tas) {
    int renkIdx = static_cast<int>(renk);
    int tasIdx = static_cast<int>(tas);
    
    BitboardYardimci::bitEkle(bitboardlar[renkIdx][tasIdx], kare);
    BitboardYardimci::bitEkle(bitboardlar[renkIdx][6], kare);
    
    // Zobrist güncelle
    zobristHash ^= zobristTaslar[kare][renkIdx][tasIdx];
}

// Taş kaldır
void Tahta::tasKaldir(int kare, Renk renk, TasTuru tas) {
    int renkIdx = static_cast<int>(renk);
    int tasIdx = static_cast<int>(tas);
    
    BitboardYardimci::bitTemizle(bitboardlar[renkIdx][tasIdx], kare);
    BitboardYardimci::bitTemizle(bitboardlar[renkIdx][6], kare);
    
    // Zobrist güncelle
    zobristHash ^= zobristTaslar[kare][renkIdx][tasIdx];
}

// Taş taşı
void Tahta::tasTasi(int kaynak, int hedef, Renk renk, TasTuru tas) {
    int renkIdx = static_cast<int>(renk);
    int tasIdx = static_cast<int>(tas);
    
    // Kaynak kareyi temizle
    BitboardYardimci::bitTemizle(bitboardlar[renkIdx][tasIdx], kaynak);
    BitboardYardimci::bitTemizle(bitboardlar[renkIdx][6], kaynak);
    
    // Hedef kareye ekle
    BitboardYardimci::bitEkle(bitboardlar[renkIdx][tasIdx], hedef);
    BitboardYardimci::bitEkle(bitboardlar[renkIdx][6], hedef);
    
    // Zobrist güncelle
    zobristHash ^= zobristTaslar[kaynak][renkIdx][tasIdx];
    zobristHash ^= zobristTaslar[hedef][renkIdx][tasIdx];
}

// Hamle yap
void Tahta::hamleYap(const Hamle& hamle) {
    // Mevcut durumu kaydet
    TahtaDurumu durum;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            durum.rokHaklari[i][j] = rokHaklari[i][j];
        }
    }
    durum.enPassantKare = enPassantKare;
    durum.elliHamleKurali = elliHamleKurali;
    durum.zobristHash = zobristHash;
    durumGecmisi.push_back(durum);
    
    // Son hamleyi kaydet
    sonHamle = hamle;
    
    // En passant güncelle
    int eskiEnPassant = enPassantKare;
    if (eskiEnPassant != -1) {
        zobristHash ^= zobristEnPassant[sutunIndeksi(eskiEnPassant)];
    }
    enPassantKare = -1;
    
    // Hamle türüne göre işlem yap
    switch (hamle.tur) {
        case HamleTuru::KISA_ROK:
        case HamleTuru::UZUN_ROK:
            rokYap(hamle);
            break;
        case HamleTuru::EN_PASSANT:
            enPassantYap(hamle);
            break;
        case HamleTuru::TERFI:
        case HamleTuru::TERFI_ALMA:
            terfiYap(hamle);
            break;
        default:
            normalHamleYap(hamle);
            break;
    }
    
    // İki kare piyon hamlesi için en passant ayarla
    if (hamle.tur == HamleTuru::IKI_KARE) {
        enPassantKare = (hamle.kaynak + hamle.hedef) / 2;
        zobristHash ^= zobristEnPassant[sutunIndeksi(enPassantKare)];
    }
    
    // 50 hamle kuralı güncelle
    if (hamle.tas == TasTuru::PIYON || hamle.alinanTas != TasTuru::YOK) {
        elliHamleKurali = 0;
    } else {
        elliHamleKurali++;
    }
    
    // Hamle numarası güncelle
    if (sira == Renk::SIYAH) {
        hamleNumarasi++;
    }
    
    // Sıra değiştir
    sira = tersRenk(sira);
    zobristHash ^= zobristSira;
}

// Normal hamle yap
void Tahta::normalHamleYap(const Hamle& hamle) {
    Renk renk = renkAl(hamle.kaynak);
    
    // Alınan taş varsa kaldır
    if (hamle.alinanTas != TasTuru::YOK) {
        Renk rakipRenk = tersRenk(renk);
        tasKaldir(hamle.hedef, rakipRenk, hamle.alinanTas);
    }
    
    // Taşı hareket ettir
    tasTasi(hamle.kaynak, hamle.hedef, renk, hamle.tas);
    
    // Rok haklarını güncelle
    if (hamle.tas == TasTuru::SAH) {
        if (rokHaklari[static_cast<int>(renk)][0]) {
            rokHaklari[static_cast<int>(renk)][0] = false;
            zobristHash ^= zobristRok[static_cast<int>(renk)][0];
        }
        if (rokHaklari[static_cast<int>(renk)][1]) {
            rokHaklari[static_cast<int>(renk)][1] = false;
            zobristHash ^= zobristRok[static_cast<int>(renk)][1];
        }
    } else if (hamle.tas == TasTuru::KALE) {
        // Kale hareketi rok haklarını etkileyebilir
        if (renk == Renk::BEYAZ) {
            if (hamle.kaynak == H1 && rokHaklari[0][0]) {
                rokHaklari[0][0] = false;
                zobristHash ^= zobristRok[0][0];
            } else if (hamle.kaynak == A1 && rokHaklari[0][1]) {
                rokHaklari[0][1] = false;
                zobristHash ^= zobristRok[0][1];
            }
        } else {
            if (hamle.kaynak == H8 && rokHaklari[1][0]) {
                rokHaklari[1][0] = false;
                zobristHash ^= zobristRok[1][0];
            } else if (hamle.kaynak == A8 && rokHaklari[1][1]) {
                rokHaklari[1][1] = false;
                zobristHash ^= zobristRok[1][1];
            }
        }
    }
    
    // Rakip kale alındıysa rok haklarını güncelle
    if (hamle.alinanTas == TasTuru::KALE) {
        Renk rakip = tersRenk(renk);
        if (rakip == Renk::BEYAZ) {
            if (hamle.hedef == H1 && rokHaklari[0][0]) {
                rokHaklari[0][0] = false;
                zobristHash ^= zobristRok[0][0];
            } else if (hamle.hedef == A1 && rokHaklari[0][1]) {
                rokHaklari[0][1] = false;
                zobristHash ^= zobristRok[0][1];
            }
        } else {
            if (hamle.hedef == H8 && rokHaklari[1][0]) {
                rokHaklari[1][0] = false;
                zobristHash ^= zobristRok[1][0];
            } else if (hamle.hedef == A8 && rokHaklari[1][1]) {
                rokHaklari[1][1] = false;
                zobristHash ^= zobristRok[1][1];
            }
        }
    }
}

// Rok yap
void Tahta::rokYap(const Hamle& hamle) {
    Renk renk = renkAl(hamle.kaynak);
    
    // Şahı hareket ettir
    tasTasi(hamle.kaynak, hamle.hedef, renk, TasTuru::SAH);
    
    // Kaleyi hareket ettir
    if (hamle.tur == HamleTuru::KISA_ROK) {
        int kaleSrc = hamle.hedef + 1;
        int kaleDst = hamle.hedef - 1;
        tasTasi(kaleSrc, kaleDst, renk, TasTuru::KALE);
    } else { // Uzun rok
        int kaleSrc = hamle.hedef - 2;
        int kaleDst = hamle.hedef + 1;
        tasTasi(kaleSrc, kaleDst, renk, TasTuru::KALE);
    }
    
    // Rok haklarını kaldır
    if (rokHaklari[static_cast<int>(renk)][0]) {
        rokHaklari[static_cast<int>(renk)][0] = false;
        zobristHash ^= zobristRok[static_cast<int>(renk)][0];
    }
    if (rokHaklari[static_cast<int>(renk)][1]) {
        rokHaklari[static_cast<int>(renk)][1] = false;
        zobristHash ^= zobristRok[static_cast<int>(renk)][1];
    }
}

// En passant yap
void Tahta::enPassantYap(const Hamle& hamle) {
    Renk renk = renkAl(hamle.kaynak);
    Renk rakip = tersRenk(renk);
    
    // Piyonu hareket ettir
    tasTasi(hamle.kaynak, hamle.hedef, renk, TasTuru::PIYON);
    
    // Alınan piyonu kaldır
    int alinanPiyonKare = hamle.hedef + (renk == Renk::BEYAZ ? -8 : 8);
    tasKaldir(alinanPiyonKare, rakip, TasTuru::PIYON);
}

// Terfi yap
void Tahta::terfiYap(const Hamle& hamle) {
    Renk renk = renkAl(hamle.kaynak);
    
    // Piyonu kaldır
    tasKaldir(hamle.kaynak, renk, TasTuru::PIYON);
    
    // Alınan taş varsa kaldır
    if (hamle.alinanTas != TasTuru::YOK) {
        Renk rakip = tersRenk(renk);
        tasKaldir(hamle.hedef, rakip, hamle.alinanTas);
    }
    
    // Terfi taşını yerleştir
    tasYerlestir(hamle.hedef, renk, hamle.terfiTasi);
}

// Şah pozisyonu
int Tahta::sahPozisyonu(Renk renk) const {
    Bitboard sahBB = bitboardlar[static_cast<int>(renk)][static_cast<int>(TasTuru::SAH)];
    return BitboardYardimci::enDusukBitIndeksi(sahBB);
}

// Kare atak altında mı?
bool Tahta::kareAtakAltindaMi(int kare, Renk saldiran) const {
    Bitboard tumTaslar = getTumTaslar();
    int saldirganIdx = static_cast<int>(saldiran);
    
    // Piyon saldırıları
    Bitboard piyonSaldiri = BitboardYardimci::PIYON_SALDIRILARI[1 - saldirganIdx][kare];
    if (piyonSaldiri & bitboardlar[saldirganIdx][static_cast<int>(TasTuru::PIYON)]) {
        return true;
    }
    
    // At saldırıları
    Bitboard atSaldiri = BitboardYardimci::AT_HAMLELERI[kare];
    if (atSaldiri & bitboardlar[saldirganIdx][static_cast<int>(TasTuru::AT)]) {
        return true;
    }
    
    // Fil saldırıları
    Bitboard filSaldiri = BitboardYardimci::filSaldirilari(kare, tumTaslar);
    if (filSaldiri & bitboardlar[saldirganIdx][static_cast<int>(TasTuru::FIL)]) {
        return true;
    }
    
    // Kale saldırıları
    Bitboard kaleSaldiri = BitboardYardimci::kaleSaldirilari(kare, tumTaslar);
    if (kaleSaldiri & bitboardlar[saldirganIdx][static_cast<int>(TasTuru::KALE)]) {
        return true;
    }
    
    // Vezir saldırıları
    Bitboard vezirSaldiri = filSaldiri | kaleSaldiri;
    if (vezirSaldiri & bitboardlar[saldirganIdx][static_cast<int>(TasTuru::VEZIR)]) {
        return true;
    }
    
    // Şah saldırıları
    Bitboard sahSaldiri = BitboardYardimci::SAH_HAMLELERI[kare];
    if (sahSaldiri & bitboardlar[saldirganIdx][static_cast<int>(TasTuru::SAH)]) {
        return true;
    }
    
    return false;
}

// Şah çekildi mi?
bool Tahta::sahCekildiMi(Renk renk) const {
    int sahKare = sahPozisyonu(renk);
    return kareAtakAltindaMi(sahKare, tersRenk(renk));
}

// Kısa rok yapabilir mi?
bool Tahta::kisaRokYapabilirMi(Renk renk) const {
    int renkIdx = static_cast<int>(renk);
    
    // Rok hakkı var mı?
    if (!rokHaklari[renkIdx][0]) return false;
    
    // Şah çekiliyor mu?
    if (sahCekildiMi(renk)) return false;
    
    int sahKare = renk == Renk::BEYAZ ? E1 : E8;
    int f1 = sahKare + 1;
    int g1 = sahKare + 2;
    
    // Arası boş mu?
    Bitboard tumTaslar = getTumTaslar();
    if ((tumTaslar & (1ULL << f1)) || (tumTaslar & (1ULL << g1))) {
        return false;
    }
    
    // Geçiş kareleri atak altında mı?
    Renk rakip = tersRenk(renk);
    if (kareAtakAltindaMi(f1, rakip) || kareAtakAltindaMi(g1, rakip)) {
        return false;
    }
    
    return true;
}

// Uzun rok yapabilir mi?
bool Tahta::uzunRokYapabilirMi(Renk renk) const {
    int renkIdx = static_cast<int>(renk);
    
    // Rok hakkı var mı?
    if (!rokHaklari[renkIdx][1]) return false;
    
    // Şah çekiliyor mu?
    if (sahCekildiMi(renk)) return false;
    
    int sahKare = renk == Renk::BEYAZ ? E1 : E8;
    int d1 = sahKare - 1;
    int c1 = sahKare - 2;
    int b1 = sahKare - 3;
    
    // Arası boş mu?
    Bitboard tumTaslar = getTumTaslar();
    if ((tumTaslar & (1ULL << d1)) || (tumTaslar & (1ULL << c1)) || (tumTaslar & (1ULL << b1))) {
        return false;
    }
    
    // Geçiş kareleri atak altında mı?
    Renk rakip = tersRenk(renk);
    if (kareAtakAltindaMi(d1, rakip) || kareAtakAltindaMi(c1, rakip)) {
        return false;
    }
    
    return true;
}

// Tahta yazdır (debug için)
void Tahta::yazdir() const {
    std::cout << "\n   a b c d e f g h\n";
    std::cout << "  +---------------+\n";
    
    for (int satir = 7; satir >= 0; satir--) {
        std::cout << satir + 1 << " |";
        
        for (int sutun = 0; sutun < 8; sutun++) {
            int kare = kareIndeksi(satir, sutun);
            TasTuru tas = tasAl(kare);
            Renk renk = renkAl(kare);
            
            char sembol = ' ';
            if (tas != TasTuru::YOK) {
                switch (tas) {
                    case TasTuru::PIYON: sembol = 'P'; break;
                    case TasTuru::AT: sembol = 'N'; break;
                    case TasTuru::FIL: sembol = 'B'; break;
                    case TasTuru::KALE: sembol = 'R'; break;
                    case TasTuru::VEZIR: sembol = 'Q'; break;
                    case TasTuru::SAH: sembol = 'K'; break;
                    default: break;
                }
                
                if (renk == Renk::SIYAH) {
                    sembol = std::tolower(sembol);
                }
            }
            
            std::cout << sembol << " ";
        }
        
        std::cout << "| " << satir + 1 << "\n";
    }
    
    std::cout << "  +---------------+\n";
    std::cout << "   a b c d e f g h\n\n";
    std::cout << "Sıra: " << (sira == Renk::BEYAZ ? "Beyaz" : "Siyah") << "\n";
    std::cout << "FEN: " << fenAl() << "\n\n";
}

// FEN notasyonu al
std::string Tahta::fenAl() const {
    std::stringstream fen;
    
    // Tahta pozisyonu
    for (int satir = 7; satir >= 0; satir--) {
        int bosKare = 0;
        
        for (int sutun = 0; sutun < 8; sutun++) {
            int kare = kareIndeksi(satir, sutun);
            TasTuru tas = tasAl(kare);
            
            if (tas == TasTuru::YOK) {
                bosKare++;
            } else {
                if (bosKare > 0) {
                    fen << bosKare;
                    bosKare = 0;
                }
                
                Renk renk = renkAl(kare);
                char sembol;
                
                switch (tas) {
                    case TasTuru::PIYON: sembol = 'P'; break;
                    case TasTuru::AT: sembol = 'N'; break;
                    case TasTuru::FIL: sembol = 'B'; break;
                    case TasTuru::KALE: sembol = 'R'; break;
                    case TasTuru::VEZIR: sembol = 'Q'; break;
                    case TasTuru::SAH: sembol = 'K'; break;
                    default: sembol = '?'; break;
                }
                
                if (renk == Renk::SIYAH) {
                    sembol = std::tolower(sembol);
                }
                
                fen << sembol;
            }
        }
        
        if (bosKare > 0) {
            fen << bosKare;
        }
        
        if (satir > 0) {
            fen << '/';
        }
    }
    
    // Sıra
    fen << ' ' << (sira == Renk::BEYAZ ? 'w' : 'b');
    
    // Rok hakları
    fen << ' ';
    bool rokVar = false;
    if (rokHaklari[0][0]) { fen << 'K'; rokVar = true; }
    if (rokHaklari[0][1]) { fen << 'Q'; rokVar = true; }
    if (rokHaklari[1][0]) { fen << 'k'; rokVar = true; }
    if (rokHaklari[1][1]) { fen << 'q'; rokVar = true; }
    if (!rokVar) fen << '-';
    
    // En passant
    fen << ' ';
    if (enPassantKare != -1) {
        fen << static_cast<char>('a' + sutunIndeksi(enPassantKare));
        fen << static_cast<char>('1' + satirIndeksi(enPassantKare));
    } else {
        fen << '-';
    }
    
    // 50 hamle kuralı ve hamle numarası
    fen << ' ' << elliHamleKurali << ' ' << hamleNumarasi;
    
    return fen.str();
}

// FEN'den tahta kur
void Tahta::fenKur(const std::string& fen) {
    // Bitboard'ları temizle
    std::memset(bitboardlar, 0, sizeof(bitboardlar));
    
    std::istringstream fenStream(fen);
    std::string tahta, siraStr, rokStr, epStr, elliStr, hamleStr;
    
    fenStream >> tahta >> siraStr >> rokStr >> epStr >> elliStr >> hamleStr;
    
    // Tahta pozisyonu
    int kare = 56; // a8'den başla
    for (char c : tahta) {
        if (c == '/') {
            kare -= 16; // Bir satır aşağı
        } else if (std::isdigit(c)) {
            kare += (c - '0');
        } else {
            Renk renk = std::isupper(c) ? Renk::BEYAZ : Renk::SIYAH;
            TasTuru tas;
            
            switch (std::tolower(c)) {
                case 'p': tas = TasTuru::PIYON; break;
                case 'n': tas = TasTuru::AT; break;
                case 'b': tas = TasTuru::FIL; break;
                case 'r': tas = TasTuru::KALE; break;
                case 'q': tas = TasTuru::VEZIR; break;
                case 'k': tas = TasTuru::SAH; break;
                default: continue;
            }
            
            tasYerlestir(kare, renk, tas);
            kare++;
        }
    }
    
    // Sıra
    sira = (siraStr == "w") ? Renk::BEYAZ : Renk::SIYAH;
    
    // Rok hakları
    rokHaklari[0][0] = rokHaklari[0][1] = false;
    rokHaklari[1][0] = rokHaklari[1][1] = false;
    
    for (char c : rokStr) {
        switch (c) {
            case 'K': rokHaklari[0][0] = true; break;
            case 'Q': rokHaklari[0][1] = true; break;
            case 'k': rokHaklari[1][0] = true; break;
            case 'q': rokHaklari[1][1] = true; break;
        }
    }
    
    // En passant
    if (epStr != "-") {
        int sutun = epStr[0] - 'a';
        int satir = epStr[1] - '1';
        enPassantKare = kareIndeksi(satir, sutun);
    } else {
        enPassantKare = -1;
    }
    
    // 50 hamle kuralı ve hamle numarası
    elliHamleKurali = std::stoi(elliStr);
    hamleNumarasi = std::stoi(hamleStr);
    
    // Bitboard'ları güncelle
    bitboardGuncelle();
    
    // Durum geçmişini temizle
    durumGecmisi.clear();
    
    // Zobrist hash hesapla
    zobristHash = zobristHesapla();
}

// Hamle geri alma
void Tahta::hamleGeriAl(const Hamle& hamle) {
    // Sırayı geri al
    sira = tersRenk(sira);
    zobristHash ^= zobristSira;
    
    // Hamle türüne göre geri al
    switch (hamle.tur) {
        case HamleTuru::KISA_ROK:
        case HamleTuru::UZUN_ROK:
            rokGeriAl(hamle);
            break;
        case HamleTuru::EN_PASSANT:
            enPassantGeriAl(hamle);
            break;
        case HamleTuru::TERFI:
        case HamleTuru::TERFI_ALMA:
            terfiGeriAl(hamle);
            break;
        default:
            normalHamleGeriAl(hamle);
            break;
    }
    
    // Önceki durumu geri yükle
    if (!durumGecmisi.empty()) {
        TahtaDurumu oncekiDurum = durumGecmisi.back();
        durumGecmisi.pop_back();
        
        // En passant durumunu geri yükle ve Zobrist'i güncelle
        if (enPassantKare != oncekiDurum.enPassantKare) {
            if (enPassantKare != -1) {
                zobristHash ^= zobristEnPassant[sutunIndeksi(enPassantKare)];
            }
            if (oncekiDurum.enPassantKare != -1) {
                zobristHash ^= zobristEnPassant[sutunIndeksi(oncekiDurum.enPassantKare)];
            }
            enPassantKare = oncekiDurum.enPassantKare;
        }
        
        // Rok haklarını geri yükle ve Zobrist'i güncelle
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                if (rokHaklari[i][j] != oncekiDurum.rokHaklari[i][j]) {
                    if (rokHaklari[i][j]) {
                        zobristHash ^= zobristRok[i][j]; // Mevcut hakkı kaldır
                    }
                    if (oncekiDurum.rokHaklari[i][j]) {
                        zobristHash ^= zobristRok[i][j]; // Eski hakkı ekle
                    }
                    rokHaklari[i][j] = oncekiDurum.rokHaklari[i][j];
                }
            }
        }
        
        // Diğer durumları geri yükle
        elliHamleKurali = oncekiDurum.elliHamleKurali;
        // zobristHash = oncekiDurum.zobristHash; // Bu satırı kaldırdık!
    }
}

// Normal hamle geri al
void Tahta::normalHamleGeriAl(const Hamle& hamle) {
    Renk renk = renkAl(hamle.hedef);
    
    // Taşı geri taşı
    tasTasi(hamle.hedef, hamle.kaynak, renk, hamle.tas);
    
    // Alınan taş varsa yerine koy
    if (hamle.alinanTas != TasTuru::YOK) {
        Renk rakipRenk = tersRenk(renk);
        tasYerlestir(hamle.hedef, rakipRenk, hamle.alinanTas);
    }
}

// Rok geri al
void Tahta::rokGeriAl(const Hamle& hamle) {
    Renk renk = renkAl(hamle.hedef);
    
    // Şahı geri taşı
    tasTasi(hamle.hedef, hamle.kaynak, renk, TasTuru::SAH);
    
    // Kaleyi geri taşı
    if (hamle.tur == HamleTuru::KISA_ROK) {
        int kaleDst = hamle.hedef - 1;
        int kaleSrc = hamle.hedef + 1;
        tasTasi(kaleDst, kaleSrc, renk, TasTuru::KALE);
    } else { // Uzun rok
        int kaleDst = hamle.hedef + 1;
        int kaleSrc = hamle.hedef - 2;
        tasTasi(kaleDst, kaleSrc, renk, TasTuru::KALE);
    }
}

// En passant geri al
void Tahta::enPassantGeriAl(const Hamle& hamle) {
    Renk renk = renkAl(hamle.hedef);
    Renk rakip = tersRenk(renk);
    
    // Piyonu geri taşı
    tasTasi(hamle.hedef, hamle.kaynak, renk, TasTuru::PIYON);
    
    // Alınan piyonu yerine koy
    int alinanPiyonKare = hamle.hedef + (renk == Renk::BEYAZ ? -8 : 8);
    tasYerlestir(alinanPiyonKare, rakip, TasTuru::PIYON);
}

// Terfi geri al
void Tahta::terfiGeriAl(const Hamle& hamle) {
    Renk renk = renkAl(hamle.hedef);
    
    // Terfi taşını kaldır
    tasKaldir(hamle.hedef, renk, hamle.terfiTasi);
    
    // Piyonu yerine koy
    tasYerlestir(hamle.kaynak, renk, TasTuru::PIYON);
    
    // Alınan taş varsa yerine koy
    if (hamle.alinanTas != TasTuru::YOK) {
        Renk rakip = tersRenk(renk);
        tasYerlestir(hamle.hedef, rakip, hamle.alinanTas);
    }
}

// Legal mi?
bool Tahta::legalMi(const Hamle& hamle) const {
    // Basit kontroller
    if (hamle.kaynak < 0 || hamle.kaynak >= 64 ||
        hamle.hedef < 0 || hamle.hedef >= 64) {
        return false;
    }
    
    // Doğru renk mi?
    if (renkAl(hamle.kaynak) != sira) {
        return false;
    }
    
    // Kendi taşını almaya çalışıyor mu?
    if (renkAl(hamle.hedef) == sira) {
        return false;
    }
    
    // Geçici olarak hamleyi yap ve şah kontrolü yap
    Tahta tmpTahta = *this;
    tmpTahta.hamleYap(hamle);
    
    // Şah çekildi mi?
    return !tmpTahta.sahCekildiMi(tersRenk(tmpTahta.getSira()));
}

// Mat mı?
bool Tahta::matMi() const {
    // Şah çekilmeli ve legal hamle olmamalı
    if (!sahCekildiMi(sira)) return false;
    
    HamleListesi hamleler;
    HamleUretici::tumHamleleriUret(*this, hamleler);
    
    Tahta tmpTahta = *this;
    for (const Hamle& hamle : hamleler) {
        tmpTahta.hamleYap(hamle);
        bool legal = !tmpTahta.sahCekildiMi(tersRenk(tmpTahta.getSira()));
        tmpTahta.hamleGeriAl(hamle);
        
        if (legal) return false;
    }
    
    return true;
}

// Pat mı?
bool Tahta::patMi() const {
    // Şah çekilmemeli ve legal hamle olmamalı
    if (sahCekildiMi(sira)) return false;
    
    HamleListesi hamleler;
    HamleUretici::tumHamleleriUret(*this, hamleler);
    
    Tahta tmpTahta = *this;
    for (const Hamle& hamle : hamleler) {
        tmpTahta.hamleYap(hamle);
        bool legal = !tmpTahta.sahCekildiMi(tersRenk(tmpTahta.getSira()));
        tmpTahta.hamleGeriAl(hamle);
        
        if (legal) return false;
    }
    
    return true;
}

// Berabere mi?
bool Tahta::berabereMi() const {
    // 50 hamle kuralı
    if (elliHamleKurali >= 100) return true;
    
    // Yetersiz materyal kontrolü
    int toplamTas = 0;
    for (int renk = 0; renk < 2; renk++) {
        for (int tas = 0; tas < 5; tas++) { // Şah hariç
            toplamTas += BitboardYardimci::bitSayisi(
                bitboardlar[renk][tas]
            );
        }
    }
    
    // Sadece şahlar kaldıysa
    if (toplamTas == 0) return true;
    
    // Diğer berabere durumları için daha fazla kontrol eklenebilir
    
    return false;
}

// Saldırılar
Bitboard Tahta::saldirilar(int kare, TasTuru tas, Renk renk) const {
    Bitboard tumTaslar = getTumTaslar();
    
    switch (tas) {
        case TasTuru::PIYON:
            return BitboardYardimci::PIYON_SALDIRILARI[static_cast<int>(renk)][kare];
        case TasTuru::AT:
            return BitboardYardimci::AT_HAMLELERI[kare];
        case TasTuru::FIL:
                    return BitboardYardimci::filSaldirilari(kare, tumTaslar);
    case TasTuru::KALE:
        return BitboardYardimci::kaleSaldirilari(kare, tumTaslar);
    case TasTuru::VEZIR:
        return BitboardYardimci::vezirSaldirilari(kare, tumTaslar);
        case TasTuru::SAH:
            return BitboardYardimci::SAH_HAMLELERI[kare];
        default:
            return 0;
    }
}