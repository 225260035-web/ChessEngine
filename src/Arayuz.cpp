#include "Arayuz.h"
#include "Oyun.h"
#include "Tahta.h"
#include "Degerlendirici.h"
#include <iostream>
#include <cstdio>

// Yapıcı
Arayuz::Arayuz() 
    : seciliKare(-1), hamleAnimasyonu(false), suruklemeAktif(false),
      suruklenenKare(-1), terfiSecimAktif(false), terfiKaynak(-1),
      terfiHedef(-1), motorDerinlik(0), motorSkor(0), motorDugumSayisi(0),
      oyun(nullptr), hedefDerinlik(6), fenGirisAktif(false), fenMetni(""),
      sadeceDegerlendirmeModAktif(false) {
    
    // Renkleri ayarla
    renkleriAyarla();
}

// Yıkıcı
Arayuz::~Arayuz() {
    if (pencere.isOpen()) {
        pencere.close();
    }
}

// Başlat
bool Arayuz::baslat() {
    // Pencere oluştur
    pencere.create(sf::VideoMode(PENCERE_GENISLIGI, PENCERE_YUKSEKLIGI), 
                   "Satranç Motoru - C++ / SFML",
                   sf::Style::Titlebar | sf::Style::Close);
    
    // View'ları ayarla
    tahtaGorunumu.reset(sf::FloatRect(0, 0, TAHTA_BOYUTU, TAHTA_BOYUTU));
    tahtaGorunumu.setViewport(sf::FloatRect(0, 0, 
        (float)TAHTA_BOYUTU / PENCERE_GENISLIGI, 1));
    
    panelGorunumu.reset(sf::FloatRect(0, 0, PANEL_GENISLIGI, PENCERE_YUKSEKLIGI));
    panelGorunumu.setViewport(sf::FloatRect(
        (float)TAHTA_BOYUTU / PENCERE_GENISLIGI, 0,
        (float)PANEL_GENISLIGI / PENCERE_GENISLIGI, 1));
    
    // Fontları yükle
    if (!fontlariYukle()) {
        std::cerr << "Fontlar yüklenemedi!\n";
        // Font olmadan devam et
    }
    
    // Taş görsellerini yükle
    if (!tasGorselleriniYukle()) {
        std::cerr << "Taş görselleri yüklenemedi!\n";
        // Görsel olmadan da çalışabilir
    }
    
    return true;
}

// Ana döngü
void Arayuz::calistir() {
    if (!pencere.isOpen() || !oyun) return;
    
    while (pencere.isOpen()) {
        olaylariIsle();
        guncelle();
        ciz();
    }
}

// Olayları işle
void Arayuz::olaylariIsle() {
    sf::Event olay;
    while (pencere.pollEvent(olay)) {
        switch (olay.type) {
            case sf::Event::Closed:
                pencere.close();
                break;
                
            case sf::Event::MouseButtonPressed:
                fareTiklamisiniIsle(olay);
                break;
                
            case sf::Event::MouseButtonReleased:
                fareBirakmaIsle(olay);
                break;
                
            case sf::Event::MouseMoved:
                fareHareketiniIsle(olay);
                break;
                
            case sf::Event::KeyPressed:
                klavyeGirdisiniIsle(olay);
                break;
                
            case sf::Event::TextEntered:
                if (fenGirisAktif && olay.text.unicode >= 32 && olay.text.unicode < 127) {
                    fenMetni += static_cast<char>(olay.text.unicode);
                }
                break;
                
            default:
                break;
        }
    }
}

// Güncelle
void Arayuz::guncelle() {
    if (!oyun) return;
    
    // Motor hamlesi hazır mı?
    if (oyun->motorHazirMi()) {
        oyun->motorHamlesiniAl();
    }
    
    // Animasyon güncelle
    if (hamleAnimasyonu) {
        animasyonuGuncelle();
    }
}

// Çiz
void Arayuz::ciz() {
    pencere.clear();
    
    // Tahta görünümü
    pencere.setView(tahtaGorunumu);
    tahtaCiz();
    
    if (oyun) {
        taslariCiz(oyun->getTahta());
        sonHamleyiCiz();
        seciliKareCiz();
        legalHamleleriCiz();
        sahKontrolCiz(oyun->getTahta());
    }
    
    koordinatlariCiz();
    
    // Terfi seçimi aktifse göster
    if (terfiSecimAktif) {
        terfiPenceresiCiz();
    }

    // FEN girişi aktifse göster
    fenGirisineCiz();
    
    // Panel görünümü
    pencere.setView(panelGorunumu);
    panelCiz();
    
    pencere.display();
}

// Tahta çiz
void Arayuz::tahtaCiz() {
    for (int satir = 0; satir < 8; satir++) {
        for (int sutun = 0; sutun < 8; sutun++) {
            sf::RectangleShape kare(sf::Vector2f(KARE_BOYUTU, KARE_BOYUTU));
            kare.setPosition(sutun * KARE_BOYUTU, (7 - satir) * KARE_BOYUTU);
            
            // Satranç tahtası deseni - a1 koyu renk olacak şekilde
            if ((satir + sutun) % 2 == 1) {
                kare.setFillColor(beyazKareRengi);
            } else {
                kare.setFillColor(siyahKareRengi);
            }
            
            pencere.draw(kare);
        }
    }
}

// Taşları çiz
void Arayuz::taslariCiz(const Tahta& tahta) {
    for (int kare = 0; kare < 64; kare++) {
        // Sürüklenen taşı atla
        if (suruklemeAktif && kare == suruklenenKare) continue;
        
        TasTuru tas = tahta.tasAl(kare);
        if (tas != TasTuru::YOK) {
            Renk renk = tahta.renkAl(kare);
            
            // Basit görselleştirme (sprite yoksa)
            if (tasSpriteleri.empty()) {
                sf::CircleShape tasGorseli(KARE_BOYUTU * 0.4f);
                sf::Vector2f pozisyon = kareyePixel(kare);
                tasGorseli.setPosition(pozisyon.x + KARE_BOYUTU * 0.1f,
                                       pozisyon.y + KARE_BOYUTU * 0.1f);
                
                // Renk ayarla
                if (renk == Renk::BEYAZ) {
                    tasGorseli.setFillColor(sf::Color::White);
                    tasGorseli.setOutlineColor(sf::Color::Black);
                } else {
                    tasGorseli.setFillColor(sf::Color::Black);
                    tasGorseli.setOutlineColor(sf::Color::White);
                }
                tasGorseli.setOutlineThickness(2);
                
                pencere.draw(tasGorseli);
                
                // Taş harfi
                sf::Text tasHarfi;
                tasHarfi.setFont(anaFont);
                tasHarfi.setCharacterSize(30);
                tasHarfi.setFillColor(renk == Renk::BEYAZ ? sf::Color::Black : sf::Color::White);
                
                std::string harf;
                switch (tas) {
                    case TasTuru::PIYON: harf = "P"; break;
                    case TasTuru::AT: harf = "N"; break;
                    case TasTuru::FIL: harf = "B"; break;
                    case TasTuru::KALE: harf = "R"; break;
                    case TasTuru::VEZIR: harf = "Q"; break;
                    case TasTuru::SAH: harf = "K"; break;
                    default: break;
                }
                
                tasHarfi.setString(harf);
                sf::FloatRect bounds = tasHarfi.getLocalBounds();
                tasHarfi.setOrigin(bounds.width / 2, bounds.height / 2);
                tasHarfi.setPosition(pozisyon.x + KARE_BOYUTU / 2,
                                    pozisyon.y + KARE_BOYUTU / 2);
                
                pencere.draw(tasHarfi);
            } else {
                // Sprite kullan
                auto it = tasSpriteleri.find({renk, tas});
                if (it != tasSpriteleri.end()) {
                    spriteKonumlandir(it->second, kare);
                    pencere.draw(it->second);
                }
            }
        }
    }
    
    // Sürüklenen taşı çiz
    if (suruklemeAktif && suruklenenKare >= 0) {
        sf::Vector2i farePoz = sf::Mouse::getPosition(pencere);
        sf::Vector2f dunya = pencere.mapPixelToCoords(farePoz, tahtaGorunumu);
        
        TasTuru tas = oyun->getTahta().tasAl(suruklenenKare);
        Renk renk = oyun->getTahta().renkAl(suruklenenKare);
        
        if (tasSpriteleri.empty()) {
            sf::CircleShape tasGorseli(KARE_BOYUTU * 0.4f);
            tasGorseli.setPosition(dunya.x - KARE_BOYUTU * 0.4f,
                                  dunya.y - KARE_BOYUTU * 0.4f);
            
            if (renk == Renk::BEYAZ) {
                tasGorseli.setFillColor(sf::Color(255, 255, 255, 200));
                tasGorseli.setOutlineColor(sf::Color::Black);
            } else {
                tasGorseli.setFillColor(sf::Color(0, 0, 0, 200));
                tasGorseli.setOutlineColor(sf::Color::White);
            }
            tasGorseli.setOutlineThickness(2);
            
            pencere.draw(tasGorseli);
        } else {
            // Sprite kullan
            auto it = tasSpriteleri.find({renk, tas});
            if (it != tasSpriteleri.end()) {
                sf::Sprite suruklenenSprite = it->second;
                suruklenenSprite.setPosition(dunya.x - KARE_BOYUTU / 2,
                                           dunya.y - KARE_BOYUTU / 2);
                // Yarı saydam yap
                suruklenenSprite.setColor(sf::Color(255, 255, 255, 200));
                pencere.draw(suruklenenSprite);
            }
        }
    }
}

// Panel çiz
void Arayuz::panelCiz() {
    // Arka plan
    sf::RectangleShape arkaplan(sf::Vector2f(PANEL_GENISLIGI, PENCERE_YUKSEKLIGI));
    arkaplan.setFillColor(panelArkaplanRengi);
    pencere.draw(arkaplan);
    
    oyunBilgileriniCiz();
    hamleGecmisiCiz();
    motorBilgileriniCiz();
    derinlikKontrolleriniCiz();
}

// Derinlik kontrollerini çiz
void Arayuz::derinlikKontrolleriniCiz() {
    // Başlık
    sf::Text baslik;
    baslik.setFont(anaFont);
    baslik.setCharacterSize(18);
    baslik.setFillColor(sf::Color::White);
    baslik.setString("Derinlik Ayari");
    baslik.setPosition(10, PENCERE_YUKSEKLIGI - 100);
    pencere.draw(baslik);
    
    // Azalt butonu
    sf::RectangleShape azaltButon(sf::Vector2f(30, 30));
    azaltButon.setPosition(10, PENCERE_YUKSEKLIGI - 60);
    azaltButon.setFillColor(sf::Color(70, 70, 70));
    azaltButon.setOutlineColor(sf::Color::White);
    azaltButon.setOutlineThickness(1);
    pencere.draw(azaltButon);
    
    sf::Text eksi;
    eksi.setFont(anaFont);
    eksi.setCharacterSize(20);
    eksi.setFillColor(sf::Color::White);
    eksi.setString("-");
    eksi.setPosition(20, PENCERE_YUKSEKLIGI - 60);
    pencere.draw(eksi);
    
    // Derinlik değeri
    sf::Text deger;
    deger.setFont(anaFont);
    deger.setCharacterSize(24);
    deger.setFillColor(sf::Color::Yellow);
    deger.setString(std::to_string(hedefDerinlik));
    sf::FloatRect bounds = deger.getLocalBounds();
    deger.setPosition(60 - bounds.width/2, PENCERE_YUKSEKLIGI - 62);
    pencere.draw(deger);
    
    // Artır butonu
    sf::RectangleShape artirButon(sf::Vector2f(30, 30));
    artirButon.setPosition(80, PENCERE_YUKSEKLIGI - 60);
    artirButon.setFillColor(sf::Color(70, 70, 70));
    artirButon.setOutlineColor(sf::Color::White);
    artirButon.setOutlineThickness(1);
    pencere.draw(artirButon);
    
    sf::Text arti;
    arti.setFont(anaFont);
    arti.setCharacterSize(20);
    arti.setFillColor(sf::Color::White);
    arti.setString("+");
    arti.setPosition(90, PENCERE_YUKSEKLIGI - 60);
    pencere.draw(arti);
    
    // Açıklama
    sf::Text aciklama;
    aciklama.setFont(anaFont);
    aciklama.setCharacterSize(12);
    aciklama.setFillColor(sf::Color(200, 200, 200));
    aciklama.setString("(1-20 arasi)");
    aciklama.setPosition(120, PENCERE_YUKSEKLIGI - 55);
    pencere.draw(aciklama);
}

// Oyun bilgilerini çiz
void Arayuz::oyunBilgileriniCiz() {
    if (!oyun) return;
    
    sf::Text baslik;
    baslik.setFont(anaFont);
    baslik.setCharacterSize(24);
    baslik.setFillColor(sf::Color::White);
    baslik.setString("Satranc Motoru");
    baslik.setPosition(10, 10);
    pencere.draw(baslik);
    
    // Sıra kimde
    sf::Text sira;
    sira.setFont(anaFont);
    sira.setCharacterSize(18);
    sira.setFillColor(sf::Color::White);
    sira.setString("Sira: " + std::string(oyun->getSira() == Renk::BEYAZ ? "Beyaz" : "Siyah"));
    sira.setPosition(10, 50);
    pencere.draw(sira);
    
    // Oyun durumu
    if (oyun->oyunBittiMi()) {
        sf::Text durum;
        durum.setFont(anaFont);
        durum.setCharacterSize(16);
        durum.setFillColor(sf::Color::Yellow);
        durum.setString(oyun->sonucMetni());
        durum.setPosition(10, 80);
        pencere.draw(durum);
    }
    
    // FEN modu göstergesi
    if (sadeceDegerlendirmeModAktif) {
        sf::Text fenMod;
        fenMod.setFont(anaFont);
        fenMod.setCharacterSize(14);
        fenMod.setFillColor(sf::Color(100, 200, 100));
        fenMod.setString("FEN Degerlendirme Modu");
        fenMod.setPosition(10, 110);
        pencere.draw(fenMod);
        
        sf::Text ipucu;
        ipucu.setFont(anaFont);
        ipucu.setCharacterSize(12);
        ipucu.setFillColor(sf::Color(180, 180, 180));
        ipucu.setString("F: FEN yukle | R: Yeni oyun");
        ipucu.setPosition(10, 130);
        pencere.draw(ipucu);
    }
}

// Motor bilgilerini çiz
void Arayuz::motorBilgileriniCiz() {
    sf::Text baslik;
    baslik.setFont(anaFont);
    baslik.setCharacterSize(18);
    baslik.setFillColor(sf::Color::White);
    baslik.setString("Motor Bilgisi");
    baslik.setPosition(10, 300);
    pencere.draw(baslik);
    
    // Skor
    sf::Text skor;
    skor.setFont(anaFont);
    skor.setCharacterSize(16);
    skor.setFillColor(sf::Color::Green);
    char skorStr[32];
    snprintf(skorStr, sizeof(skorStr), "Skor: %+.2f", motorSkor / 100.0f);
    skor.setString(skorStr);
    skor.setPosition(10, 330);
    pencere.draw(skor);
    
    // Derinlik
    sf::Text derinlik;
    derinlik.setFont(anaFont);
    derinlik.setCharacterSize(14);
    derinlik.setFillColor(sf::Color::White);
    derinlik.setString("Derinlik: " + std::to_string(motorDerinlik));
    derinlik.setPosition(10, 355);
    pencere.draw(derinlik);
    
    // Düğüm sayısı
    sf::Text dugumler;
    dugumler.setFont(anaFont);
    dugumler.setCharacterSize(14);
    dugumler.setFillColor(sf::Color::White);
    
    // Düğüm sayısını formatla (K/M)
    std::string dugumStr;
    if (motorDugumSayisi >= 1000000) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1fM", motorDugumSayisi / 1000000.0);
        dugumStr = buf;
    } else if (motorDugumSayisi >= 1000) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1fK", motorDugumSayisi / 1000.0);
        dugumStr = buf;
    } else {
        dugumStr = std::to_string(motorDugumSayisi);
    }
    
    dugumler.setString("Düğümler: " + dugumStr);
    dugumler.setPosition(10, 375);
    pencere.draw(dugumler);
    
    // Düşünce
    if (!motorDusuncesi.empty()) {
        sf::Text dusunce;
        dusunce.setFont(anaFont);
        dusunce.setCharacterSize(14);
        dusunce.setFillColor(sf::Color::Cyan);
        dusunce.setString(motorDusuncesi);
        dusunce.setPosition(10, 395);
        pencere.draw(dusunce);
    }
}

// Hamle geçmişini çiz
void Arayuz::hamleGecmisiCiz() {
    sf::Text baslik;
    baslik.setFont(anaFont);
    baslik.setCharacterSize(18);
    baslik.setFillColor(sf::Color::White);
    baslik.setString("Hamleler");
    baslik.setPosition(10, 120);
    pencere.draw(baslik);
    
    // Son 8 hamleyi göster
    int baslangic = std::max(0, (int)hamleGecmisi.size() - 8);
    for (int i = baslangic; i < hamleGecmisi.size(); i++) {
        sf::Text hamle;
        hamle.setFont(anaFont);
        hamle.setCharacterSize(14);
        hamle.setFillColor(sf::Color::White);
        
        std::string hamleStr;
        if (i % 2 == 0) {
            hamleStr = std::to_string(i/2 + 1) + ". ";
        } else {
            hamleStr = "    ";
        }
        hamleStr += hamleGecmisi[i];
        
        hamle.setString(hamleStr);
        hamle.setPosition(10, 150 + (i - baslangic) * 18);
        pencere.draw(hamle);
    }
}

// Fare tıklaması işle
void Arayuz::fareTiklamisiniIsle(const sf::Event& olay) {
    if (!oyun || oyun->oyunBittiMi()) return;
    
    // Sol tık
    if (olay.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i piksel(olay.mouseButton.x, olay.mouseButton.y);
        
        // Panel tıklaması kontrolü
        if (piksel.x >= TAHTA_BOYUTU) {
            // Derinlik kontrolleri
            if (piksel.y >= PENCERE_YUKSEKLIGI - 60 && piksel.y <= PENCERE_YUKSEKLIGI - 30) {
                // Azalt butonu
                if (piksel.x >= TAHTA_BOYUTU + 10 && piksel.x <= TAHTA_BOYUTU + 40) {
                    if (hedefDerinlik > 1) {
                        hedefDerinlik--;
                        if (oyun) {
                            oyun->motorDerinliginiAyarla(hedefDerinlik);
                        }
                    }
                }
                // Artır butonu
                else if (piksel.x >= TAHTA_BOYUTU + 80 && piksel.x <= TAHTA_BOYUTU + 110) {
                    if (hedefDerinlik < 20) {
                        hedefDerinlik++;
                        if (oyun) {
                            oyun->motorDerinliginiAyarla(hedefDerinlik);
                        }
                    }
                }
            }
            return;
        }
        
        // Tahta tıklaması
        sf::Vector2f dunya = pencere.mapPixelToCoords(piksel, tahtaGorunumu);
        
        if (tahtaIcindeMi(dunya)) {
            int kare = pixeldenKareye(dunya);
            
            // Geçerli kare kontrolü
            if (kare < 0 || kare >= 64) {
                return;
            }
            
            // Terfi seçimi aktifse
            if (terfiSecimAktif) {
                // Terfi taşı seçimi yapılacak
                terfiSecimiTiklamaIsle(dunya);
                return;
            }
            
            // Taş seç veya hamle yap
            if (seciliKare == -1) {
                // Taş seç
                if (!oyun->getTahta().bosMu(kare) &&
                    oyun->getTahta().renkAl(kare) == oyun->getSira() &&
                    oyun->siradakiOyuncuInsan()) {  // Sadece insan oyuncu taş seçebilir
                    seciliKare = kare;
                    legalHamleKareleri.clear();
                    rokHamleKareleri.clear();  // Rok hamleleri listesini temizle
                    
                    // Legal hamleleri bul
                    HamleListesi legalHamleler = oyun->legalHamleleriAl(kare);
                    
                    // DEBUG: Sadece şah seçildiğinde ve rok hamlesi varsa göster
                    TasTuru secilenTas = oyun->getTahta().tasAl(kare);
                    if (secilenTas == TasTuru::SAH) {
                        for (const Hamle& hamle : legalHamleler) {
                            if (hamle.tur == HamleTuru::KISA_ROK || hamle.tur == HamleTuru::UZUN_ROK) {
                                std::cout << "\n[ROK] Şah seçildi, rok hamlesi mevcut: " 
                                          << hamle.notasyon() 
                                          << " (" << (hamle.tur == HamleTuru::KISA_ROK ? "Kısa" : "Uzun") << " rok)\n";
                            }
                        }
                    }
                    
                    for (const Hamle& hamle : legalHamleler) {
                        // Rok hamlesi ise ayrı listeye ekle
                        if (hamle.tur == HamleTuru::KISA_ROK || hamle.tur == HamleTuru::UZUN_ROK) {
                            rokHamleKareleri.push_back(hamle.hedef);
                        } else {
                            legalHamleKareleri.push_back(hamle.hedef);
                        }
                    }
                    
                    // Sürüklemeyi başlat
                    suruklemeAktif = true;
                    suruklenenKare = kare;
                }
            } else {
                // Hamle yap
                if (oyun->siradakiOyuncuInsan()) {  // Sadece insan oyuncu hamle yapabilir
                    // Rok kontrolü - eğer şah seçiliyse ve hedef 2 kare uzaktaysa
                    TasTuru seciliTas = oyun->getTahta().tasAl(seciliKare);
                    if (seciliTas == TasTuru::SAH) {
                        int fark = kare - seciliKare;
                        if (std::abs(fark) == 2) {
                            // Bu bir rok hamlesi olabilir
                            hamleYap(seciliKare, kare);
                        } else {
                            hamleYap(seciliKare, kare);
                        }
                    } else {
                        hamleYap(seciliKare, kare);
                    }
                }
                seciliKare = -1;
                legalHamleKareleri.clear();
                rokHamleKareleri.clear();
            }
        }
    }
}

// Fare bırakma işle
void Arayuz::fareBirakmaIsle(const sf::Event& olay) {
    if (suruklemeAktif) {
        sf::Vector2i piksel(olay.mouseButton.x, olay.mouseButton.y);
        sf::Vector2f dunya = pencere.mapPixelToCoords(piksel, tahtaGorunumu);
        
        if (tahtaIcindeMi(dunya)) {
            int hedefKare = pixeldenKareye(dunya);
            
            if (hedefKare != suruklenenKare && oyun->siradakiOyuncuInsan()) {
                hamleYap(suruklenenKare, hedefKare);
            }
        }
        
        suruklemeAktif = false;
        suruklenenKare = -1;
        seciliKare = -1;
        legalHamleKareleri.clear();
        rokHamleKareleri.clear();
    }
}

// Fare hareketi işle
void Arayuz::fareHareketiniIsle(const sf::Event& olay) {
    if (suruklemeAktif) {
        // Fare pozisyonunu güncelle (sürükleme animasyonu için)
        sf::Vector2i farePoz(olay.mouseMove.x, olay.mouseMove.y);
        sf::Vector2f dunya = pencere.mapPixelToCoords(farePoz, tahtaGorunumu);
        
        // Sürükleme offset'ini güncelle
        suruklemeOffset = dunya;
    }
}

// Hamle yap
void Arayuz::hamleYap(int kaynak, int hedef) {
    if (!oyun) return;
    
    // Legal mi kontrol et
    bool legal = false;
    for (int legalKare : legalHamleKareleri) {
        if (legalKare == hedef) {
            legal = true;
            break;
        }
    }
    
    // Rok hamlesi mi kontrol et
    if (!legal) {
        for (int rokKare : rokHamleKareleri) {
            if (rokKare == hedef) {
                legal = true;
                break;
            }
        }
    }
    
    if (!legal && seciliKare != -1) return;
    
    // Terfi kontrolü
    TasTuru tas = oyun->getTahta().tasAl(kaynak);
    Renk renk = oyun->getTahta().renkAl(kaynak);
    int hedefSatir = satirIndeksi(hedef);
    
    if (tas == TasTuru::PIYON && 
        ((renk == Renk::BEYAZ && hedefSatir == 7) ||
         (renk == Renk::SIYAH && hedefSatir == 0))) {
        // Terfi seçimi göster
        terfiSeciminiGoster(kaynak, hedef);
        return;
    }
    
    // Normal hamle
    oyun->hamleYap(kaynak, hedef);
    sonHamle = Hamle(kaynak, hedef, tas);
}

// Seçili kareyi çiz
void Arayuz::seciliKareCiz() {
    if (seciliKare >= 0) {
        dikdortgenCiz(seciliKare, seciliKareRengi, 4);
    }
}

// Legal hamleleri çiz
void Arayuz::legalHamleleriCiz() {
    for (int kare : legalHamleKareleri) {
        daireCiz(kare, legalHamleRengi, KARE_BOYUTU * 0.15f);
    }
    
    // Rok hamlelerini farklı renkle göster
    for (int kare : rokHamleKareleri) {
        daireCiz(kare, rokHamleRengi, KARE_BOYUTU * 0.2f);  // Biraz daha büyük daire
    }
}

// Son hamleyi çiz
void Arayuz::sonHamleyiCiz() {
    if (sonHamle.kaynak != sonHamle.hedef) {
        dikdortgenCiz(sonHamle.kaynak, sonHamleKaynakRengi, 3);
        dikdortgenCiz(sonHamle.hedef, sonHamleHedefRengi, 3);
    }
}

// Şah kontrolü çiz
void Arayuz::sahKontrolCiz(const Tahta& tahta) {
    if (tahta.sahCekildiMi(tahta.getSira())) {
        int sahKare = tahta.sahPozisyonu(tahta.getSira());
        dikdortgenCiz(sahKare, sahKontrolRengi, 5);
    }
}

// Koordinatları çiz
void Arayuz::koordinatlariCiz() {
    // Dosya harfleri (a-h)
    for (int i = 0; i < 8; i++) {
        sf::Text harf;
        harf.setFont(anaFont);
        harf.setCharacterSize(16);
        harf.setFillColor(sf::Color(200, 200, 200));
        harf.setString(std::string(1, 'a' + i));
        harf.setPosition(i * KARE_BOYUTU + KARE_BOYUTU/2 - 5, 
                        TAHTA_BOYUTU + 5);
        pencere.draw(harf);
    }
    
    // Satır numaraları (1-8)
    for (int i = 0; i < 8; i++) {
        sf::Text numara;
        numara.setFont(anaFont);
        numara.setCharacterSize(16);
        numara.setFillColor(sf::Color(200, 200, 200));
        numara.setString(std::to_string(i + 1));
        numara.setPosition(-20, (7 - i) * KARE_BOYUTU + KARE_BOYUTU/2 - 10);
        pencere.draw(numara);
    }
}

// Renkleri ayarla
void Arayuz::renkleriAyarla() {
    beyazKareRengi = sf::Color(240, 217, 181);
    siyahKareRengi = sf::Color(181, 136, 99);
    seciliKareRengi = sf::Color(255, 255, 0, 128);
    legalHamleRengi = sf::Color(0, 255, 0, 100);
    sonHamleKaynakRengi = sf::Color(255, 255, 0, 80);
    sonHamleHedefRengi = sf::Color(255, 255, 0, 120);
    sahKontrolRengi = sf::Color(255, 0, 0, 150);
    rokHamleRengi = sf::Color(0, 150, 255, 120);  // Mavi renk rok hamlesi için
    panelArkaplanRengi = sf::Color(50, 50, 50);
}

// Fontları yükle
bool Arayuz::fontlariYukle() {
    // Proje içindeki font dosyasını yüklemeyi dene
    if (!anaFont.loadFromFile("kaynaklar/fonts/DejaVuSans.ttf")) {
        std::cerr << "Font 'kaynaklar/fonts/DejaVuSans.ttf' yüklenemedi.\n";
        // Sistem fontunu dene
        if (!anaFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Sistem fontu da yüklenemedi, varsayılan font kullanılacak.\n";
            // SFML'in varsayılan fontunu kullan
            return true; // Font olmadan da çalışabilir
        }
    }

    sembolFont = anaFont;
    return true;
}

// Taş görsellerini yükle
bool Arayuz::tasGorselleriniYukle() {
    // Taş dosya isimleri
    const std::vector<std::pair<Renk, TasTuru>> taslar = {
        {Renk::BEYAZ, TasTuru::PIYON},
        {Renk::BEYAZ, TasTuru::AT},
        {Renk::BEYAZ, TasTuru::FIL},
        {Renk::BEYAZ, TasTuru::KALE},
        {Renk::BEYAZ, TasTuru::VEZIR},
        {Renk::BEYAZ, TasTuru::SAH},
        {Renk::SIYAH, TasTuru::PIYON},
        {Renk::SIYAH, TasTuru::AT},
        {Renk::SIYAH, TasTuru::FIL},
        {Renk::SIYAH, TasTuru::KALE},
        {Renk::SIYAH, TasTuru::VEZIR},
        {Renk::SIYAH, TasTuru::SAH}
    };
    
    // Her taş için
    for (const auto& [renk, tas] : taslar) {
        std::string dosyaAdi = "kaynaklar/";
        
        // Renk adı
        dosyaAdi += (renk == Renk::BEYAZ) ? "beyaz" : "siyah";
        
        // Taş adı
        switch (tas) {
            case TasTuru::PIYON: dosyaAdi += "Piyon"; break;
            case TasTuru::AT: dosyaAdi += "At"; break;
            case TasTuru::FIL: dosyaAdi += "Fil"; break;
            case TasTuru::KALE: dosyaAdi += "Kale"; break;
            case TasTuru::VEZIR: dosyaAdi += "Vezir"; break;
            case TasTuru::SAH: dosyaAdi += "Sah"; break;
            default: continue;
        }
        
        dosyaAdi += ".png";
        
        // Texture yükle
        sf::Texture texture;
        if (!texture.loadFromFile(dosyaAdi)) {
            std::cerr << "Taş görseli yüklenemedi: " << dosyaAdi << std::endl;
            continue;
        }
        
        // Smooth filtre
        texture.setSmooth(true);
        
        // Texture'ı kaydet
        tasTextureleri[{renk, tas}] = texture;
        
        // Sprite oluştur
        sf::Sprite sprite;
        sprite.setTexture(tasTextureleri[{renk, tas}]);
        
        // Sprite'ı tahtadaki kare boyutuna göre ölçekle
        float olcek = (float)KARE_BOYUTU / texture.getSize().x;
        sprite.setScale(olcek, olcek);
        
        // Sprite'ı kaydet
        tasSpriteleri[{renk, tas}] = sprite;
    }
    
    return !tasSpriteleri.empty();
}

// Piksel koordinatından kare indeksine çevir
int Arayuz::pixeldenKareye(const sf::Vector2f& pozisyon) const {
    // Sınır kontrolü
    if (!tahtaIcindeMi(pozisyon)) {
        return -1;
    }
    
    int sutun = static_cast<int>(pozisyon.x / KARE_BOYUTU);
    int satir = static_cast<int>(pozisyon.y / KARE_BOYUTU);
    
    // Sınır kontrolü
    if (sutun < 0 || sutun >= 8 || satir < 0 || satir >= 8) {
        return -1;
    }
    
    // Y koordinatını ters çevir (SFML'de Y yukarıdan aşağıya, satrançta aşağıdan yukarıya)
    satir = 7 - satir;
    
    return kareIndeksi(satir, sutun);
}

// Kare indeksinden piksel koordinatına çevir
sf::Vector2f Arayuz::kareyePixel(int kare) const {
    int satir = satirIndeksi(kare);
    int sutun = sutunIndeksi(kare);
    return sf::Vector2f(sutun * KARE_BOYUTU, (7 - satir) * KARE_BOYUTU);
}

// Tahta içinde mi?
bool Arayuz::tahtaIcindeMi(const sf::Vector2f& pozisyon) const {
    return pozisyon.x >= 0 && pozisyon.x < TAHTA_BOYUTU &&
           pozisyon.y >= 0 && pozisyon.y < TAHTA_BOYUTU;
}

// Dikdörtgen çiz
void Arayuz::dikdortgenCiz(int kare, const sf::Color& renk, float kalinlik) {
    sf::Vector2f pozisyon = kareyePixel(kare);
    sf::RectangleShape dikdortgen(sf::Vector2f(KARE_BOYUTU, KARE_BOYUTU));
    dikdortgen.setPosition(pozisyon);
    dikdortgen.setFillColor(sf::Color::Transparent);
    dikdortgen.setOutlineColor(renk);
    dikdortgen.setOutlineThickness(kalinlik);
    pencere.draw(dikdortgen);
}

// Daire çiz
void Arayuz::daireCiz(int kare, const sf::Color& renk, float yariCap) {
    sf::Vector2f pozisyon = kareyePixel(kare);
    sf::CircleShape daire(yariCap);
    daire.setPosition(pozisyon.x + KARE_BOYUTU/2 - yariCap,
                     pozisyon.y + KARE_BOYUTU/2 - yariCap);
    daire.setFillColor(renk);
    pencere.draw(daire);
}

// Sprite konumlandır
void Arayuz::spriteKonumlandir(sf::Sprite& sprite, int kare) {
    sf::Vector2f pozisyon = kareyePixel(kare);
    sprite.setPosition(pozisyon);
}

// Bilgi güncelleme
void Arayuz::hamleEkle(const Hamle& hamle, const std::string& notasyon) {
    hamleGecmisi.push_back(notasyon);
    sonHamle = hamle;
}

void Arayuz::motorBilgisiGuncelle(int derinlik, int skor, uint64_t dugumSayisi, 
                                 const std::string& dusunce) {
    motorDerinlik = derinlik;
    motorSkor = skor;
    motorDugumSayisi = dugumSayisi;
    motorDusuncesi = dusunce;
}

void Arayuz::oyunSonuBildir(const std::string& sonuc) {
    // Oyun sonu bildirimini göster
    std::cout << "Oyun Bitti: " << sonuc << std::endl;
}

// Terfi seçimi
void Arayuz::terfiSeciminiGoster(int kaynak, int hedef) {
    terfiSecimAktif = true;
    terfiKaynak = kaynak;
    terfiHedef = hedef;
}

void Arayuz::terfiTasiniSec(TasTuru tas) {
    if (terfiSecimAktif && oyun) {
        oyun->hamleYap(terfiKaynak, terfiHedef, tas);
        terfiSecimAktif = false;
    }
}

// Terfi seçimi tıklama işle
void Arayuz::terfiSecimiTiklamaIsle(const sf::Vector2f& pozisyon) {
    float merkezX = TAHTA_BOYUTU / 2.0f;
    float merkezY = TAHTA_BOYUTU / 2.0f;
    float tasArasi = KARE_BOYUTU * 1.5f;
    
    const TasTuru taslar[] = {TasTuru::VEZIR, TasTuru::KALE, TasTuru::FIL, TasTuru::AT};
    
    for (int i = 0; i < 4; i++) {
        float x = merkezX - (1.5f - i) * tasArasi;
        float y = merkezY - KARE_BOYUTU / 2;
        
        // Tıklama bu taş üzerinde mi?
        if (pozisyon.x >= x && pozisyon.x <= x + KARE_BOYUTU &&
            pozisyon.y >= y && pozisyon.y <= y + KARE_BOYUTU) {
            terfiTasiniSec(taslar[i]);
            break;
        }
    }
}

// Terfi penceresi çiz
void Arayuz::terfiPenceresiCiz() {
    // Arka plan
    sf::RectangleShape arkaplan(sf::Vector2f(TAHTA_BOYUTU, TAHTA_BOYUTU));
    arkaplan.setFillColor(sf::Color(0, 0, 0, 180));
    pencere.draw(arkaplan);
    
    // Terfi seçenekleri merkezi
    float merkezX = TAHTA_BOYUTU / 2.0f;
    float merkezY = TAHTA_BOYUTU / 2.0f;
    float tasArasi = KARE_BOYUTU * 1.5f;
    
    // Renk belirle
    Renk renk = oyun->getSira();
    
    // Terfi taşları
    const TasTuru taslar[] = {TasTuru::VEZIR, TasTuru::KALE, TasTuru::FIL, TasTuru::AT};
    const char* tasIsimleri[] = {"Vezir", "Kale", "Fil", "At"};
    
    for (int i = 0; i < 4; i++) {
        float x = merkezX - (1.5f - i) * tasArasi;
        float y = merkezY - KARE_BOYUTU / 2;
        
        // Kare çerçevesi
        sf::RectangleShape cerceve(sf::Vector2f(KARE_BOYUTU, KARE_BOYUTU));
        cerceve.setPosition(x, y);
        cerceve.setFillColor(sf::Color(200, 200, 200));
        cerceve.setOutlineColor(sf::Color::Black);
        cerceve.setOutlineThickness(2);
        pencere.draw(cerceve);
        
        // Taş görseli
        auto it = tasSpriteleri.find({renk, taslar[i]});
        if (it != tasSpriteleri.end()) {
            sf::Sprite sprite = it->second;
            sprite.setPosition(x, y);
            const sf::Texture* texture = sprite.getTexture();
            if (texture) {
                sprite.setScale((float)KARE_BOYUTU / texture->getSize().x,
                              (float)KARE_BOYUTU / texture->getSize().y);
            }
            pencere.draw(sprite);
        }
        
        // Taş ismi
        sf::Text isim;
        isim.setFont(anaFont);
        isim.setString(tasIsimleri[i]);
        isim.setCharacterSize(16);
        isim.setFillColor(sf::Color::White);
        isim.setPosition(x + 10, y + KARE_BOYUTU + 5);
        pencere.draw(isim);
    }
    
    // Başlık
    sf::Text baslik;
    baslik.setFont(anaFont);
    baslik.setString("Terfi Tasi Secin");
    baslik.setCharacterSize(24);
    baslik.setFillColor(sf::Color::White);
    baslik.setPosition(merkezX - 100, merkezY - KARE_BOYUTU - 50);
    pencere.draw(baslik);
}

// Animasyon güncelle
void Arayuz::animasyonuGuncelle() {
    // Şimdilik animasyon yok
    hamleAnimasyonu = false;
}

// Klavye girdisi işle
void Arayuz::klavyeGirdisiniIsle(const sf::Event& olay) {
    // FEN girişi aktifse
    if (fenGirisAktif) {
        if (olay.key.code == sf::Keyboard::Enter) {
            fenYukle(fenMetni);
            fenGirisiniKapat();
        } else if (olay.key.code == sf::Keyboard::Escape) {
            fenGirisiniKapat();
        }
        return;
    }
    
    switch (olay.key.code) {
        case sf::Keyboard::Escape:
            pencere.close();
            break;
            
        case sf::Keyboard::R:
            // Yeni oyun
            if (oyun) {
                oyun->yeniOyun();
                hamleGecmisi.clear();
                sonHamle = Hamle();
                sadeceDegerlendirmeModAktif = false;
            }
            break;
            
        case sf::Keyboard::U:
            // Hamle geri al
            if (oyun && !sadeceDegerlendirmeModAktif) {
                oyun->hamleGeriAl();
                if (!hamleGecmisi.empty()) {
                    hamleGecmisi.pop_back();
                }
            }
            break;
            
        case sf::Keyboard::F:
            // FEN girişini göster
            fenGirisiniGoster();
            break;
            
        default:
            break;
    }
}

// FEN girişini göster
void Arayuz::fenGirisiniGoster() {
    fenGirisAktif = true;
    fenMetni = "";
}

// FEN girişini kapat
void Arayuz::fenGirisiniKapat() {
    fenGirisAktif = false;
    fenMetni = "";
}

// FEN yükle
void Arayuz::fenYukle(const std::string& fen) {
    if (!oyun) return;
    
    // Exception handling devre dışı
    oyun->fenYukle(fen);
    hamleGecmisi.clear();
    sonHamle = Hamle();
    sadeceDegerlendirmeModAktif = true;
    
    // Pozisyonu değerlendir
    int skor = Degerlendirici::degerlendir(oyun->getTahta());
    
    // Sıranın perspektifinden göster
    if (oyun->getTahta().getSira() == Renk::SIYAH) {
        skor = -skor;
    }
    
    // Motor bilgisini güncelle
    motorBilgisiGuncelle(0, skor, 0, "Pozisyon yuklendi");
    
    // En iyi hamle önerisi için motoru başlat
    if (!oyun->siradakiOyuncuInsan()) {
        oyun->motoruBaslat();
    }
}

// FEN girişine çiz
void Arayuz::fenGirisineCiz() {
    if (!fenGirisAktif) return;
    
    // Arka plan
    sf::RectangleShape arkaplan(sf::Vector2f(500, 100));
    arkaplan.setPosition(70, 300);
    arkaplan.setFillColor(sf::Color(50, 50, 50, 230));
    arkaplan.setOutlineColor(sf::Color::White);
    arkaplan.setOutlineThickness(2);
    pencere.draw(arkaplan);
    
    // Başlık
    sf::Text baslik;
    baslik.setFont(anaFont);
    baslik.setCharacterSize(18);
    baslik.setFillColor(sf::Color::White);
    baslik.setString("FEN Pozisyonu Girin:");
    baslik.setPosition(85, 310);
    pencere.draw(baslik);
    
    // Giriş kutusu
    sf::RectangleShape girisKutusu(sf::Vector2f(470, 30));
    girisKutusu.setPosition(85, 340);
    girisKutusu.setFillColor(sf::Color(30, 30, 30));
    girisKutusu.setOutlineColor(sf::Color::White);
    girisKutusu.setOutlineThickness(1);
    pencere.draw(girisKutusu);
    
    // FEN metni
    sf::Text fenText;
    fenText.setFont(anaFont);
    fenText.setCharacterSize(14);
    fenText.setFillColor(sf::Color::White);
    fenText.setString(fenMetni);
    fenText.setPosition(90, 345);
    pencere.draw(fenText);
    
    // İmleç
    if (static_cast<int>(clock.getElapsedTime().asSeconds() * 2) % 2 == 0) {
        sf::RectangleShape imlec(sf::Vector2f(2, 20));
        imlec.setPosition(fenText.getPosition().x + fenText.getLocalBounds().width + 2, 345);
        imlec.setFillColor(sf::Color::White);
        pencere.draw(imlec);
    }
    
    // Yardım metni
    sf::Text yardim;
    yardim.setFont(anaFont);
    yardim.setCharacterSize(12);
    yardim.setFillColor(sf::Color(200, 200, 200));
    yardim.setString("Enter: Yukle | Esc: Iptal");
    yardim.setPosition(85, 375);
    pencere.draw(yardim);
}